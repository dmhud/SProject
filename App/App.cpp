#include "App.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include <restbed>
#include <utility>
#include <fmt/format.h>

#include <regex>

namespace app
{
    App::App(AppOptions&& options)
        : _options(std::forward<AppOptions>(options))
    {
    }

    void App::Run()
    {
        using namespace std::placeholders;

        _db.Connect(_options.DB);

        // Get devices from DB
        _devicesIds = _db.ReadDevicesNames();
                       
        _service = std::make_shared<restbed::Service>();
        PublishResources();

        auto settings = std::make_shared<restbed::Settings>();
        settings->set_port((uint16_t)_options.App.port);
        settings->set_default_header("Connection", "close");
        settings->set_worker_limit(_options.App.worker_count);


        _mqtt.SetDevMessageCallback([service = _service, onDevMessage = std::bind(&App::OnMqttDevMessage, this, _1)](DevParam devParam)
        {
            service->schedule(std::bind(onDevMessage, std::move(devParam)));
        });
        _mqtt.Connect(_options.MQTT);
        _mqtt.Start();

        _service->start(settings);
    }

    void App::PublishResources()
    {
        using namespace std::placeholders;
        std::multimap<std::string, std::string> jsonFilters =
        {
            { "Accept",       "application/json" },
            { "Content-Type", "application/json" }
        };
        
        // GET/POST/DELETE devices
        {
            auto resource = std::make_shared<restbed::Resource>();
            resource->set_paths({
                    "/devices",
                    "/devices/",
                    "/devices/{deviceID: .*}"
                });

            resource->set_method_handler("GET",    jsonFilters, std::bind(&App::HTTP_GET_Devices,    this, _1));
            resource->set_method_handler("POST",   jsonFilters, std::bind(&App::HTTP_POST_Devices,   this, _1));
            resource->set_method_handler("DELETE", jsonFilters, std::bind(&App::HTTP_DELETE_Devices, this, _1));

            _service->publish(resource);
        }
    }

    void App::HTTP_GET_Devices(SharedSession session)
    {
        using namespace std::chrono;
        auto timestampSeconds = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

        std::vector<Device> devices;

        const auto request = session->get_request();
        if (request->has_path_parameter("deviceID"))
        {
            std::string deviceId = request->get_path_parameter("deviceID");
            std::cout << "Get deviceID: " << deviceId << std::endl;
            devices = GetDevice(deviceId);
        }
        else
        {
            std::cout << "Get all devices" << std::endl;
            devices = GetAllDevices();
        }

        // Calc changeTimestamp
        for (auto& device : devices)
            for (auto& it : device.flags)
            {
                auto& flag = it.second;
                if (flag.value == true)
                    flag.changeTimestamp = timestampSeconds - flag.timestamp;
            }        

        // Calc dynamic flag
        size_t threshold = _options.App.change_timestamp_threshold;
        for (auto& device : devices)
        {
            device.CalcDynamicFlags(threshold);
        }

        // Devices to JSON
        json jsonData{ {"devices",devices}, };
        std::string strJson = jsonData.dump(4);
        SessionClose_JSON(session, restbed::OK, jsonData.dump(4));
    }

    void App::HTTP_POST_Devices(SharedSession session)
    {
        static std::string jsonDevicesIdsKey = "devicesIds";

        const auto request = session->get_request();
        std::size_t length = request->get_header<size_t>("Content-Length", 0);
        if (length == 0)
        {
            if (request->has_path_parameter("deviceID"))
            {
                // Single device from path parameter
                std::string deviceId = request->get_path_parameter("deviceID");
                CreateDevices({ deviceId });
                session->close(restbed::OK);
            }
            else
            {
                SessionClose_TEXT(session, restbed::NOT_ACCEPTABLE, "Not Acceptable, empty body");
            }
        }
        else
        {
            session->fetch(length, [this](SharedSession s, const restbed::Bytes &data)
            {
                // Devices from JSON
                json jsonData;
                try
                {
                    jsonData = json::parse(data);
                }
                catch (std::exception& ex)
                {
                    std::cout << "JSON parse error: " << ex.what() << std::endl;
                    s->close(restbed::BAD_REQUEST);
                    return;
                }

                if (!jsonData.contains(jsonDevicesIdsKey))
                {
                    std::cout << fmt::format("JSON no key: \"{}\"", jsonDevicesIdsKey) << std::endl;
                    s->close(restbed::BAD_REQUEST);
                    return;
                }

                auto devices = jsonData[jsonDevicesIdsKey].get<std::set<std::string>>();
                CreateDevices(std::move(devices));
                s->close(restbed::OK);
            });
        }
    }

    void App::HTTP_DELETE_Devices(SharedSession session)
    {
        static std::string jsonDevicesIdsKey = "devicesIds";

        const auto request = session->get_request();
        std::size_t length = request->get_header<size_t>("Content-Length", 0);
        if (length == 0)
        {
            if (request->has_path_parameter("deviceID"))
            {
                // Single device from path parameter
                std::string deviceId = request->get_path_parameter("deviceID");
                DeleteDevices({ deviceId });
                session->close(restbed::OK);
            }
            else
            {
                DeleteAllDevices();
                session->close(restbed::OK);
            }
        }
        else
        {
            session->fetch(length, [this](SharedSession s, const restbed::Bytes &data)
            {
                // Devices from JSON
                json jsonData;
                try
                {
                    jsonData = json::parse(data);
                }
                catch (std::exception& ex)
                {
                    std::cout << "JSON parse error: " << ex.what() << std::endl;
                    s->close(restbed::BAD_REQUEST);
                    return;
                }

                if (!jsonData.contains(jsonDevicesIdsKey))
                {
                    std::cout << fmt::format("JSON no key: \"{}\"", jsonDevicesIdsKey) << std::endl;
                    s->close(restbed::BAD_REQUEST);
                    return;
                }

                auto devices = jsonData[jsonDevicesIdsKey].get<std::set<std::string>>();
                DeleteDevices(std::move(devices));
                s->close(restbed::OK);
            });
        }
    }

    void App::CreateDevices(std::set<std::string> devicesIds)
    {
        std::lock_guard<std::shared_timed_mutex> lock(_syncDevices);

        std::set<std::string> addDevices;
        std::set_difference(
            devicesIds.begin(), devicesIds.end(),
            _devicesIds.begin(), _devicesIds.end(),
            std::inserter(addDevices, addDevices.end()));

        std::cout << "Create new devicesIds:" << std::endl;
        for (auto& devName : addDevices)
        {
            std::cout << "\t" << devName << std::endl;
        }
        std::cout << std::endl;

        // Add devices to DB
        Device device;
        for (auto& deviceName : addDevices)
        {
            device.name = deviceName;
            _db.CreateDevice(device);
        }

        _devicesIds.insert(addDevices.begin(), addDevices.end());
    }

    void App::DeleteDevices(std::set<std::string> devicesIds)
    {
        std::lock_guard<std::shared_timed_mutex> lock(_syncDevices);

        std::set<std::string> deleteDevices;
        std::set_intersection(
            devicesIds.begin(), devicesIds.end(),
            _devicesIds.begin(), _devicesIds.end(),
            std::inserter(deleteDevices, deleteDevices.end()));

        std::cout << "Delete devicesIds:" << std::endl;
        for (auto& devName : deleteDevices)
        {
            std::cout << "\t" << devName << std::endl;
        }
        std::cout << std::endl;

        // Delete devices from DB
        for (auto& deviceName : deleteDevices)
        {
            _db.DeleteDevice(deviceName);
        }

        std::set<std::string> remainingDevices;
        std::set_difference(
            _devicesIds.begin(), _devicesIds.end(),
            deleteDevices.begin(), deleteDevices.end(),
            std::inserter(remainingDevices, remainingDevices.end()));
        _devicesIds = std::move(remainingDevices);
    }

    void App::DeleteAllDevices()
    {
        std::lock_guard<std::shared_timed_mutex> lock(_syncDevices);

        std::cout << "Delete all devices" << std::endl;
        
        // Delete devices from DB
        for (auto& deviceName : _devicesIds)
        {
            _db.DeleteDevice(deviceName);
        }
        _devicesIds.clear();
    }

    std::vector<Device> App::GetDevice(std::string deviceId)
    {
        std::shared_lock<std::shared_timed_mutex> lock(_syncDevices);

        auto it = _devicesIds.find(deviceId);
        if (it == _devicesIds.end())
            return {};

        Device device = _db.ReadDevice(deviceId);
        return { device };
    }

    std::vector<Device> App::GetAllDevices()
    {
        std::shared_lock<std::shared_timed_mutex> lock(_syncDevices);

        std::vector<Device> devices;
        devices.reserve(_devicesIds.size());

        for (auto deviceId : _devicesIds)
        {
            Device device = _db.ReadDevice(deviceId);            
            devices.push_back(device);
        }

        return devices;
    }

    void App::OnMqttDevMessage(DevParam devParam)
    {
        using namespace std::chrono;
        auto timestampSeconds = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

        //std::cout << "OnMqttDevMessage:" << std::endl;
        //std::cout << "\t devName: " << devParam.devName
        //          << "\t param: " << devParam.paramId
        //          << " value: " << devParam.paramValue << std::endl;

        std::lock_guard<std::shared_timed_mutex> lock(_syncDevices);

        //std::cout << "Current devices: "<< std::endl;
        //for (auto& dev : _devicesIds)
        //{
        //    std::cout << "\t " << dev << std::endl;
        //}

        auto it = _devicesIds.find(devParam.devName);
        if (it == _devicesIds.end())
        {
            //std::cout << "OnMqttDevMessage: ignore devName: " << devParam.devName << std::endl;
            return; // Ignore other devices
        }

        // Update flags
        auto device = _db.ReadDevice(devParam.devName);
        bool isUpdated = false;

        // flags mapping:
        if (devParam.paramId == 1 && devParam.paramValue == 0)
        {
            std::cout << "OnMqttDevMessage: update flag1" << std::endl;
            device.flags["flag1"].value = true;
            device.flags["flag1"].timestamp = timestampSeconds;
            isUpdated = true;
        }
        else if (devParam.paramId == 1 && devParam.paramValue == 1)
        {
            std::cout << "OnMqttDevMessage: update flag2" << std::endl;
            device.flags["flag2"].value = true;
            device.flags["flag2"].timestamp = timestampSeconds;
            isUpdated = true;
        }
        else if (devParam.paramId == 2 && devParam.paramValue > 11)
        {
            std::cout << "OnMqttDevMessage: update flag3" << std::endl;
            device.flags["flag3"].value = true;
            device.flags["flag3"].timestamp = timestampSeconds;
            isUpdated = true;
        }
        
        if (isUpdated)
            _db.UpdateDeviceFlags(std::move(device));
    }
    
    void App::SessionClose_JSON(const SharedSession& session, int statusCode, const std::string& json)
    {
        session->close(statusCode, json, {
            {"Content-Type",   "application/json"},
            {"Content-Length", std::to_string(json.size())}
        });
    }

    void App::SessionClose_TEXT(const SharedSession& session, int statusCode, const std::string& msg)
    {
        session->close(statusCode, msg, {
            {"Content-Type",   "text/plain"},
            {"Content-Length", std::to_string(msg.size())}
        });
    }
} // namespace app
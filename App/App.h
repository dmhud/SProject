#pragma once

#include <mutex>
#include <shared_mutex>
#include <set>

#include "AppOptions.h"
#include "Mqtt.h"
#include "DB.h"
#include "Device.h"

namespace restbed
{
    class Service;
    class Session;
}

namespace app
{
    using SharedSession = std::shared_ptr<restbed::Session>;

    class App
    {
    public:
        explicit App(AppOptions&& options);
        void Run();
        
    private:
        void PublishResources();
        
        void HTTP_GET_Devices(SharedSession session);
        void HTTP_POST_Devices(SharedSession session);
        void HTTP_DELETE_Devices(SharedSession session);
        
        void SessionClose_JSON(const SharedSession& session, int statusCode, const std::string& json);
        void SessionClose_TEXT(const SharedSession& session, int statusCode, const std::string& msg);

        void CreateDevices(std::set<std::string> devicesIds);
        void DeleteDevices(std::set<std::string> devicesIds);
        void DeleteAllDevices();
        std::vector<Device> GetDevice(std::string deviceId);
        std::vector<Device> GetAllDevices();

        void OnMqttDevMessage(DevParam devParam);
        
    private:
        std::shared_ptr<restbed::Service> _service;

        AppOptions _options;
        Mqtt       _mqtt;
        DB         _db;

        std::set<std::string>   _devicesIds;
        std::shared_timed_mutex _syncDevices;
    };
} // namespace app
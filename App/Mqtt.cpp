#include "Mqtt.h"

#include <functional>
#include <mosquitto.h>
#include <regex>
#include <stdexcept>

#include <fmt/format.h>
#include <iostream>

namespace app
{
    Mqtt::Mqtt()
    {    
        mosquitto_lib_init(); 
    }

    Mqtt::~Mqtt()
    {
        mosquitto_loop_stop(_mosq.get(), true);
        _mosq.reset();

        mosquitto_lib_cleanup();
    }

    void Mqtt::SetDevMessageCallback(const DevMessageCallback& callback)
    {
        _onDevMessage = callback;
    }

    void Mqtt::Connect(const MqttParams& p)
    {
        using namespace std::placeholders;

        _topic = p.topic;

        _mosq = mosquitto_ptr(mosquitto_new(NULL, true, this), [](mosquitto* ptr) { mosquitto_destroy(ptr); });
        if (!_mosq)
        {
            throw std::runtime_error("MQTT Error: mosquitto_new - Out of memory.");
        }

        mosquitto_connect_callback_set(_mosq.get(), [](mosquitto* mosq, void* obj, int reasonCode)
        {
            Mqtt* thisObj = static_cast<Mqtt*>(obj);
            if (thisObj)
                thisObj->OnConnect(reasonCode);
        });
        mosquitto_subscribe_callback_set(_mosq.get(), [](struct mosquitto *mosq, void *obj, int mid, int qosCount, const int *grantedQos)
        {
            Mqtt* thisObj = static_cast<Mqtt*>(obj);
            if (thisObj)
                thisObj->OnSubscribe(mid, qosCount, grantedQos);
        });
        mosquitto_message_callback_set(_mosq.get(), [](struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg)
        {
            Mqtt* thisObj = static_cast<Mqtt*>(obj);
            if (thisObj)
                thisObj->OnMessage(msg);
        });

        int rc = mosquitto_connect(_mosq.get(), p.host.data(), p.port, p.timeout);
        if (rc != MOSQ_ERR_SUCCESS) 
        {
            _mosq.reset();
            throw std::runtime_error(fmt::format("MQTT Error: {}", mosquitto_strerror(rc)));
        }
    }

    void Mqtt::Start()
    {
        mosquitto_loop_start(_mosq.get());
    }

    void Mqtt::OnConnect(int reasonCode)
    {
        std::cerr << fmt::format("MQTT: OnConnect: {}", mosquitto_connack_string(reasonCode)) << std::endl;
        if (reasonCode != 0) 
        {
            mosquitto_disconnect(_mosq.get());
        }

        int rc = mosquitto_subscribe(_mosq.get(), NULL, _topic.data(), 1);
        if (rc != MOSQ_ERR_SUCCESS) 
        {
            std::cerr << fmt::format("MQTT Error - subscribing: {}", mosquitto_strerror(rc)) << std::endl;
            mosquitto_disconnect(_mosq.get());
        }
    }

    void Mqtt::OnSubscribe(int mid, int qosCount, const int* grantedQos)
    {
        int i;
        bool have_subscription = false;

        for (i = 0; i < qosCount; i++) 
        {
            std::cout << fmt::format("MQTT: OnSubscribe: {}:granted qos = {}", i, grantedQos[i]) << std::endl;
            if (grantedQos[i] <= 2) {
                have_subscription = true;
            }
        }
        if (have_subscription == false) 
        {
            std::cerr << "MQTT Error: All subscriptions rejected." << std::endl;
            mosquitto_disconnect(_mosq.get());
        }
    }

    void Mqtt::OnMessage(const mosquitto_message* msg)
    {
        //static std::string delimiter = "/";
        //std::string devId = msg->topic;
        //size_t pos = devId.find(delimiter);
        //if (pos != std::string::npos)
        //    devId.erase(pos);
        //std::cout << "\tMQTT topic: device_id: " << devId << std::endl;

        std::string s = (char *)msg->payload;

        // regex for parse dev_eui,param_id,value:
        //      \{\s*"dev_eui"\s*:\s*"([^"]*)"\s*,\s*"param_id"\s*:\s*([^,\s]*)\s*,\s*"value"\s*:\s*([^,\s]*)\s*\}
        static std::regex r("\\{\\s*\"dev_eui\"\\s*:\\s*\"([^\"]*)\"\\s*,\\s*\"param_id\"\\s*:\\s*([^,\\s]*)\\s*,\\s*\"value\"\\s*:\\s*([^,\\s]*)\\s*\\}");

        for (auto it = std::sregex_iterator(s.begin(), s.end(), r); it != std::sregex_iterator(); ++it)
        {
            std::smatch m = *it;
            if (m.size() >= 4)
            try
            {  
                DevParam devParam;
                devParam.devName          = { m[1].first, m[1].second };
                std::string strParamId    = { m[2].first, m[2].second };
                std::string strParamValue = { m[3].first, m[3].second };
                devParam.paramId = std::stoi(strParamId);
                devParam.paramValue = std::stoi(strParamValue);

                if (_onDevMessage)
                    _onDevMessage(std::move(devParam));
            }
            catch (std::exception& ex)
            {
                std::cout << "MQTT Error: parse message: " << ex.what() << std::endl;
            }
        }
    }

} // namespace app
#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "AppOptions.h"

struct mosquitto;
struct mosquitto_message;

namespace app
{
    struct DevParam
    {
        std::string devName;
        int         paramId;
        int         paramValue;
    };

    class Mqtt
    {
    public:
        using DevMessageCallback = std::function<void(DevParam devParam)>;

        Mqtt();
        ~Mqtt();

        void SetDevMessageCallback(const DevMessageCallback& callback);
        void Connect(const MqttParams& p);
        void Start();
        
    private:
        void OnConnect(int reasonCode);
        void OnSubscribe(int mid, int qosCount, const int *grantedQos);
        void OnMessage(const struct mosquitto_message* message);

    private:
        using mosquitto_ptr = std::unique_ptr<mosquitto, std::function<void(mosquitto*)>>;
        mosquitto_ptr _mosq;

        DevMessageCallback _onDevMessage;
        std::string _topic;
    };
} // namespace app
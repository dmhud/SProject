#pragma once

#include <string>

namespace app
{
    struct AppParams
    {
        uint16_t port;
        uint32_t worker_count;
        size_t   change_timestamp_threshold;
    };

    struct DBConnectionParams
    {
        std::string dbname;
        std::string user;
        std::string password;
        std::string host;
        uint16_t    port;
        size_t      timeout;

        size_t      pool_size = 1;
    };

    struct MqttParams
    {
        std::string host;
        uint16_t    port;
        size_t      timeout;
        std::string topic;
    };

    struct AppOptions
    {
        AppParams          App;
        DBConnectionParams DB;
        MqttParams         MQTT;

        static AppOptions FromArgs(int argc, char** argv);
    };

} // namespace app
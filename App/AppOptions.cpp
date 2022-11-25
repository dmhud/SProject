#include "AppOptions.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace app
{
    AppOptions AppOptions::FromArgs(int argc, char** argv)
    {        
        po::options_description desc("Options");
        desc.add_options()
            //
            ("app_port",                       po::value<uint16_t>()->required(),       "REST port")
            ("app_worker_count",               po::value<uint32_t>()->default_value(0), "REST worker count")
            ("app_change_timestamp_threshold", po::value<size_t>()->default_value(std::numeric_limits<size_t>::max()), 
                                                                                        "REST change_timestamp_threshold")
            //               
            ("db_name",      po::value<std::string>()->required(),         "DataBase name")
            ("db_user",      po::value<std::string>()->required(),         "DataBase user")
            ("db_password",  po::value<std::string>()->required(),         "DataBase password")
            ("db_host",      po::value<std::string>()->required(),         "DataBase host IP")
            ("db_port",      po::value<uint16_t>()->required(),            "DataBase port")
            ("db_timeout",   po::value<size_t>()->required(),              "DataBase timeout")
            ("db_pool_size", po::value<size_t>()->default_value(1),        "DataBase pool size")
            //
            ("mqtt_host",    po::value<std::string>()->required(),         "MQTT host")
            ("mqtt_port",    po::value<uint16_t>()->required(),            "MQTT port")
            ("mqtt_timeout", po::value<size_t>()->required(),              "MQTT timeout")
            ("mqtt_topic",   po::value<std::string>()->default_value("#"), "MQTT topic")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        AppOptions options;
        options.App.port                       = vm["app_port"].as<uint16_t>();
        options.App.worker_count               = vm["app_worker_count"].as<uint32_t>();
        options.App.change_timestamp_threshold = vm["app_change_timestamp_threshold"].as<size_t>();

        options.DB.dbname    = vm["db_name"].as<std::string>();
        options.DB.user      = vm["db_user"].as<std::string>();
        options.DB.password  = vm["db_password"].as<std::string>();
        options.DB.host      = vm["db_host"].as<std::string>();
        options.DB.port      = vm["db_port"].as<uint16_t>();
        options.DB.timeout   = vm["db_timeout"].as<size_t>();
        options.DB.pool_size = vm["db_pool_size"].as<size_t>();
                             
        options.MQTT.host    = vm["mqtt_host"].as<std::string>();
        options.MQTT.port    = vm["mqtt_port"].as<uint16_t>();
        options.MQTT.timeout = vm["mqtt_timeout"].as<size_t>();
        options.MQTT.topic   = vm["mqtt_topic"].as<std::string>();

        return options;
    }
} // namespace app
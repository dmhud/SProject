cmake_minimum_required(VERSION 3.22)

project ("App" CXX) 

# Add source to this project's executable.
add_executable (${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp            
        App.cpp
        App.h    
        AppOptions.cpp
        AppOptions.h
        Mqtt.cpp
        Mqtt.h
        DB.cpp
        DB.h
        Device.cpp
        Device.h
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        fmt::fmt
        Boost::program_options
        restbed::restbed
        nlohmann_json::nlohmann_json
        SOCI::SOCI
        mosquitto::mosquitto
)

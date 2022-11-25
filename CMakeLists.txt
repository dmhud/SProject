cmake_minimum_required(VERSION 3.22)

project ("SProject" CXX)

# Add pakcages from conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)


find_package(fmt REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(restbed REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SOCI REQUIRED)
find_package(mosquitto REQUIRED)

# Include sub-projects.
add_subdirectory ("App")

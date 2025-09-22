cmake_minimum_required(VERSION 3.29)
project(MqttCpp LANGUAGES CXX)

if(CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
  set(CMAKE_DEFAULT_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wunused -Wnrvo)

set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "/usr/local/qt6/lib;/usr/local/gcc-15/lib64")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Suppress warnings about deprecated is_trivial_v in C++26
if(CMAKE_CXX_STANDARD GREATER_EQUAL 26)
    add_compile_options(-Wno-deprecated)
endif()

list(PREPEND CMAKE_PREFIX_PATH "/usr/local/jsoncpp-1.9.7")
find_package(jsoncpp CONFIG REQUIRED)

find_package(PahoMqttCpp NAMES PahoMqttCpp eclipse-paho-mqtt-cpp CONFIG REQUIRED)

find_package(PahoMqttC CONFIG QUIET)
if (NOT PahoMqttC_FOUND)
  find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
endif()

if (TARGET eclipse-paho-mqtt-c::paho-mqtt3as)
  set(PAHO_C_TARGET eclipse-paho-mqtt-c::paho-mqtt3as)
elseif (TARGET PahoMqttC::paho-mqtt3as)
  set(PAHO_C_TARGET PahoMqttC::paho-mqtt3as)
else()
  message(FATAL_ERROR "paho-mqtt3as target not found")
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
  $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_subdirectory(subscriber)
add_subdirectory(publisher)

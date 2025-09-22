#ifndef MQTT_PUBLISHER_HPP__
#define MQTT_PUBLISHER_HPP__
#include <filesystem>

#include "mqttcommon.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace MqttCpp {
  class Publisher {
   public:
    Publisher() = delete;
    explicit Publisher(const ConnectionConfig &config) {}
  };
};  // namespace MqttCpp
#endif

#ifndef MQTT_COMMON_HPP__
#define MQTT_COMMON_HPP__
#include <json/reader.h>
#include <json/value.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>

namespace MqttCpp {
  namespace fs = std::filesystem;

  struct ConnectionConfig {
    std::string hostUri;
    std::string clientId;
    std::string username;
    std::string password;
  };

  struct SubscriptionConfig {
    std::string topic;
    int qos;
  };

  ConnectionConfig loadConnectionConfig(const fs::path& configFilePath) {
    auto filepath = fs::weakly_canonical(fs::absolute(configFilePath));
    if (!fs::exists(filepath))
      throw std::runtime_error(std::format("Configuration file not found at: {}", filepath.string()));
    auto fsize = fs::file_size(filepath);
    ConnectionConfig cfg{};

    std::unique_ptr<uint8_t[]> inbuffer;
    {
      std::ifstream infile;
      infile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      try {
        infile.open(filepath.c_str(), std::ios::in | std::ifstream::binary);
      } catch (const std::ifstream::failure& e) {
        throw std::runtime_error(std::format("Can't open input file {}: {} ({}: {})", filepath.string(),
                                             e.what(), e.code().value(), e.code().message()));
      }
      try {
        inbuffer = std::make_unique_for_overwrite<uint8_t[]>(fsize);
      } catch (const std::bad_alloc& e) {
        throw;
      }
      auto buf = std::bit_cast<char*>(inbuffer.get());
      infile.read(buf, fsize);

      JSONCPP_STRING err;
      Json::Value root;
      Json::CharReaderBuilder builder;
      const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
      if (!reader->parse(buf, buf + fsize, &root, &err)) {
        throw std::runtime_error(std::format("Congif file parse error: {}", err));
      }

      cfg.hostUri = root["hosturi"].asString();
      cfg.username = root["username"].asString();
      cfg.password = root["password"].asString();

      if (root.isMember("clientId")) {
        cfg.clientId = root["clientid"].asString();
      }
    }

    return cfg;
  }
};  // namespace MqttCpp
#endif

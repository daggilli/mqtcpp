#ifndef MQTT_SUBSCRIBER_HPP__
#define MQTT_SUBSCRIBER_HPP__
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <mutex>
#include <string>

#include "mqtt/async_client.h"
#include "mqttcommon.hpp"

namespace MqttCpp {
  static const std::string STATUS{"status"};
  static const std::string TOPIC{"testtopic"};
  constexpr int QOS{1};

  using namespace std::chrono_literals;

  namespace fs = std::filesystem;
  static inline std::atomic_bool running{true};
  inline std::once_flag sigSetup;

  class Subscriber {
   public:
    Subscriber() = delete;
    Subscriber(const fs::path &configFilePath) {
      connCfg = loadConnectionConfig(configFilePath);
      std::println("H {}\nC {}\nU {}\nP {}", connCfg.hostUri, connCfg.clientId, connCfg.username,
                   connCfg.password);
      Subscriber::setSignals();
      client = std::make_shared<mqtt::async_client>(connCfg.hostUri, connCfg.clientId);
      client->set_message_callback([&](mqtt::const_message_ptr msg) {
        std::println("TOPIC: {} | MSG: {}", msg->get_topic(), msg->to_string());
      });
      client->set_connected_handler([&](const std::string &) {
        std::println("+++ Connected +++");
        client->subscribe(TOPIC, QOS);
        std::println("Waiting...");

        auto alive = mqtt::make_message(STATUS, "ALIVE");
        alive->set_qos(1);
        alive->set_retained(true);
        client->publish(alive);
      });

      client->set_connection_lost_handler([&](const std::string &cause) {
        std::println("--- connection lost: {} ---", (cause.empty() ? "<unknown>" : cause));
      });

      will = mqtt::message(STATUS, "DEAD", 1, true);
      auto connectionOptions = mqtt::connect_options_builder::v3()
                                   .user_name(connCfg.username)
                                   .password(connCfg.password)
                                   .keep_alive_interval(60s)
                                   .clean_session(false)
                                   .automatic_reconnect()
                                   .will(std::move(will))
                                   .finalize();

      try {
        client->start_consuming();

        try {
          client->connect(connectionOptions)->wait();
        } catch (const mqtt::exception &e) {
          std::cerr << "Initial connect failed: " << e.what() << "\n";
          exit(ECONNREFUSED);
        }

        while (running) std::this_thread::sleep_for(500ms);

        client->disconnect()->wait();
      } catch (const mqtt::exception &e) {
        std::println("Connection failed: {}", e.what());
        exit(ECONNREFUSED);
      }
    }

   private:
    ConnectionConfig connCfg;
    mqtt::async_client_ptr client;
    mqtt::message will;

    static inline void setSignals() {
      std::println("setSignals");
      std::call_once(sigSetup, [] {
        struct sigaction sa {};
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = +[](int) { running.store(false, std::memory_order_relaxed); };
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
      });
    }
  };
};  // namespace MqttCpp
#endif

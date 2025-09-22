#ifndef MQTT_SUBSCRIBER_HPP__
#define MQTT_SUBSCRIBER_HPP__
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "mqtt/async_client.h"
#include "mqttcommon.hpp"

namespace MqttCpp {
  using MessageHandler = std::function<void(std::string &, std::string &, std::string &)>;
  using TopicRouter = std::tuple<std::string, MessageHandler>;

  static const std::string STATUS{"status"};
  constexpr int QOS{1};

  using namespace std::chrono_literals;

  namespace fs = std::filesystem;
  static inline std::atomic_bool running{false};
  inline std::once_flag sigSetup;
  inline std::once_flag sigRestore;

  class Subscriber {
   public:
    Subscriber() = delete;
    Subscriber(const fs::path &configFilePath) {
      connCfg = loadConnectionConfig(configFilePath);
      Subscriber::setSignals();
      client = std::make_shared<mqtt::async_client>(connCfg.hostUri, connCfg.clientId,
                                                    mqtt::create_options(MQTTVERSION_5));
    }
    void start() {
      auto lock = std::scoped_lock(mx);
      running.store(true, std::memory_order_seq_cst);
      std::println("JNBLE {}", runningThread.joinable());
      if (runningThread.joinable()) return;
      runningThread = std::jthread(&Subscriber::run, this);
    }
    void stop() {
      std::jthread tmpThread;
      {
        auto lock = std::scoped_lock(mx);
        if (!runningThread.joinable()) return;
        runningThread.request_stop();
        tmpThread = std::move(runningThread);
      }
      tmpThread.join();
    }
    void addSubscription(const std::string &topic, MessageHandler handler) {
      auto lock = std::scoped_lock(mx);
      if (runningThread.joinable()) return;

      topicHandlerVec.emplace_back(topic, handler);
    }

   private:
    void run(std::stop_token stopToken) {
      client->set_message_callback([this](mqtt::const_message_ptr msg) {
        MessageHandler handler;
        auto props = msg->get_properties();
        auto topic = msg->get_topic();
        std::string subscription;

        for (const auto &p : props) {
          if (p.type() == mqtt::property::SUBSCRIPTION_IDENTIFIER) {
            auto subId = mqtt::get<uint32_t>(p);
            auto topicRouter = subscriptionMap.at(subId);
            subscription = std::get<0>(topicRouter);
            handler = std::get<1>(topicRouter);
          }
        }
        auto message = msg->to_string();
        std::println("CB [{}] | TOPIC: {} | MSG: {}", connCfg.clientId, msg->get_topic(), message);
        (handler)(topic, subscription, message);
      });

      client->set_connected_handler([this](const std::string &) {
        std::println("+++ Connected +++");

        uint32_t subId{1};
        for (TopicRouter &topic : topicHandlerVec) {
          mqtt::properties props{{mqtt::property::SUBSCRIPTION_IDENTIFIER, subId}};
          subscriptionMap.emplace(subId++, topic);
          auto subToken = client->subscribe(std::get<0>(topic), QOS, mqtt::subscribe_options{}, props);
        }
        std::println("Waiting...");

        auto alive = mqtt::make_message(STATUS, "ALIVE");
        alive->set_qos(1);
        alive->set_retained(true);
        client->publish(alive);
      });

      client->set_connection_lost_handler([this](const std::string &cause) {
        std::println("--- connection lost: {} ---", (cause.empty() ? "<unknown>" : cause));
      });

      will = mqtt::message(STATUS, "DEAD", 1, true);
      auto connectionOptions = mqtt::connect_options_builder::v5()
                                   .user_name(connCfg.username)
                                   .password(connCfg.password)
                                   .keep_alive_interval(60s)
                                   .clean_session(false)
                                   .automatic_reconnect()
                                   .will(std::move(will))
                                   .finalize();

      try {
        try {
          client->connect(connectionOptions)->wait();
        } catch (const mqtt::exception &e) {
          std::cerr << "Initial connect failed: " << e.what() << "\n";
          exit(ECONNREFUSED);
        }

        while (!stopToken.stop_requested() && MqttCpp::running.load(std::memory_order_relaxed)) {
          std::this_thread::sleep_for(100ms);
        }
        if (!running) {  // this is because of a signal and we are exiting
          std::println(stderr, "\n{} STOPPED", connCfg.clientId);
          restoreSignals();
        }
        client->disconnect()->wait();
      } catch (const mqtt::exception &e) {
        std::println("Connection failed: {}", e.what());
        exit(ECONNREFUSED);
      }
    }

    ConnectionConfig connCfg;
    mqtt::async_client_ptr client;
    mqtt::message will;
    std::jthread runningThread;
    mutable std::mutex mx;
    std::vector<TopicRouter> topicHandlerVec;
    std::unordered_map<uint32_t, TopicRouter> subscriptionMap;

    static inline void setSignals() {
      std::call_once(sigSetup, [] {
        struct sigaction sa {};
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = +[](int) { running.store(false, std::memory_order_relaxed); };
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
      });
    }

    static inline void restoreSignals() {
      std::call_once(sigRestore, [] {
        struct sigaction sigAct;

        sigaction(SIGINT, nullptr, &sigAct);
        sigAct.sa_handler = SIG_DFL;
        sigaction(SIGINT, &sigAct, nullptr);
        sigaction(SIGTERM, &sigAct, nullptr);
      });
    }
  };
};  // namespace MqttCpp
#endif

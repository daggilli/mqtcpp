#ifndef MQTT_PUBLISHER_HPP__
#define MQTT_PUBLISHER_HPP__
#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include "mqtt/async_client.h"
#include "mqttcommon.hpp"

namespace MqttCpp {
  using Message = std::pair<std::string, std::string>;

  static const std::string STATUS{"status"};
  constexpr int QOS{1};
  constexpr int MAXBUFMSGS{2048};

  using namespace std::chrono_literals;

  namespace fs = std::filesystem;
  static inline std::atomic_bool running{false};
  inline std::once_flag sigSetup;
  inline std::once_flag sigRestore;

  class Publisher {
   public:
    Publisher() = delete;
    Publisher(const fs::path &configFilePath) : instanceRunning{false} {
      Publisher::instances++;
      connCfg = loadConnectionConfig(configFilePath);
      Publisher::setSignals();
      auto createOpts = mqtt::create_options_builder()
                            .mqtt_version(MQTTVERSION_5)
                            .server_uri(connCfg.hostUri)
                            .send_while_disconnected(true, true)
                            .max_buffered_messages(MAXBUFMSGS)
                            .delete_oldest_messages()
                            .finalize();

      client = std::make_shared<mqtt::async_client>(createOpts);
    }
    ~Publisher() {
      int idx = std::atomic_fetch_sub_explicit(&Publisher::instances, 1, std::memory_order_seq_cst);
      if (idx == 1) {
        Publisher::restoreSignals();
      }
    }
    void start() {
      if (instanceRunning) return;
      running.store(true, std::memory_order_seq_cst);
      instanceRunning = true;
      runningThread = std::jthread(&Publisher::run, this);
    }
    void stop() {
      if (!instanceRunning) return;
      if (runningThread.joinable()) runningThread.request_stop();
    }
    void enqueue(const std::string &topic, const std::string &message) {
      if (!MqttCpp::running.load(std::memory_order_seq_cst)) return;
      auto lock = std::scoped_lock(mx);
      messageQueue.emplace(std::move(topic), std::move(message));
      queueCondition.notify_one();
    }

   private:
    ConnectionConfig connCfg;
    mqtt::async_client_ptr client;
    mqtt::message will;
    std::jthread runningThread;
    bool instanceRunning;
    std::queue<Message> messageQueue;
    mutable std::mutex mx;
    std::condition_variable queueCondition;
    static inline std::atomic_uint16_t instances{0};

    void run(std::stop_token stopToken) {
      client->set_connected_handler([this](const std::string &) {
        std::println("+++ Connected +++");
        auto alive = mqtt::make_message(STATUS, "ALIVE");
        alive->set_qos(1);
        alive->set_retained(true);
        client->publish(alive);
      });

      client->set_connection_lost_handler([this](const std::string &cause) {
        std::println("--- connection lost: {} ---", (cause.empty() ? "<unknown>" : cause));
      });

      auto will = mqtt::message(STATUS, "DEAD", 1, true);
      auto connectionOptions = mqtt::connect_options_builder::v5()
                                   .user_name(connCfg.username)
                                   .password(connCfg.password)
                                   .keep_alive_interval(60s)
                                   .clean_session()
                                   .automatic_reconnect(1s, 10s)
                                   .will(std::move(will))
                                   .finalize();

      try {
        try {
          client->connect(connectionOptions)->wait();
        } catch (const mqtt::exception &e) {
          std::cerr << "Initial connect failed: " << e.what() << "\n";
          exit(ECONNREFUSED);
        }

        Message message;
        while (!stopToken.stop_requested() && instanceRunning &&
               MqttCpp::running.load(std::memory_order_relaxed)) {
          if (dequeue(message)) {
            auto [topicStr, messageStr] = message;

            auto topic = mqtt::topic(*client, topicStr, QOS);
            topic.publish(messageStr);
          }
        }
        if (!running) {  // this is because of a signal and we are exiting
          instanceRunning = false;
          std::println(stderr, "\n{} STOPPED", connCfg.clientId);
          Publisher::restoreSignals();
        }
        client->disconnect()->wait();
      } catch (const mqtt::exception &e) {
        std::println("Connection failed: {}", e.what());
        exit(ECONNREFUSED);
      }
    }

    bool dequeue(Message &message) {
      auto lock = std::unique_lock(mx);
      auto hasItem = queueCondition.wait_for(lock, 500ms, [this] { return !messageQueue.empty(); });
      if (hasItem) {
        message = std::move(messageQueue.front());
        messageQueue.pop();
      }
      return hasItem;
    }
    bool empty() const {
      auto lock = std::scoped_lock(mx);
      return messageQueue.empty();
    }

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

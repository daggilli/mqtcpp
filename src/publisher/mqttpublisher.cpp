#include "mqttpublisher.hpp"

#include <format>
#include <print>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "mqttcommon.hpp"
using namespace std::chrono_literals;

void async_publish(std::stop_token tok, MqttCpp::Publisher &pub) {
  auto ct{0u};
  while (!tok.stop_requested()) {
    auto message = std::format("from thread {}", ct++);
    std::println("FROM THR PUBLISH |{}|", message);

    pub.enqueue("testtopic", message);
    std::this_thread::sleep_for(300ms);
  }
}

int main() {
  std::vector<std::string> topics{"testtopic",
                                  "anothertopic",
                                  "callable",
                                  "lambda",
                                  "devices/microwave",
                                  "devices/fridge",
                                  "devices/fridge/dooropen",
                                  "nosubscriber"};
  MqttCpp::Publisher publisher{"config/publishercfg.json"};
  MqttCpp::Publisher publisher2{"config/publishercfg.json"};

  // only need this if autostart is falsy in config file
  // publisher.start();

  auto pubthread = std::jthread(async_publish, std::ref(publisher));

  std::mt19937 engine{std::random_device{}()};

  auto ct{0u};
  while (ct++ < 100) {
    auto message = std::format("Message {}", ct);
    auto topic = topics[std::uniform_int_distribution<unsigned long>{0, topics.size() - 1}(engine)];
    publisher.enqueue(topic.c_str(), message);
    std::println("PUBLISH |{}| |{}|", topic, message);
    std::this_thread::sleep_for(80ms);
  }

  // std::this_thread::sleep_for(3000ms);
  return 0;
}
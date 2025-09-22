#include "mqttpublisher.hpp"

#include <format>
#include <print>
#include <thread>

#include "mqttcommon.hpp"
using namespace std::chrono_literals;

void async_publish(std::stop_token tok, MqttCpp::Publisher &pub) {
  auto ct{0u};
  while (!tok.stop_requested()) {
    auto message = std::format("from thread {}", ct++);
    std::println("FROM THR PUBLISH |{}|", message);

    pub.enqueue("testtopic", message);
    std::this_thread::sleep_for(1000ms);
  }
}

int main() {
  MqttCpp::Publisher publisher{"config/publishercfg.json"};
  MqttCpp::Publisher publisher2{"config/publishercfg.json"};

  // only need this if autostart is falsy in config file
  // publisher.start();

  auto pubthread = std::jthread(async_publish, std::ref(publisher));

  auto ct{0u};
  while (ct++ < 100) {
    auto message = std::format("Message {}", ct);
    std::println("PUBLISH |{}|", message);
    publisher.enqueue("testtopic", message);
    std::this_thread::sleep_for(80ms);
  }

  // std::this_thread::sleep_for(3000ms);
  return 0;
}
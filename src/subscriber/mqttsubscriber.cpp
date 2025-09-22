#include "mqttsubscriber.hpp"

#include <thread>

#include "mqttcommon.hpp"

int main() {
  MqttCpp::Subscriber subscriber{"config/subscribercfg.json"};
  // subscriber.run();
  // MqttCpp::Subscriber subscriber2{"config/subscribercfg2.json"};
  // std::jthread(&MqttCpp::Subscriber::run, subscriber);
  // std::jthread(&MqttCpp::Subscriber::run, subscriber2);
  return 0;
}
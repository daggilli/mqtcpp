#include "mqttsubscriber.hpp"

#include <chrono>
#include <print>
#include <string>
#include <thread>

#include "mqttcommon.hpp"
using namespace std::chrono_literals;

class MessageHandler {
 public:
  MessageHandler(const std::string &name) : name_{name} {}
  void operator()(std::string &topic, std::string &subscription, std::string &message) {
    std::println("CALLABLE MSGHNDLR [{}] || TOPIC |{}| || SUBSCRIPTION: |{}| || MSG: |{}|", name_, topic,
                 subscription, message);
  }
  const std::string &name() const { return name_; }

 private:
  std::string name_;
};

void handler(std::string &topic, std::string &subscription, std::string &message);

int main() {
  MqttCpp::Subscriber subscriber{"config/subscribercfg.json"};
  MqttCpp::Subscriber subscriber2{"config/subscribercfg2.json"};

  // subscriptions must be added before calling start()

  subscriber.addSubscription("testtopic", handler);
  subscriber.addSubscription("devices/fridge/temp", handler);

  MessageHandler mh("callable object");

  subscriber.addSubscription("callable", std::ref(mh));
  subscriber2.addSubscription("testtopic", handler);
  subscriber2.addSubscription("anothertopic", handler);
  subscriber2.addSubscription("devices/fridge/#", handler);
  subscriber2.addSubscription("devices/#", handler);
  subscriber2.addSubscription("lambda", [](std::string &topic, std::string &subscription,
                                           std::string &message) {
    std::println("LAMBDA || TOPIC [{}] || SUBSCRIPTION: |{}| || MESSAGE: |{}|", topic, subscription, message);
  });

  subscriber.start();
  subscriber2.start();

  while (true) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}

void handler(std::string &topic, std::string &subscription, std::string &message) {
  std::println("FREE FUNC || TOPIC |{}| || SUBSCRIPTION |{}| || MESSAGE |{}|", topic, subscription, message);
}

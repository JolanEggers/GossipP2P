#include "GossipNode.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    GossipNode node("192.168.178.126", 5000);
    node.add_known_node("192.168.178.126", 5001);

    node.subscribe("Temperature", [&node](const std::string& topic, const std::string& content) {
        std::cout << "Received [" << topic << "]: " << content << std::endl;
        node.publish("Humidity", "Humidity is" +content+"%");// std::to_string(std::stoi(content))
    });

    std::cout << "Node is running... Waiting for messages." << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}

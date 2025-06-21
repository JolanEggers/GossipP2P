#include "GossipNode.h"
#include <chrono>
#include <thread>

int main() {
    GossipNode node("192.168.178.126", 5001); // This node is the publisher

    int i = 0;
    while (true) {
        node.publish("Temperature", "Temperature is " + std::to_string(i) + "°C");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++i;
    }

    return 0;
}

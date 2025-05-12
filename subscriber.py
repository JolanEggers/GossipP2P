from gossip_node import GossipNode
import time
import socket


def temperature(topic, content):
    print(f"Received {topic} data: {content}")


def humidity(topic, content):
    print(f"Received {topic} data: {content}")


if __name__ == "__main__":
    gossip_node = GossipNode(host='auto', port=5001)
    gossip_node.subscribe("Temperature", temperature)

    while True:
        time.sleep(1)

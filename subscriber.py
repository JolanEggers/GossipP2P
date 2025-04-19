from gossip_node import GossipNode
import time


def temperature(topic, content):
    print(f"Received {topic} data: {content}")


def humidity(topic, content):
    print(f"Received {topic} data: {content}")


if __name__ == "__main__":
    gossip_node = GossipNode(host="192.168.178.67",port=5001)

    # Subscribe to topics
    gossip_node.subscribe("Temperature", temperature)
    #gossip_node.subscribe("Humidity", humidity)

    # Add known nodes with IP and Port, no topics for this example
    gossip_node.add_known_node("192.168.178.67", 5000)

    i = 0
    while True:
        time.sleep(1)

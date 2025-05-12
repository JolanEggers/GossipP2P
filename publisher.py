from gossip_node import GossipNode
import time

if __name__ == "__main__":
    gossip_node = GossipNode(host="auto",port=5000)
    gossip_node.add_known_node("192.168.152.204", 5001)
    i = 0
    while True:
        gossip_node.publish("Temperature", f"Temperature is {i}°C")
        #print(gossip_node.get_info())
        i += 1
        time.sleep(0.01)

from gossip_node import GossipNode
import time

if __name__ == "__main__":
    gossip_node = GossipNode(host="127.0.0.1",port=5000)
    i = 0
    while True:
        gossip_node.publish("Temperature", f"Temperature is {i}°C")
        #print(gossip_node.get_info())
        i += 1
        time.sleep(0.01)

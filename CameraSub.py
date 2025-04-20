import time
import cv2
import base64
import numpy as np
from gossip_node import GossipNode

img_base64=""
flag = False
def handle_camera_data(topic, content):
    global img_base64
    global flag
    img_base64 = content
    flag = True

def capture_and_display():
    global flag
    while True:
        while not flag:
            time.sleep(0.001)
        flag = False
        img_decoded_bytes = base64.b64decode(img_base64)
        np_array = np.frombuffer(img_decoded_bytes, dtype=np.uint8)
        img = cv2.imdecode(np_array, cv2.IMREAD_COLOR)

        if img is not None:
            cv2.imshow("Camera Feed", img)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    gossip_node = GossipNode(host="127.0.0.1", port=5001)
    gossip_node.add_known_node("127.0.0.1", 5000)
    gossip_node.subscribe("CameraData", handle_camera_data)
    capture_and_display()

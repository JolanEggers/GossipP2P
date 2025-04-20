import cv2
import base64
from gossip_node import GossipNode


def capture_and_send_image(gossip_node, topic_name):
    cap = cv2.VideoCapture(0)
    while True:
        ret, frame = cap.read()
        if ret:
            ret, encoded_img = cv2.imencode('.jpg', frame)
            if ret:
                img_bytes = encoded_img.tobytes()
                base64_data = base64.b64encode(img_bytes).decode('utf-8')
                gossip_node.publish(topic_name, base64_data)


if __name__ == "__main__":
    gossip_node = GossipNode(host="127.0.0.1", port=5000)
    capture_and_send_image(gossip_node, "CameraData")

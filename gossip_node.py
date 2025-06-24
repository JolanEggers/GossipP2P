import json
import socket
import threading
import time
import random


class GossipNode:
    def __init__(self, host='127.0.0.1', port=5000):
        self.host = self.get_local_ip() if host == 'auto' else host
        self.port = port

        self.connection_pool = {}  # (ip, port): socket
        self.connection_lock = threading.Lock()

        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        self.bind_with_retry(self.host, self.port)

        self.server_socket.listen(5)

        self.info = {
            "self": {
                "IP": self.host,
                "port": self.port,
                "subscribed_topics": []
            },
            "known_nodes": {}  # Changed to dict: {(ip, port): {"subscribed_topics": []}}
        }

        self.subscriptions = {}

        self.start_server_thread()

        threading.Thread(target=self.update_known_nodes_periodically, daemon=True).start()

    def bind_with_retry(self, host, port):
        while True:
            try:
                self.server_socket.bind((host, port))
                print(f"Successfully bound to {host}:{port}")
                break
            except OSError as e:
                print(f"Failed to bind to {host}:{port} — {e}. Retrying in 5 seconds...")
                time.sleep(5)

    
    def get_local_ip(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # This IP doesn't need to be reachable
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
        except Exception:
            ip = "127.0.0.1"
        finally:
            s.close()
        return ip

    def start_server_thread(self):
        server_thread = threading.Thread(target=self.start)
        server_thread.daemon = True
        server_thread.start()

    def add_known_node(self, node_ip, node_port, subscribed_topics=None):
        if subscribed_topics is None:
            subscribed_topics = []

        if node_ip == self.host and node_port == self.port:
            return

        key = (node_ip, node_port)
        if key in self.info["known_nodes"]:
            # Update existing topics
            existing_topics = self.info["known_nodes"][key]["subscribed_topics"]
            for topic in subscribed_topics:
                if topic not in existing_topics:
                    existing_topics.append(topic)
        else:
            self.info["known_nodes"][key] = {
                "IP": node_ip,
                "port": node_port,
                "subscribed_topics": subscribed_topics
            }

    def get_info(self):
        info_json = json.dumps(self.info, indent=4)
        return info_json

    def subscribe(self, topic_name, callback):
        if topic_name not in self.subscriptions:
            self.subscriptions[topic_name] = []
        self.subscriptions[topic_name].append(callback)

        if topic_name not in self.info["self"]["subscribed_topics"]:
            self.info["self"]["subscribed_topics"].append(topic_name)

    def remove_node(self, node_ip, node_port):
        """Remove a node entirely from the known_nodes dict."""
        key = (node_ip, node_port)
        self.info["known_nodes"].pop(key, None)  # Safe removal, no error if key doesn't exist

    def send_data_to_node(self, node_ip, node_port, topic_name, content):
        key = (node_ip, node_port)

        try:
            with self.connection_lock:
                if key not in self.connection_pool:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(5)
                    sock.connect(key)
                    self.connection_pool[key] = sock
                else:
                    sock = self.connection_pool[key]

            message = f"POST /{node_ip}:{node_port}/{topic_name} HTTP/1.1\r\nContent-Type: text/plain\r\n\r\n{content}END238973"
            sock.sendall(message.encode('utf-8'))

        except (socket.error, socket.timeout) as e:
            print(f"Connection error with {node_ip}:{node_port} — {e}")
            self.close_connection(node_ip, node_port)
            self.remove_node(node_ip, node_port)

    def close_connection(self, node_ip, node_port):
        key = (node_ip, node_port)
        with self.connection_lock:
            sock = self.connection_pool.pop(key, None)
            if sock:
                try:
                    sock.shutdown(socket.SHUT_RDWR)
                except:
                    pass
                sock.close()

    def close_all_connections(self): # must be called manually
        with self.connection_lock:
            for sock in self.connection_pool.values():
                try:
                    sock.shutdown(socket.SHUT_RDWR)
                except:
                    pass
                sock.close()
            self.connection_pool.clear()


    def publish(self, topic_name, content):
        for (node_ip, node_port), node_data in self.info["known_nodes"].items():
            if topic_name in node_data["subscribed_topics"]:
                threading.Thread(target=self.send_data_to_node, args=(node_ip, node_port, topic_name, content)).start()

        if topic_name in self.subscriptions:
            for callback in self.subscriptions[topic_name]:
                callback(topic_name, content)

    def handle_client(self, client_socket):
        try:
            buffer = b""
            while True:
                chunk = client_socket.recv(1024)
                if not chunk:
                    break  # client disconnected
                buffer += chunk

                while b"END238973" in buffer:
                    # Split the message
                    message, buffer = buffer.split(b"END238973", 1)
                    request = message.decode('utf-8')

                    if request.startswith("GET /info"):
                        request_json = json.loads(request.split("\r\n\r\n")[1])
                        node_ip = request_json.get("self", {}).get("IP")
                        node_port = request_json.get("self", {}).get("port")
                        subscribed_topics = request_json.get("self", {}).get("subscribed_topics", [])

                        self.add_known_node(node_ip, node_port, subscribed_topics)
                        response = self.get_info()

                        client_socket.sendall(response.encode('utf-8'))

                    else:
                        try:
                            topic_info = request.split(' ')[1].strip('/')
                            ip_port, topic_name = topic_info.split('/')

                            if topic_name in self.subscriptions:
                                content = request.split("\r\n\r\n")[1]
                                for callback in self.subscriptions[topic_name]:
                                    callback(topic_name, content)

                            response = f"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                        except Exception as e:
                            response = "HTTP/1.1 404 Not Found\r\n\r\n"

                        client_socket.sendall(response.encode('utf-8'))

        except Exception as e:
            print(f"Error handling client: {e}")
        finally:
            client_socket.close()


    def start(self):
        while True:
            client_socket, client_address = self.server_socket.accept()
            client_handler = threading.Thread(target=self.handle_client, args=(client_socket,))
            client_handler.start()

    def update_known_nodes_periodically(self):
        #thread_id = threading.get_ident()
        while True:
            #with open(f"/tmp/thread_{threading.get_ident()}.alive", "w") as f:
            #    f.write(str(time.time()))
            if self.info["known_nodes"]:
                random_key = random.choice(list(self.info["known_nodes"].keys()))
                node_ip, node_port = random_key
                self.query_node_for_info(node_ip, node_port)
            time.sleep(1)

    def query_node_for_info(self, node_ip, node_port):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((node_ip, node_port))

                request = f"GET /info\r\n\r\n{json.dumps(self.info)}END238973"
                s.sendall(request.encode('utf-8'))

                response = s.recv(1024).decode('utf-8')

                node_info = json.loads(response)
                subscribed_topics = node_info.get("self", {}).get("subscribed_topics", [])
                known_nodes = node_info.get("known_nodes", [])

                self.add_known_node(node_ip, node_port, subscribed_topics)

                for new_node in known_nodes:
                    if isinstance(new_node, dict) and "IP" in new_node and "port" in new_node:
                        self.add_known_node(new_node["IP"], new_node["port"], new_node.get("subscribed_topics", []))

                for topic in subscribed_topics:
                    if topic not in self.info["self"]["subscribed_topics"]:
                        self.info["self"]["subscribed_topics"].append(topic)

        except Exception as e:
            print(f"Error querying {node_ip}:{node_port}/info: {e}")
            self.remove_node(node_ip, node_port)

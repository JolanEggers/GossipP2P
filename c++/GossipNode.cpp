#include "GossipNode.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h> // for gethostbyname
#include <signal.h>

using json = nlohmann::json;

GossipNode::GossipNode(const std::string& host, int port)
    : host_(host), port_(port), shutdown_flag_(false) {

    // Ignore SIGPIPE globally
    static bool signal_handled = false;
    if (!signal_handled) {
        signal(SIGPIPE, SIG_IGN);
        signal_handled = true;
    }

    info_["self"] = {
        {"IP", host_},
        {"port", port_},
        {"subscribed_topics", json::array()}
    };
    info_["known_nodes"] = json::array();

    bind_with_retry();
    server_thread_ = std::thread(&GossipNode::start_server, this);
    update_thread_ = std::thread(&GossipNode::update_known_nodes_periodically, this);
}

GossipNode::~GossipNode() {
    shutdown();
}

void GossipNode::bind_with_retry() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host_.c_str());

    while (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind, retrying in 5s...\n";
        sleep(5);
    }

    listen(server_fd_, 5);
    std::cout << "Listening on " << host_ << ":" << port_ << std::endl;
}

void GossipNode::start_server() {
    while (!shutdown_flag_.load()) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if (client_fd >= 0) {
            std::thread(&GossipNode::handle_client, this, client_fd).detach();
        } else {
            // If accept fails (e.g., socket closed), break out of loop
            if (shutdown_flag_.load()) {
                break;
            }
            // Brief sleep to avoid busy waiting on persistent errors
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void GossipNode::handle_client(int client_fd) {
    char buffer[1024];
    std::string data;
    ssize_t received;

    try {
        while ((received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
            data.append(buffer, received);

            size_t end_marker;
            while ((end_marker = data.find("END238973")) != std::string::npos) {
                std::string message = data.substr(0, end_marker);
                data.erase(0, end_marker + 9);  // Remove message + marker

                if (message.find("GET /info") == 0) {
                    std::string json_payload = message.substr(message.find("\r\n\r\n") + 4);
                    try {
                        json remote = json::parse(json_payload);
                        std::string ip = remote["self"]["IP"];
                        int port = remote["self"]["port"];
                        std::vector<std::string> topics = remote["self"]["subscribed_topics"];
                        add_known_node(ip, port, topics);
                    } catch (...) {
                        std::cerr << "Failed to parse JSON in GET /info.\n";
                    }

                    std::string response = get_info_json();
                    send(client_fd, response.c_str(), response.size(), 0);
                }
                else if (message.find("POST /") == 0) {
                    try {
                        auto start = message.find("POST /") + 6;
                        auto end = message.find(" HTTP", start);
                        std::string path = message.substr(start, end - start);

                        auto split_pos = path.find('/');
                        std::string topic = path.substr(split_pos + 1);

                        std::string body = message.substr(message.find("\r\n\r\n") + 4);

                        if (subscriptions_.count(topic)) {
                            for (auto& cb : subscriptions_[topic]) {
                                cb(topic, body);
                            }
                        }

                        std::string response = "HTTP/1.1 200 OK\r\n\r\n";
                        send(client_fd, response.c_str(), response.size(), 0);
                    } catch (...) {
                        std::cerr << "Error parsing POST message\n";
                        std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
                        send(client_fd, response.c_str(), response.size(), 0);
                    }
                }
                else {
                    std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
                    send(client_fd, response.c_str(), response.size(), 0);
                }
            }
        }
    } catch (...) {
        std::cerr << "Error in client handler.\n";
    }

    close(client_fd); // Close once disconnected
}

void GossipNode::add_known_node(const std::string& ip, int port, const std::vector<std::string>& topics) {
    for (auto& node : info_["known_nodes"]) {
        if (node["IP"] == ip && node["port"] == port) {
            for (const auto& topic : topics) {
                if (std::find(node["subscribed_topics"].begin(), node["subscribed_topics"].end(), topic) == node["subscribed_topics"].end()) {
                    node["subscribed_topics"].push_back(topic);
                }
            }
            return;
        }
    }

    json node = {
        {"IP", ip},
        {"port", port},
        {"subscribed_topics", topics}
    };
    info_["known_nodes"].push_back(node);
}

void GossipNode::add_known_node(const std::string& ip, int port) {
    add_known_node(ip, port, {});
}

void GossipNode::publish(const std::string& topic, const std::string& content) {
    for (const auto& node : info_["known_nodes"]) {
        const std::string ip = node["IP"];
        int port = node["port"];
        std::string path = ip + ":" + std::to_string(port) + "/" + topic;
        std::string message = "POST /" + path + " HTTP/1.1\r\nContent-Type: text/plain\r\n\r\n" + content + "END238973";

        std::pair<std::string, int> key = {ip, port};
        int sock;
        bool socket_is_valid = true;

        {
            std::lock_guard<std::mutex> lock(conn_mutex_);
            if (socket_pool_.count(key)) {
                sock = socket_pool_[key];
            } else {
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    std::cerr << "Socket creation failed.\n";
                    continue;
                }

                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

                if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
                    //std::cerr << "Connection failed to " << ip << ":" << port << "\n";
                    close(sock);
                    continue;
                }

                socket_pool_[key] = sock;
            }
        }

        try {
            ssize_t sent = send(sock, message.c_str(), message.size(), 0);
            if (sent != (ssize_t)message.size()) {
                throw std::runtime_error("Partial or failed send");
            }
        } catch (...) {
            std::cerr << "Send error to " << ip << ":" << port << ", cleaning up.\n";
            std::lock_guard<std::mutex> lock(conn_mutex_);
            close(sock);
            socket_pool_.erase(key);
            continue; // Don't crash — just skip this node
        }
    }

    // Local delivery if subscribed
    if (subscriptions_.count(topic)) {
        for (auto& cb : subscriptions_[topic]) {
            cb(topic, content);
        }
    }
}


void GossipNode::subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    subscriptions_[topic].push_back(callback);
    if (std::find(info_["self"]["subscribed_topics"].begin(), info_["self"]["subscribed_topics"].end(), topic) == info_["self"]["subscribed_topics"].end()) {
        info_["self"]["subscribed_topics"].push_back(topic);
    }
}

void GossipNode::update_known_nodes_periodically() {
    while (!shutdown_flag_.load()) {
        {
            std::lock_guard<std::mutex> lock(conn_mutex_);
            if (!info_["known_nodes"].empty()) {
                size_t i = rand() % info_["known_nodes"].size();
                auto node = info_["known_nodes"][i];
                std::string ip = node["IP"];
                int port = node["port"];
                query_node_for_info(ip, port);
            }
        }
        
        // Sleep in smaller chunks to respond faster to shutdown
        for (int i = 0; i < 10 && !shutdown_flag_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
void GossipNode::query_node_for_info(const std::string& ip, int port) {
    try {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return;
        }

        std::string body = info_.dump();
        std::string request = "GET /info\r\n\r\n" + body + "END238973";
        send(sock, request.c_str(), request.size(), 0);

        char buffer[4096];
        ssize_t bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            std::string response(buffer, bytes);
            auto json_start = response.find("{");
            if (json_start != std::string::npos) {
                std::string json_body = response.substr(json_start);
                json remote_info = json::parse(json_body);

                auto remote_self = remote_info["self"];
                add_known_node(remote_self["IP"], remote_self["port"], remote_self["subscribed_topics"]);

                for (const auto& node : remote_info["known_nodes"]) {
                    add_known_node(node["IP"], node["port"], node["subscribed_topics"]);
                }
            }
        }

        close(sock);
    } catch (...) {
        // Silently fail, optional logging
    }
}


std::string GossipNode::get_info_json() const {
    return info_.dump(4);
}

void GossipNode::shutdown() {
    // Set shutdown flag to stop all loops
    shutdown_flag_.store(true);
    
    // Shutdown and close server socket to interrupt accept() calls
    ::shutdown(server_fd_, SHUT_RDWR);
    close(server_fd_);
    
    // Wait for threads to finish
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    
    // Clean up socket pool
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (auto& [_, sock] : socket_pool_) {
        close(sock);
    }
    socket_pool_.clear();
}

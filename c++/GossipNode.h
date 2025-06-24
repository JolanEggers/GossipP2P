#pragma once

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <netinet/in.h>
#include "json.hpp"

class GossipNode {
public:
    GossipNode(const std::string& host = "127.0.0.1", int port = 5000);
    ~GossipNode();

    // Subscriptions
    void subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback);

    // Node registration
    void add_known_node(const std::string& ip, int port);
    void add_known_node(const std::string& ip, int port, const std::vector<std::string>& topics);

    // Publish data to all interested nodes
    void publish(const std::string& topic, const std::string& content);

    // Info for debugging
    std::string get_info_json() const;

    // Shutdown method
    void shutdown();

private:
    // Node identity
    std::string host_;
    int port_;
    int server_fd_;
    std::thread server_thread_;
    std::thread update_thread_;

    // Shutdown flag
    std::atomic<bool> shutdown_flag_;

    // JSON info structure (self & known_nodes)
    nlohmann::json info_;

    // Local topic subscriptions
    std::map<std::string, std::vector<std::function<void(const std::string&, const std::string&)>>> subscriptions_;

    // Outbound connections
    std::map<std::pair<std::string, int>, int> socket_pool_;
    std::mutex conn_mutex_;

    // Server logic
    void bind_with_retry();
    void start_server();
    void handle_client(int client_fd);
    void query_node_for_info(const std::string& ip, int port);
    void update_known_nodes_periodically();
};

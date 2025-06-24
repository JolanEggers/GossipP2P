#include "GossipNode.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cassert>
#include <vector>
#include <future>

// Simple test framework
class TestFramework {
private:
    static int test_count;
    static int passed_count;
    
public:
    static void assert_true(bool condition, const std::string& message) {
        test_count++;
        if (condition) {
            passed_count++;
            std::cout << "✓ PASS: " << message << std::endl;
        } else {
            std::cout << "✗ FAIL: " << message << std::endl;
        }
    }
    
    static void assert_equals(const std::string& expected, const std::string& actual, const std::string& message) {
        assert_true(expected == actual, message + " (expected: '" + expected + "', actual: '" + actual + "')");
    }
    
    static void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << test_count << std::endl;
        std::cout << "Passed: " << passed_count << std::endl;
        std::cout << "Failed: " << (test_count - passed_count) << std::endl;
        std::cout << "Success rate: " << (test_count > 0 ? (100.0 * passed_count / test_count) : 0) << "%" << std::endl;
    }
    
    static bool all_passed() {
        return test_count == passed_count;
    }
};

int TestFramework::test_count = 0;
int TestFramework::passed_count = 0;

// Test helper functions
void wait_for_network_ready(int milliseconds = 100) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Test 1: Basic Construction and Info
void test_basic_construction() {
    std::cout << "\n--- Test: Basic Construction ---" << std::endl;
    
    try {
        GossipNode node("127.0.0.1", 5100);
        wait_for_network_ready();
        
        std::string info = node.get_info_json();
        TestFramework::assert_true(!info.empty(), "Node info should not be empty");
        TestFramework::assert_true(info.find("127.0.0.1") != std::string::npos, "Node info should contain host IP");
        TestFramework::assert_true(info.find("5100") != std::string::npos, "Node info should contain port");
        TestFramework::assert_true(info.find("subscribed_topics") != std::string::npos, "Node info should contain subscribed_topics");
        TestFramework::assert_true(info.find("known_nodes") != std::string::npos, "Node info should contain known_nodes");
        
    } catch (const std::exception& e) {
        TestFramework::assert_true(false, "Constructor should not throw exception: " + std::string(e.what()));
    }
}

// Test 2: Subscription Functionality
void test_subscription() {
    std::cout << "\n--- Test: Subscription ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5101);
    wait_for_network_ready();
    
    std::atomic<int> message_count(0);
    std::string received_topic;
    std::string received_content;
    
    // Subscribe to a topic
    node.subscribe("test_topic", [&](const std::string& topic, const std::string& content) {
        received_topic = topic;
        received_content = content;
        message_count++;
    });
    
    // Check subscription was added to info
    std::string info = node.get_info_json();
    TestFramework::assert_true(info.find("test_topic") != std::string::npos, "Subscribed topic should appear in node info");
    
    // Test local delivery (publish to self)
    node.publish("test_topic", "test_message");
    wait_for_network_ready();
    
    TestFramework::assert_true(message_count.load() == 1, "Should receive exactly one message");
    TestFramework::assert_equals("test_topic", received_topic, "Received topic should match");
    TestFramework::assert_equals("test_message", received_content, "Received content should match");
}

// Test 3: Multiple Subscriptions
void test_multiple_subscriptions() {
    std::cout << "\n--- Test: Multiple Subscriptions ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5102);
    wait_for_network_ready();
    
    std::atomic<int> topic1_count(0);
    std::atomic<int> topic2_count(0);
    
    node.subscribe("topic1", [&](const std::string&, const std::string&) {
        topic1_count++;
    });
    
    node.subscribe("topic2", [&](const std::string&, const std::string&) {
        topic2_count++;
    });
    
    // Multiple callbacks for same topic
    node.subscribe("topic1", [&](const std::string&, const std::string&) {
        topic1_count++;
    });
    
    node.publish("topic1", "message1");
    node.publish("topic2", "message2");
    node.publish("topic1", "message3");
    
    wait_for_network_ready();
    
    TestFramework::assert_true(topic1_count.load() == 4, "Topic1 should receive 4 messages (2 callbacks * 2 messages)");
    TestFramework::assert_true(topic2_count.load() == 1, "Topic2 should receive 1 message");
}

// Test 4: Node Discovery and Known Nodes
void test_node_discovery() {
    std::cout << "\n--- Test: Node Discovery ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5103);
    wait_for_network_ready();
    
    // Test adding known nodes
    node.add_known_node("127.0.0.1", 5104);
    node.add_known_node("127.0.0.1", 5105, {"topic1", "topic2"});
    
    std::string info = node.get_info_json();
    TestFramework::assert_true(info.find("5104") != std::string::npos, "Known node 5104 should be in info");
    TestFramework::assert_true(info.find("5105") != std::string::npos, "Known node 5105 should be in info");
    TestFramework::assert_true(info.find("topic1") != std::string::npos, "Known node topics should be in info");
}

// Test 5: Inter-Node Communication
void test_inter_node_communication() {
    std::cout << "\n--- Test: Inter-Node Communication ---" << std::endl;
    
    // Create two nodes
    GossipNode publisher("127.0.0.1", 5106);
    GossipNode subscriber("127.0.0.1", 5107);
    
    wait_for_network_ready(200); // Give nodes time to start
    
    std::atomic<int> message_count(0);
    std::string received_topic;
    std::string received_content;
    
    // Set up subscription on subscriber
    subscriber.subscribe("inter_node_topic", [&](const std::string& topic, const std::string& content) {
        received_topic = topic;
        received_content = content;
        message_count++;
    });
    
    // Publisher knows about subscriber
    publisher.add_known_node("127.0.0.1", 5107);
    
    wait_for_network_ready();
    
    // Publish from publisher to subscriber
    publisher.publish("inter_node_topic", "hello_from_publisher");
    
    // Wait for message propagation
    wait_for_network_ready(500);
    
    TestFramework::assert_true(message_count.load() >= 1, "Subscriber should receive message from publisher");
    if (message_count.load() > 0) {
        TestFramework::assert_equals("inter_node_topic", received_topic, "Received topic should match");
        TestFramework::assert_equals("hello_from_publisher", received_content, "Received content should match");
    }
}

// Test 6: Bidirectional Communication
void test_bidirectional_communication() {
    std::cout << "\n--- Test: Bidirectional Communication ---" << std::endl;
    
    GossipNode node1("127.0.0.1", 5108);
    GossipNode node2("127.0.0.1", 5109);
    
    wait_for_network_ready(200);
    
    std::atomic<int> node1_received(0);
    std::atomic<int> node2_received(0);
    
    // Set up mutual subscriptions
    node1.subscribe("ping", [&](const std::string&, const std::string&) {
        node1_received++;
    });
    
    node2.subscribe("pong", [&](const std::string&, const std::string&) {
        node2_received++;
    });
    
    // Mutual knowledge
    node1.add_known_node("127.0.0.1", 5109);
    node2.add_known_node("127.0.0.1", 5108);
    
    wait_for_network_ready();
    
    // Send messages in both directions
    node1.publish("pong", "from_node1");
    node2.publish("ping", "from_node2");
    
    wait_for_network_ready(500);
    
    TestFramework::assert_true(node1_received.load() >= 1, "Node1 should receive ping message");
    TestFramework::assert_true(node2_received.load() >= 1, "Node2 should receive pong message");
}

// Test 7: Stress Test - Multiple Messages
void test_multiple_messages() {
    std::cout << "\n--- Test: Multiple Messages ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5110);
    wait_for_network_ready();
    
    std::atomic<int> total_received(0);
    
    node.subscribe("stress_topic", [&](const std::string&, const std::string&) {
        total_received++;
    });
    
    const int message_count = 100;
    for (int i = 0; i < message_count; i++) {
        node.publish("stress_topic", "message_" + std::to_string(i));
    }
    
    wait_for_network_ready(200);
    
    TestFramework::assert_true(total_received.load() == message_count, 
        "Should receive all " + std::to_string(message_count) + " messages, got " + std::to_string(total_received.load()));
}

// Test 8: Edge Cases
void test_edge_cases() {
    std::cout << "\n--- Test: Edge Cases ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5111);
    wait_for_network_ready();
    
    // Test empty topic and content
    std::atomic<int> empty_received(0);
    node.subscribe("", [&](const std::string&, const std::string&) {
        empty_received++;
    });
    
    node.publish("", "");
    wait_for_network_ready();
    
    TestFramework::assert_true(empty_received.load() == 1, "Should handle empty topic and content");
    
    // Test large content
    std::string large_content(1000, 'A');
    std::atomic<int> large_received(0);
    std::string received_large_content;
    
    node.subscribe("large_topic", [&](const std::string&, const std::string& content) {
        large_received++;
        received_large_content = content;
    });
    
    node.publish("large_topic", large_content);
    wait_for_network_ready();
    
    TestFramework::assert_true(large_received.load() == 1, "Should handle large content");
    TestFramework::assert_equals(large_content, received_large_content, "Large content should be preserved");
    
    // Test publishing to non-existent topic (should not crash)
    node.publish("non_existent_topic", "test");
    wait_for_network_ready();
    
    TestFramework::assert_true(true, "Publishing to non-existent topic should not crash");
}

// Test 9: Concurrent Operations
void test_concurrent_operations() {
    std::cout << "\n--- Test: Concurrent Operations ---" << std::endl;
    
    GossipNode node("127.0.0.1", 5112);
    wait_for_network_ready();
    
    std::atomic<int> concurrent_received(0);
    
    node.subscribe("concurrent_topic", [&](const std::string&, const std::string&) {
        concurrent_received++;
    });
    
    // Launch multiple threads publishing simultaneously
    const int thread_count = 5;
    const int messages_per_thread = 20;
    std::vector<std::future<void>> futures;
    
    for (int t = 0; t < thread_count; t++) {
        futures.push_back(std::async(std::launch::async, [&node, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; i++) {
                node.publish("concurrent_topic", "thread_" + std::to_string(t) + "_msg_" + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    wait_for_network_ready(500);
    
    int expected_messages = thread_count * messages_per_thread;
    TestFramework::assert_true(concurrent_received.load() == expected_messages, 
        "Should receive all " + std::to_string(expected_messages) + " concurrent messages, got " + std::to_string(concurrent_received.load()));
}

// Main test runner
int main() {
    std::cout << "Starting GossipNode Test Suite" << std::endl;
    std::cout << "==============================" << std::endl;
    
    try {
        test_basic_construction();
        test_subscription();
        test_multiple_subscriptions();
        test_node_discovery();
        test_inter_node_communication();
        test_bidirectional_communication();
        test_multiple_messages();
        test_edge_cases();
        test_concurrent_operations();
        
    } catch (const std::exception& e) {
        std::cout << "Unexpected exception during testing: " << e.what() << std::endl;
        TestFramework::assert_true(false, "No exceptions should be thrown during testing");
    }
    
    TestFramework::print_summary();
    
    return TestFramework::all_passed() ? 0 : 1;
}
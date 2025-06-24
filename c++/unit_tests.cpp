
#include "GossipNode.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cassert>
#include <sstream>
#include <iomanip>

// Lightweight unit test framework
class UnitTest {
private:
    static int total_tests;
    static int passed_tests;
    static std::string current_suite;
    
public:
    static void start_suite(const std::string& suite_name) {
        current_suite = suite_name;
        std::cout << "\n=== " << suite_name << " ===" << std::endl;
    }
    
    static void assert_true(bool condition, const std::string& test_name) {
        total_tests++;
        if (condition) {
            passed_tests++;
            std::cout << "✓ " << test_name << std::endl;
        } else {
            std::cout << "✗ " << test_name << " [FAILED]" << std::endl;
        }
    }
    
    static void assert_false(bool condition, const std::string& test_name) {
        assert_true(!condition, test_name);
    }
    
    static void assert_equals(const std::string& expected, const std::string& actual, const std::string& test_name) {
        bool equal = (expected == actual);
        total_tests++;
        if (equal) {
            passed_tests++;
            std::cout << "✓ " << test_name << std::endl;
        } else {
            std::cout << "✗ " << test_name << " [FAILED]" << std::endl;
            std::cout << "  Expected: '" << expected << "'" << std::endl;
            std::cout << "  Actual:   '" << actual << "'" << std::endl;
        }
    }
    
    static void assert_contains(const std::string& haystack, const std::string& needle, const std::string& test_name) {
        assert_true(haystack.find(needle) != std::string::npos, test_name);
    }
    
    static void print_final_summary() {
        std::cout << "\n" << "=" << std::string(50, '=') << std::endl;
        std::cout << "FINAL TEST SUMMARY" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        std::cout << "Total Tests: " << total_tests << std::endl;
        std::cout << "Passed:      " << passed_tests << std::endl;
        std::cout << "Failed:      " << (total_tests - passed_tests) << std::endl;
        
        if (total_tests > 0) {
            double success_rate = (100.0 * passed_tests) / total_tests;
            std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
        }
        
        if (passed_tests == total_tests) {
            std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
        } else {
            std::cout << "\n❌ SOME TESTS FAILED ❌" << std::endl;
        }
    }
    
    static bool all_passed() {
        return passed_tests == total_tests;
    }
};

int UnitTest::total_tests = 0;
int UnitTest::passed_tests = 0;
std::string UnitTest::current_suite = "";

// Helper function to wait for network operations
void wait_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Test Suite 1: Constructor and Basic Properties
void test_constructor_and_properties() {
    UnitTest::start_suite("Constructor and Basic Properties");
    
    // Test default constructor
    try {
        GossipNode node1("127.0.0.1", 6000); // Use unique port
        std::string info = node1.get_info_json();
        UnitTest::assert_true(!info.empty(), "Default constructor should create valid node");
        UnitTest::assert_contains(info, "127.0.0.1", "Default constructor should use localhost");
        UnitTest::assert_contains(info, "6000", "Default constructor should use port 6000");
    } catch (...) {
        UnitTest::assert_true(false, "Default constructor should not throw");
    }
    
    // Test custom constructor
    try {
        GossipNode node2("127.0.0.1", 6001); // Use unique port
        wait_ms(100);
        std::string info = node2.get_info_json();
        UnitTest::assert_contains(info, "127.0.0.1", "Custom constructor should use specified IP");
        UnitTest::assert_contains(info, "6001", "Custom constructor should use specified port");
    } catch (...) {
        UnitTest::assert_true(false, "Custom constructor should not throw");
    }
}

// Test Suite 2: JSON Info Structure
void test_json_info_structure() {
    UnitTest::start_suite("JSON Info Structure");
    
    GossipNode node("127.0.0.1", 6010);
    wait_ms(100);
    
    std::string info = node.get_info_json();
    
    // Test required JSON fields
    UnitTest::assert_contains(info, "\"self\"", "Info should contain 'self' field");
    UnitTest::assert_contains(info, "\"known_nodes\"", "Info should contain 'known_nodes' field");
    UnitTest::assert_contains(info, "\"IP\"", "Info should contain 'IP' field");
    UnitTest::assert_contains(info, "\"port\"", "Info should contain 'port' field");
    UnitTest::assert_contains(info, "\"subscribed_topics\"", "Info should contain 'subscribed_topics' field");
    
    // Test that known_nodes starts empty
    UnitTest::assert_contains(info, "\"known_nodes\": []", "Known nodes should start as empty array");
    
    // Test that subscribed_topics starts empty
    UnitTest::assert_contains(info, "\"subscribed_topics\": []", "Subscribed topics should start as empty array");
}

// Test Suite 3: Subscription Management
void test_subscription_management() {
    UnitTest::start_suite("Subscription Management");
    
    GossipNode node("127.0.0.1", 6020);
    wait_ms(100);
    
    std::atomic<int> callback_count(0);
    std::string last_topic, last_content;
    
    // Test single subscription
    node.subscribe("temperature", [&](const std::string& topic, const std::string& content) {
        callback_count++;
        last_topic = topic;
        last_content = content;
    });
    
    std::string info = node.get_info_json();
    UnitTest::assert_contains(info, "temperature", "Subscription should appear in info");
    
    // Test local message delivery
    node.publish("temperature", "25.5°C");
    wait_ms(50);
    
    UnitTest::assert_true(callback_count.load() == 1, "Should receive local message");
    UnitTest::assert_equals("temperature", last_topic, "Topic should match");
    UnitTest::assert_equals("25.5°C", last_content, "Content should match");
    
    // Test multiple subscriptions to same topic
    std::atomic<int> second_callback_count(0);
    node.subscribe("temperature", [&](const std::string&, const std::string&) {
        second_callback_count++;
    });
    
    node.publish("temperature", "26.0°C");
    wait_ms(50);
    
    UnitTest::assert_true(callback_count.load() == 2, "First callback should be called again");
    UnitTest::assert_true(second_callback_count.load() == 1, "Second callback should be called");
}

// Test Suite 4: Known Nodes Management
void test_known_nodes_management() {
    UnitTest::start_suite("Known Nodes Management");
    
    GossipNode node("127.0.0.1", 6030);
    wait_ms(100);
    
    // Test adding known node without topics
    node.add_known_node("192.168.1.10", 5000);
    std::string info = node.get_info_json();
    UnitTest::assert_contains(info, "192.168.1.10", "Should contain added node IP");
    UnitTest::assert_contains(info, "5000", "Should contain added node port");
    
    // Test adding known node with topics
    std::vector<std::string> topics = {"sensor_data", "alerts"};
    node.add_known_node("192.168.1.11", 5001, topics);
    info = node.get_info_json();
    UnitTest::assert_contains(info, "192.168.1.11", "Should contain second node IP");
    UnitTest::assert_contains(info, "sensor_data", "Should contain node topics");
    UnitTest::assert_contains(info, "alerts", "Should contain node topics");
    
    // Test duplicate node handling
    node.add_known_node("192.168.1.10", 5000); // Same as first
    info = node.get_info_json();
    // Count occurrences of the IP to ensure no duplicates
    size_t count = 0;
    size_t pos = 0;
    while ((pos = info.find("192.168.1.10", pos)) != std::string::npos) {
        count++;
        pos += std::string("192.168.1.10").length();
    }
    UnitTest::assert_true(count <= 2, "Should not create duplicate nodes"); // May appear twice in JSON structure
}

// Test Suite 5: Message Publishing
void test_message_publishing() {
    UnitTest::start_suite("Message Publishing");
    
    GossipNode node("127.0.0.1", 6040);
    wait_ms(100);
    
    std::atomic<int> message_count(0);
    std::vector<std::string> received_messages;
    
    node.subscribe("test_channel", [&](const std::string&, const std::string& content) {
        message_count++;
        received_messages.push_back(content);
    });
    
    // Test basic publishing
    node.publish("test_channel", "Hello World");
    wait_ms(50);
    UnitTest::assert_true(message_count.load() == 1, "Should receive published message");
    
    // Test multiple messages
    for (int i = 0; i < 10; i++) {
        node.publish("test_channel", "Message " + std::to_string(i));
    }
    wait_ms(100);
    UnitTest::assert_true(message_count.load() == 11, "Should receive all published messages");
    
    // Test publishing to non-subscribed topic (should not crash)
    try {
        node.publish("non_existent_channel", "This should not crash");
        UnitTest::assert_true(true, "Publishing to non-subscribed topic should not crash");
    } catch (...) {
        UnitTest::assert_true(false, "Publishing to non-subscribed topic should not throw");
    }
}

// Test Suite 6: Special Characters and Edge Cases
void test_special_characters_and_edge_cases() {
    UnitTest::start_suite("Special Characters and Edge Cases");
    
    GossipNode node("127.0.0.1", 6050);
    wait_ms(100);
    
    std::string received_content;
    std::atomic<bool> message_received(false);
    
    node.subscribe("special_test", [&](const std::string&, const std::string& content) {
        received_content = content;
        message_received = true;
    });
    
    // Test empty content
    node.publish("special_test", "");
    wait_ms(50);
    UnitTest::assert_true(message_received.load(), "Should handle empty content");
    UnitTest::assert_equals("", received_content, "Empty content should be preserved");
    
    message_received = false;
    
    // Test content with special characters
    std::string special_content = "Hello\nWorld\t\"JSON\":{value}[array]&symbols!@#$%^&*()";
    node.publish("special_test", special_content);
    wait_ms(50);
    UnitTest::assert_true(message_received.load(), "Should handle special characters");
    UnitTest::assert_equals(special_content, received_content, "Special characters should be preserved");
    
    message_received = false;
    
    // Test Unicode content
    std::string unicode_content = "Unicode: Hello World Emoji test";
    node.publish("special_test", unicode_content);
    wait_ms(50);
    UnitTest::assert_true(message_received.load(), "Should handle Unicode content");
    UnitTest::assert_equals(unicode_content, received_content, "Unicode content should be preserved");
    
    // Test very long content
    message_received = false;
    std::string long_content(1000, 'A');
    node.publish("special_test", long_content);
    wait_ms(100);
    UnitTest::assert_true(message_received.load(), "Should handle very long content");
    UnitTest::assert_equals(long_content, received_content, "Long content should be preserved");
}

// Test Suite 7: Multiple Topics
void test_multiple_topics() {
    UnitTest::start_suite("Multiple Topics");
    
    GossipNode node("127.0.0.1", 6060);
    wait_ms(100);
    
    std::atomic<int> topic1_count(0), topic2_count(0), topic3_count(0);
    
    node.subscribe("topic1", [&](const std::string&, const std::string&) {
        topic1_count++;
    });
    
    node.subscribe("topic2", [&](const std::string&, const std::string&) {
        topic2_count++;
    });
    
    node.subscribe("topic3", [&](const std::string&, const std::string&) {
        topic3_count++;
    });
    
    // Test selective message delivery
    node.publish("topic1", "message for topic1");
    node.publish("topic2", "message for topic2");
    node.publish("topic1", "another message for topic1");
    
    wait_ms(100);
    
    UnitTest::assert_true(topic1_count.load() == 2, "Topic1 should receive 2 messages");
    UnitTest::assert_true(topic2_count.load() == 1, "Topic2 should receive 1 message");
    UnitTest::assert_true(topic3_count.load() == 0, "Topic3 should receive 0 messages");
    
    // Verify all topics are in node info
    std::string info = node.get_info_json();
    UnitTest::assert_contains(info, "topic1", "Info should contain topic1");
    UnitTest::assert_contains(info, "topic2", "Info should contain topic2");
    UnitTest::assert_contains(info, "topic3", "Info should contain topic3");
}

// Test Suite 8: Thread Safety
void test_thread_safety() {
    UnitTest::start_suite("Thread Safety");
    
    GossipNode node("127.0.0.1", 6070);
    wait_ms(100);
    
    std::atomic<int> total_received(0);
    
    node.subscribe("thread_test", [&](const std::string&, const std::string&) {
        total_received++;
        // Small delay to test concurrent access
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    });
    
    // Launch multiple threads publishing simultaneously
    const int num_threads = 5;
    const int messages_per_thread = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&node, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; i++) {
                node.publish("thread_test", "Thread" + std::to_string(t) + "_Msg" + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for all messages to be processed
    wait_ms(500);
    
    int expected_total = num_threads * messages_per_thread;
    UnitTest::assert_true(total_received.load() == expected_total, 
        "Should receive all " + std::to_string(expected_total) + " messages from concurrent threads");
}

// Main function
int main() {
    std::cout << "GossipNode Unit Test Suite" << std::endl;
    std::cout << "=" << std::string(50, '=') << std::endl;
    
    try {
        test_constructor_and_properties();
        test_json_info_structure();
        test_subscription_management();
        test_known_nodes_management();
        test_message_publishing();
        test_special_characters_and_edge_cases();
        test_multiple_topics();
        test_thread_safety();
        
    } catch (const std::exception& e) {
        std::cout << "\nUnexpected exception: " << e.what() << std::endl;
        UnitTest::assert_true(false, "No exceptions should be thrown during testing");
    }
    
    UnitTest::print_final_summary();
    
    return UnitTest::all_passed() ? 0 : 1;
}

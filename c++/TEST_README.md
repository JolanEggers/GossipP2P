# GossipNode C++ Test Suite

This directory contains comprehensive tests for the GossipNode C++ implementation, including both unit tests and integration tests.

## Test Files

### 1. `test_gossip_node.cpp` - Integration Tests
Comprehensive integration tests that test the full functionality of the GossipNode including:
- Basic construction and initialization
- Local subscription and publishing
- Multi-node communication
- Network message propagation
- Concurrent operations
- Stress testing with multiple messages
- Edge cases and error handling

### 2. `unit_tests.cpp` - Unit Tests
Focused unit tests that test specific components and functions:
- Constructor behavior with different parameters
- JSON info structure validation
- Subscription management
- Known nodes management
- Message publishing mechanics
- Special character handling
- Multi-topic scenarios
- Thread safety

## Building and Running Tests

### Prerequisites
- C++17 compatible compiler (g++ recommended)
- Make build system
- POSIX-compliant system (Linux/macOS)

### Quick Start
```bash
# Build and run all tests
make test

# Build and run only unit tests
make unit-test

# Build and run only integration tests
make integration-test

# Build all executables (including tests)
make all

# Clean build artifacts
make clean
```

### Manual Compilation
```bash
# Compile unit tests
g++ -std=c++17 -Wall -Wextra -O2 -pthread -I. unit_tests.cpp GossipNode.cpp -o unit_tests

# Compile integration tests
g++ -std=c++17 -Wall -Wextra -O2 -pthread -I. test_gossip_node.cpp GossipNode.cpp -o test_gossip_node

# Run tests
./unit_tests
./test_gossip_node
```

## Test Categories

### Unit Tests
1. **Constructor and Basic Properties**
   - Default constructor behavior
   - Custom IP and port configuration
   - Exception handling

2. **JSON Info Structure**
   - Required field validation
   - Initial state verification
   - Data structure integrity

3. **Subscription Management**
   - Single and multiple subscriptions
   - Callback execution
   - Topic registration in node info

4. **Known Nodes Management**
   - Adding nodes with and without topics
   - Duplicate node handling
   - Node information persistence

5. **Message Publishing**
   - Local message delivery
   - Multiple message handling
   - Non-existent topic robustness

6. **Special Characters and Edge Cases**
   - Empty content handling
   - Special character preservation
   - Unicode support
   - Large content handling

7. **Multiple Topics**
   - Topic isolation
   - Selective message delivery
   - Cross-topic independence

8. **Thread Safety**
   - Concurrent publishing
   - Thread-safe callback execution
   - Race condition prevention

### Integration Tests
1. **Basic Construction** - Node initialization and info generation
2. **Subscription Functionality** - Local pub/sub mechanics
3. **Multiple Subscriptions** - Multiple callbacks per topic
4. **Node Discovery** - Adding and managing known nodes
5. **Inter-Node Communication** - Two-node message passing
6. **Bidirectional Communication** - Mutual node communication
7. **Multiple Messages** - High-volume message handling
8. **Edge Cases** - Boundary conditions and error scenarios
9. **Concurrent Operations** - Multi-threaded stress testing

## Test Output

### Success Indicators
- ✓ PASS: Test passed successfully
- Green/positive indicators for all tests
- Final summary showing 100% success rate

### Failure Indicators
- ✗ FAIL: Test failed with details
- Expected vs actual value comparisons
- Final summary showing failed test count

### Example Output
```
=== Constructor and Basic Properties ===
✓ Default constructor should create valid node
✓ Custom constructor should use specified IP
✓ Custom constructor should use specified port

=== Test Summary ===
Total tests: 45
Passed: 45
Failed: 0
Success rate: 100.0%

🎉 ALL TESTS PASSED! 🎉
```

## Network Ports Used in Tests

The tests use ports in the 5100-5300 range to avoid conflicts:
- Unit tests: 5200-5206
- Integration tests: 5100-5112

Make sure these ports are available when running tests.

## Troubleshooting

### Common Issues

1. **Port binding errors**
   - Ensure test ports (5100-5300) are not in use
   - Wait a few seconds between test runs for port cleanup

2. **Compilation errors**
   - Verify C++17 support: `g++ --version`
   - Check that `json.hpp` is present in the directory
   - Ensure pthread library is available

3. **Test timeouts**
   - Network operations may take longer on some systems
   - Tests include appropriate wait times, but slow systems may need adjustment

4. **Memory issues**
   - Tests create multiple nodes; ensure sufficient memory
   - Run tests individually if experiencing resource constraints

### Debugging
- Enable verbose output by modifying test timeout values
- Add debug prints in test callbacks
- Run tests individually to isolate issues

## Test Design Principles

1. **Isolation**: Each test is independent and doesn't affect others
2. **Deterministic**: Tests produce consistent results across runs
3. **Comprehensive**: Cover both happy path and edge cases
4. **Fast**: Tests complete quickly for rapid development feedback
5. **Clear**: Test names and output clearly indicate what's being tested

## Contributing

When adding new tests:
1. Follow the existing test structure and naming conventions
2. Use unique port numbers to avoid conflicts
3. Include both positive and negative test cases
4. Add appropriate wait times for network operations
5. Update this README with new test categories 
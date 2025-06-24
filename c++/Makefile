CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
INCLUDES = -I.

# Source files
GOSSIP_SOURCES = GossipNode.cpp
GOSSIP_OBJECTS = $(GOSSIP_SOURCES:.cpp=.o)

# Executables
EXECUTABLES = publisher subscriber subscriberH test_gossip_node unit_tests

.PHONY: all clean test run-tests

all: $(EXECUTABLES)

# Compile GossipNode object file
GossipNode.o: GossipNode.cpp GossipNode.h json.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Publisher executable
publisher: publisher.cpp $(GOSSIP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Subscriber executable
subscriber: subscriber.cpp $(GOSSIP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# SubscriberH executable
subscriberH: subscriberH.cpp $(GOSSIP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Test executable
test_gossip_node: test_gossip_node.cpp $(GOSSIP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Unit test executable
unit_tests: unit_tests.cpp $(GOSSIP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Run tests
test: test_gossip_node unit_tests
	@echo "Running GossipNode integration tests..."
	./test_gossip_node
	@echo ""
	@echo "Running GossipNode unit tests..."
	./unit_tests

run-tests: test

# Run only unit tests
unit-test: unit_tests
	@echo "Running GossipNode unit tests..."
	./unit_tests

# Run only integration tests
integration-test: test_gossip_node
	@echo "Running GossipNode integration tests..."
	./test_gossip_node

# Clean build artifacts
clean:
	rm -f $(GOSSIP_OBJECTS) $(EXECUTABLES)
	rm -f *.o

# Install dependencies (for Ubuntu/Debian systems)
install-deps:
	@echo "Installing build dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential g++ make

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build all executables"
	@echo "  publisher        - Build publisher executable"
	@echo "  subscriber       - Build subscriber executable"
	@echo "  subscriberH      - Build subscriberH executable"
	@echo "  test             - Build and run all tests"
	@echo "  unit-test        - Build and run unit tests only"
	@echo "  integration-test - Build and run integration tests only"
	@echo "  run-tests        - Alias for test"
	@echo "  clean            - Remove build artifacts"
	@echo "  install-deps     - Install build dependencies (Ubuntu/Debian)"
	@echo "  help             - Show this help message" 
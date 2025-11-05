# Makefile for RISC-V Simulator
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
TESTDIR = test
OBJDIR = obj

# Create object files directory
$(shell mkdir -p $(OBJDIR))

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Test files
TEST_SOURCES = $(wildcard $(TESTDIR)/test_*.cpp)
TEST_TARGETS = $(TEST_SOURCES:$(TESTDIR)/test_%.cpp=$(TESTDIR)/test_%)

# Default target
all: simulator tests

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

# Build main program
simulator: $(OBJECTS) sim.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) sim.cpp $(OBJECTS) -o simulator

# Build test programs
$(TESTDIR)/test_%: $(TESTDIR)/test_%.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $< $(OBJECTS) -o $@

# Build all tests
tests: $(TEST_TARGETS)

# Run tests
run-tests: tests
	@echo "=== Running All Tests ==="
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test..."; \
		$$test; \
		echo ""; \
	done

# Test compilation (no linking)
test-compile: $(OBJECTS)
	@echo "=== Testing Compilation ==="
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c sim.cpp -o sim.o
	@echo "Compilation test successful!"

# Clean up
clean:
	rm -rf $(OBJDIR)
	rm -f $(TEST_TARGETS)
	rm -f simulator sim.o
	rm -rf $(TESTDIR)/test_data

# Clean test data
clean-test-data:
	rm -rf $(TESTDIR)/test_data

.PHONY: all tests run-tests clean clean-test-data simulator test-compile
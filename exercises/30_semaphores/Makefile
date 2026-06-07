CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++20 -g -pthread
TARGET   = semaphores
SRC      = semaphores.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

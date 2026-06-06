CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g
TARGET   = stl_containers
SRC      = stl_containers.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

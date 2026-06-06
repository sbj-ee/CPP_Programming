CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g
TARGET   = lambdas
SRC      = lambdas.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

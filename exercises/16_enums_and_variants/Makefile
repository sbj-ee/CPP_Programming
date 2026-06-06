CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g
TARGET   = enums_variants
SRC      = enums_variants.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

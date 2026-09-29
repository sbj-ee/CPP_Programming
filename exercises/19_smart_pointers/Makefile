CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g
TARGET   = smart_pointers
SRC      = smart_pointers.cpp

.PHONY: all clean valgrind

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

# Run under valgrind memcheck (Linux); non-zero exit on memory errors or
# definite/possible leaks
valgrind: all
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET)

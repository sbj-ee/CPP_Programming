CXX       = g++
CXXFLAGS  = -Wall -Wextra -Wpedantic -std=c++17 -g
PLUGFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g -fPIC -shared
TARGET    = dl_demo
PLUGIN    = plugin.so

.PHONY: all clean valgrind

all: $(PLUGIN) $(TARGET)

$(PLUGIN): plugin.cpp plugin.hpp
	$(CXX) $(PLUGFLAGS) $< -o $@

$(TARGET): dl_demo.cpp plugin.hpp
	$(CXX) $(CXXFLAGS) $< -ldl -o $@

clean:
	rm -f $(TARGET) $(PLUGIN)

# Run under valgrind memcheck (Linux); non-zero exit on memory errors or
# definite/possible leaks
valgrind: all
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET)

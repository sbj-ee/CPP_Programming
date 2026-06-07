// plugin.cpp — Math plugin for Exercise 31: Dynamic Loading
//
// Compiled as a shared library:
//   g++ -Wall -Wextra -Wpedantic -std=c++17 -g -fPIC -shared plugin.cpp -o plugin.so
//
// -fPIC: Position-Independent Code — required for shared libraries so that
//        the code can be loaded at any address in the loader's virtual space.
// -shared: produce a shared object (.so) instead of an executable.

#include "plugin.hpp"
#include <cmath>    // std::sqrt

// All three functions use C linkage (extern "C") so dlsym() can find them
// by their unmangled names (e.g., "plugin_compute" not "_Z14plugin_computed").

extern "C" {

const char* plugin_name() {
    return "math_plugin";
}

// f(x) = x^2 + 1
double plugin_compute(double input) {
    return input * input + 1.0;
}

// Square each element of arr[] in-place
void plugin_process_array(double* arr, size_t len) {
    for (size_t i = 0; i < len; ++i)
        arr[i] = arr[i] * arr[i];
}

} // extern "C"

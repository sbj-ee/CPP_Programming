// plugin.hpp — Plugin interface for Exercise 31: Dynamic Loading
//
// This header declares the Plugin struct that both the shared library and
// the loader agree on.  The functions are plain C linkage so that dlsym()
// can find them without C++ name mangling.

#pragma once

#include <cstddef>  // size_t

// The Plugin vtable — a struct of function pointers.
// The loader calls dlsym() once for each symbol name.
struct Plugin {
    // Return a human-readable name for this plugin.
    const char* (*name)();

    // Perform the plugin's primary operation on a double value.
    double (*compute)(double input);

    // Process an array of doubles in-place.
    void (*process_array)(double* arr, size_t len);
};

// C-linkage symbols exported by the plugin shared library:
//   plugin_name()         — returns static "math_plugin"
//   plugin_compute(x)     — returns x * x + 1.0
//   plugin_process_array  — squares each element in-place
extern "C" {
    const char* plugin_name();
    double      plugin_compute(double input);
    void        plugin_process_array(double* arr, size_t len);
}

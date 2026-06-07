// =============================================================================
// Exercise 31: Dynamic Loading in C++ with RAII
// =============================================================================
// Topics: DynLib RAII wrapper, dlopen/dlsym/dlclose/dlerror,
//         Plugin dispatch via DynLib, RTLD_NOW vs RTLD_LAZY,
//         RTLD_DEFAULT to find libc symbols, pitfalls
//
// Build: see Makefile (plugin.so must be built first)
// =============================================================================

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <vector>
#include <stdexcept>

#include <dlfcn.h>  // dlopen, dlsym, dlclose, dlerror

#include "plugin.hpp"

// =============================================================================
// RAII DynLib Wrapper
// =============================================================================
//
// Opens a shared library in the constructor (via dlopen) and closes it in
// the destructor (via dlclose).  This prevents handle leaks when exceptions
// propagate.  dlclose() decrements the library's reference count; the library
// is unloaded only when the count reaches zero.

class DynLib {
public:
    explicit DynLib(const char* path, int flags = RTLD_NOW | RTLD_LOCAL)
        : handle_(nullptr), path_(path ? path : "")
    {
        // Clear any prior error
        ::dlerror();
        handle_ = ::dlopen(path, flags);
        if (!handle_) {
            std::string err = ::dlerror();
            throw std::runtime_error("dlopen(\"" + path_ + "\"): " + err);
        }
    }

    ~DynLib() {
        if (handle_) ::dlclose(handle_);
    }

    // Non-copyable, movable
    DynLib(const DynLib&)            = delete;
    DynLib& operator=(const DynLib&) = delete;

    DynLib(DynLib&& o) noexcept : handle_(o.handle_), path_(std::move(o.path_)) {
        o.handle_ = nullptr;
    }

    // Resolve a symbol by name.  Returns nullptr if not found.
    void* sym(const char* name) const {
        ::dlerror();  // clear prior error
        void* s = ::dlsym(handle_, name);
        const char* err = ::dlerror();
        if (err) {
            std::cerr << "dlsym(\"" << name << "\"): " << err << "\n";
            return nullptr;
        }
        return s;
    }

    // Type-safe symbol lookup — cast to function pointer type T*.
    // Using memcpy to cast avoids strict-aliasing UB (ISO C++ requirement).
    template<typename FuncPtr>
    FuncPtr sym_as(const char* name) const {
        void* raw = sym(name);
        if (!raw) return nullptr;
        FuncPtr fp;
        std::memcpy(&fp, &raw, sizeof(fp));
        return fp;
    }

    bool   valid() const { return handle_ != nullptr; }
    void*  handle()const { return handle_; }

private:
    void*       handle_;
    std::string path_;
};

// =============================================================================
// SECTION 1: Concepts
// =============================================================================

static void section1_concepts() {
    std::cout << "\n=== Section 1: Dynamic loading concepts ===\n";

    std::cout << "  Dynamic loading lets a program load shared libraries (.so)\n";
    std::cout << "  at runtime, enabling plugin architectures and hot-reload.\n\n";

    std::cout << "  POSIX API (libdl):\n";
    std::cout << "    dlopen(path, flags) — load library; increment refcount.\n";
    std::cout << "                          path=NULL: main program's symbol table.\n";
    std::cout << "    dlsym(handle, name) — find a symbol; returns void*.\n";
    std::cout << "    dlclose(handle)     — decrement refcount; unload if 0.\n";
    std::cout << "    dlerror()           — return last error string (or NULL).\n\n";

    std::cout << "  Flags:\n";
    std::cout << "    RTLD_LAZY  — resolve symbols on first use (faster load).\n";
    std::cout << "    RTLD_NOW   — resolve all symbols at load time (fail early).\n";
    std::cout << "    RTLD_GLOBAL— symbols become available for subsequent dlopen.\n";
    std::cout << "    RTLD_LOCAL — symbols are private to this handle (default).\n\n";

    std::cout << "  RTLD_DEFAULT — special pseudo-handle; searches all globally\n";
    std::cout << "    loaded symbols (main + RTLD_GLOBAL libs).\n";
}

// =============================================================================
// SECTION 2: Basic dlopen / dlsym / dlclose + dlerror
// =============================================================================

static void section2_basic() {
    std::cout << "\n=== Section 2: Basic dlopen / dlsym / dlclose ===\n";

    try {
        DynLib lib("./plugin.so", RTLD_NOW | RTLD_LOCAL);
        std::cout << "  Loaded ./plugin.so\n";

        // Resolve the name function
        auto name_fn = lib.sym_as<const char*(*)()>("plugin_name");
        if (name_fn)
            std::cout << "  plugin_name() = \"" << name_fn() << "\"\n";

        // Resolve compute function
        auto compute = lib.sym_as<double(*)(double)>("plugin_compute");
        if (compute) {
            std::cout << "  plugin_compute(5.0) = " << compute(5.0) << "\n";
            std::cout << "  plugin_compute(3.0) = " << compute(3.0) << "\n";
        }

        // Resolve array function
        auto proc = lib.sym_as<void(*)(double*, size_t)>("plugin_process_array");
        if (proc) {
            double arr[] = {1.0, 2.0, 3.0, 4.0};
            proc(arr, 4);
            std::cout << "  plugin_process_array([1,2,3,4]) -> [";
            for (int i = 0; i < 4; ++i)
                std::cout << arr[i] << (i < 3 ? "," : "");
            std::cout << "]\n";
        }

        // DynLib destructor calls dlclose() here
        std::cout << "  DynLib goes out of scope — dlclose() called.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "  " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 3: Plugin dispatch via DynLib RAII wrapper
// =============================================================================
//
// A real plugin system: build the Plugin struct from three dlsym() calls,
// then call through the function pointers — no direct link-time dependency.

static void section3_plugin_dispatch() {
    std::cout << "\n=== Section 3: Plugin dispatch via DynLib ===\n";

    try {
        DynLib lib("./plugin.so");

        Plugin p;
        p.name          = lib.sym_as<const char*(*)()>("plugin_name");
        p.compute       = lib.sym_as<double(*)(double)>("plugin_compute");
        p.process_array = lib.sym_as<void(*)(double*, size_t)>("plugin_process_array");

        if (!p.name || !p.compute || !p.process_array) {
            std::cerr << "  Failed to resolve all plugin symbols.\n";
            return;
        }

        std::cout << "  Plugin name: " << p.name() << "\n";

        // Dispatch through the vtable
        const std::vector<double> inputs = {0.0, 1.0, 2.0, 5.0, -3.0};
        for (double x : inputs)
            std::cout << "  p.compute(" << x << ") = " << p.compute(x) << "\n";

        // Array processing
        std::vector<double> arr = {2.0, 3.0, 4.0};
        p.process_array(arr.data(), arr.size());
        std::cout << "  p.process_array([2,3,4]) -> [";
        for (size_t i = 0; i < arr.size(); ++i)
            std::cout << arr[i] << (i + 1 < arr.size() ? "," : "");
        std::cout << "]\n";

    } catch (const std::runtime_error& e) {
        std::cerr << "  " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 4: RTLD_NOW vs RTLD_LAZY + error on missing library
// =============================================================================

static void section4_flags() {
    std::cout << "\n=== Section 4: RTLD_NOW vs RTLD_LAZY, missing library ===\n";

    // RTLD_LAZY — symbols resolved on first call
    try {
        DynLib lazy("./plugin.so", RTLD_LAZY | RTLD_LOCAL);
        std::cout << "  RTLD_LAZY: loaded successfully\n";
        auto fn = lazy.sym_as<const char*(*)()>("plugin_name");
        if (fn) std::cout << "  lazy plugin_name() = \"" << fn() << "\"\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "  RTLD_LAZY: " << e.what() << "\n";
    }

    // RTLD_NOW — all symbols resolved at load; undefined symbols cause failure
    try {
        DynLib now("./plugin.so", RTLD_NOW | RTLD_LOCAL);
        std::cout << "  RTLD_NOW:  loaded successfully (all symbols resolved)\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "  RTLD_NOW: " << e.what() << "\n";
    }

    // Missing library — dlopen should fail
    std::cout << "\n  Attempting to load a non-existent library:\n";
    try {
        DynLib bad("./no_such_library.so", RTLD_NOW);
        std::cout << "  ERROR: should not reach here\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught expected error: " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 5: RTLD_DEFAULT — find strlen in libc
// =============================================================================
//
// Passing NULL (RTLD_DEFAULT) to dlsym() searches all symbols in the current
// process, including those from libc, libstdc++, and any RTLD_GLOBAL libs.
// This is useful to locate well-known runtime symbols by name.

static void section5_rtld_default() {
    std::cout << "\n=== Section 5: RTLD_DEFAULT — find strlen from libc ===\n";

    ::dlerror();
    void* raw = ::dlsym(RTLD_DEFAULT, "strlen");
    const char* err = ::dlerror();
    if (err) {
        std::cerr << "  dlsym(RTLD_DEFAULT, strlen): " << err << "\n";
        return;
    }

    // Cast void* to function pointer via memcpy (avoids strict-aliasing UB)
    using StrlenFn = size_t(*)(const char*);
    StrlenFn my_strlen;
    std::memcpy(&my_strlen, &raw, sizeof(my_strlen));

    const char* test = "Hello, RTLD_DEFAULT!";
    size_t len = my_strlen(test);
    std::cout << "  strlen(\"" << test << "\") via RTLD_DEFAULT = " << len << "\n";

    // Also look for a symbol that definitely does NOT exist
    ::dlerror();
    void* missing = ::dlsym(RTLD_DEFAULT, "this_symbol_does_not_exist_xyz");
    const char* missing_err = ::dlerror();
    if (missing == nullptr && !missing_err) {
        // dlsym returns NULL with no error when symbol simply isn't found
        std::cout << "  dlsym(RTLD_DEFAULT, <missing>): NULL (symbol not found)\n";
    } else if (missing_err) {
        std::cout << "  dlsym error: " << missing_err << "\n";
    }
}

// =============================================================================
// SECTION 6: Pitfalls
// =============================================================================

static void section6_pitfalls() {
    std::cout << "\n=== Section 6: Pitfalls ===\n";

    std::cout << "  1. VOID* CAST: dlsym returns void*, but converting directly to\n";
    std::cout << "     a function pointer is undefined in C++ (but allowed in C99).\n";
    std::cout << "     Fix: use memcpy(&fp, &raw, sizeof(fp)) — safe in C++.\n";
    std::cout << "     Most compilers accept reinterpret_cast or C cast as an extension.\n\n";

    std::cout << "  2. SYMBOL NOT FOUND: dlsym returns NULL with NO error if the symbol\n";
    std::cout << "     does not exist (dlerror returns NULL too).  Check return value\n";
    std::cout << "     separately from dlerror.\n\n";

    std::cout << "  3. NAME MANGLING: C++ functions have mangled names (e.g.,\n";
    std::cout << "     _Z14plugin_computed).  Use extern \"C\" in the plugin to export\n";
    std::cout << "     unmangled names that dlsym can find.\n\n";

    std::cout << "  4. REFCOUNTING: dlopen/dlclose are refcounted.  Two dlopen calls\n";
    std::cout << "     need two dlclose calls.  DynLib RAII handles one pair.\n\n";

    std::cout << "  5. LIBRARY UNLOADED WHILE IN USE: if DynLib goes out of scope\n";
    std::cout << "     while a function pointer from it is still live, calling that\n";
    std::cout << "     pointer is undefined behaviour.  Keep the DynLib alive as long\n";
    std::cout << "     as any resolved symbols are reachable.\n\n";

    std::cout << "  6. -fPIC required: shared libraries must be compiled with -fPIC\n";
    std::cout << "     (position-independent code) so the dynamic linker can map them\n";
    std::cout << "     at an arbitrary virtual address.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 31: Dynamic Loading in C++ ===\n";

    section1_concepts();
    section2_basic();
    section3_plugin_dispatch();
    section4_flags();
    section5_rtld_default();
    section6_pitfalls();

    std::cout << "\nDone.\n";
    return 0;
}

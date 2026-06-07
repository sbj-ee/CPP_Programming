# Dynamic Loading — Cheat Sheet

> POSIX (Portable Operating System Interface) `dlopen`/`dlclose`/`dlsym` API is identical in C++. C++ adds a RAII `DynLib` class and typed helper methods for function pointers.

## Headers

```cpp
#include <dlfcn.h>       // dlopen, dlclose, dlsym, dlerror
#include <stdexcept>
#include <string>
#include <functional>
// Link with: -ldl  (not needed on macOS)
```

---

## Core API

```cpp
// Open a shared library
void* handle = dlopen("libfoo.so",     // library path; nullptr = main executable
    RTLD_LAZY |     // resolve symbols on first use (vs RTLD_NOW = immediate)
    RTLD_LOCAL      // don't make symbols available to subsequently loaded libs
);
// Flags:
//   RTLD_LAZY / RTLD_NOW
//   RTLD_LOCAL / RTLD_GLOBAL  — symbol visibility
//   RTLD_NODELETE             — don't unload on dlclose (ref-count)
//   RTLD_NOLOAD               — return handle if already loaded, else nullptr
//   RTLD_DEEPBIND (Linux)     — use library's own symbols before global scope

if (!handle) {
    throw std::runtime_error(std::string("dlopen: ") + dlerror());
}

// Look up a symbol
void* sym = dlsym(handle, "my_function");
const char* err = dlerror();   // must call AFTER dlsym; not thread-safe
if (err) {
    throw std::runtime_error(std::string("dlsym: ") + err);
}

// Cast to the correct function pointer type
using FuncType = int(*)(int, double);
FuncType fn = reinterpret_cast<FuncType>(sym);  // technically UB in ISO C++; see Pitfalls section for the memcpy workaround
int result = fn(42, 3.14);

// Close library (decrements reference count; freed when count reaches 0)
dlclose(handle);

// Get info about an address (function/variable → library, name, etc.)
Dl_info info;
if (dladdr(sym, &info)) {
    printf("lib: %s, symbol: %s\n", info.dli_fname, info.dli_sname);
}
```

---

## Symbol Lookup Patterns

```cpp
// Clear previous error, then look up
dlerror();   // clear
void* sym = dlsym(handle, "foo");
const char* error = dlerror();   // non-null = error; null = success (sym may be nullptr)

// Note: sym == nullptr is NOT necessarily an error!
// A symbol may legitimately have the value 0 (e.g. a variable initialised to 0)
// Always check dlerror(), not just sym == nullptr

// Special handles:
dlsym(RTLD_DEFAULT, "malloc");  // search in global symbol table (already loaded libs)
dlsym(RTLD_NEXT, "malloc");     // next definition after current library (for wrapping)
```

---

## Exporting Symbols from a Shared Library

```cpp
// In library source (.cpp):
extern "C" {          // C linkage — prevents C++ name mangling
    int  my_add(int a, int b) { return a + b; }
    void my_init() { /* ... */ }
}
// Compile: g++ -shared -fPIC -o libfoo.so foo.cpp

// Without extern "C", the mangled name is compiler-specific:
// "my_function" might be "_ZN3Foo10my_functionEi" — can't look up by simple name

// Attribute visibility (GCC/Clang):
__attribute__((visibility("default"))) int public_fn();   // exported
__attribute__((visibility("hidden")))  int private_fn();  // not exported
// Compile with: -fvisibility=hidden  to hide all symbols by default
```

---

## Shared Library Versioning

```cpp
// soname and real name:
// libfoo.so.1       → soname (used at link time and runtime)
// libfoo.so.1.2.3   → real name (actual file)
// libfoo.so         → link name (symlink; used by linker at build time)

// Build with soname:
// g++ -shared -fPIC -Wl,-soname,libfoo.so.1 -o libfoo.so.1.2.3 foo.cpp
// ldconfig  (or set LD_LIBRARY_PATH)

// dlopen("libfoo.so.1") uses soname directly
// dlopen("libfoo.so")   uses the link-name symlink (dev machines only)
```

---

## RAII (Resource Acquisition Is Initialisation) `DynLib` Class (C++ Style)

```cpp
class DynLib {
    void* handle_ = nullptr;

public:
    explicit DynLib(const std::string& path, int flags = RTLD_LAZY | RTLD_LOCAL) {
        dlerror();   // clear
        handle_ = dlopen(path.c_str(), flags);
        if (!handle_) throw std::runtime_error(std::string("dlopen: ") + dlerror());
    }

    // Open main executable's symbols
    static DynLib main_exe() {
        DynLib d;
        d.handle_ = dlopen(nullptr, RTLD_LAZY | RTLD_LOCAL);
        return d;
    }

    ~DynLib() { if (handle_) dlclose(handle_); }

    DynLib(const DynLib&) = delete;
    DynLib& operator=(const DynLib&) = delete;
    DynLib(DynLib&& o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }
    DynLib& operator=(DynLib&& o) noexcept {
        if (this != &o) { if (handle_) dlclose(handle_); handle_ = o.handle_; o.handle_ = nullptr; }
        return *this;
    }

    // Generic symbol lookup
    void* symbol(const std::string& name) const {
        dlerror();
        void* sym = dlsym(handle_, name.c_str());
        const char* err = dlerror();
        if (err) throw std::runtime_error(std::string("dlsym '") + name + "': " + err);
        return sym;
    }

    // Typed function pointer lookup
    // Note: casting void* to a function pointer via reinterpret_cast is technically
    // UB (Undefined Behaviour) in ISO C++, but is POSIX-blessed and works on all
    // mainstream platforms. Strict-conformance alternative: memcpy(&fp, &sym, sizeof fp).
    template<typename Signature>
    std::function<Signature> func(const std::string& name) const {
        using FP = Signature*;
        return reinterpret_cast<FP>(symbol(name));
    }

    // Or simpler — return raw function pointer
    template<typename FP>
    FP func_ptr(const std::string& name) const {
        return reinterpret_cast<FP>(symbol(name));
    }

    void* get() const { return handle_; }
    void* release() { void* h = handle_; handle_ = nullptr; return h; }
private:
    DynLib() = default;
};

// Usage
DynLib lib("libmath_plugin.so");

// Typed via function<>
auto add = lib.func<int(int,int)>("plugin_add");
int r = add(3, 4);

// Raw function pointer (avoids std::function overhead)
using CalcFn = double(*)(double, double);
CalcFn calc = lib.func_ptr<CalcFn>("plugin_calc");
double v = calc(1.0, 2.0);

// Struct of function pointers (plugin interface pattern)
struct Plugin {
    void (*init)();
    void (*cleanup)();
    int  (*process)(int);
};

Plugin p;
p.init    = lib.func_ptr<void(*)()>("plugin_init");
p.cleanup = lib.func_ptr<void(*)()>("plugin_cleanup");
p.process = lib.func_ptr<int(*)(int)>("plugin_process");
p.init();
int out = p.process(42);
p.cleanup();
// DynLib destructor calls dlclose automatically
```

---

## Plugin System Pattern

```cpp
// plugin_api.h — shared between host and plugins
struct PluginAPI {
    int version;
    const char* (*get_name)();
    int (*run)(const char* input, char* output, int output_len);
};

extern "C" PluginAPI* get_plugin();   // every plugin exports this

// host.cpp
void load_plugin(const std::string& path) {
    DynLib lib(path);
    using GetPlugin = PluginAPI*(*)();
    GetPlugin getter = lib.func_ptr<GetPlugin>("get_plugin");
    PluginAPI* api = getter();
    if (api->version != EXPECTED_VERSION) throw std::runtime_error("version mismatch");
    char out[1024];
    api->run("hello", out, sizeof(out));
    // lib destroyed when DynLib goes out of scope; no manual dlclose needed
}

// plugin.cpp
extern "C" {
    static const char* get_name_impl() { return "my_plugin"; }
    static int run_impl(const char* in, char* out, int len) { /* ... */ return 0; }
    static PluginAPI api{1, get_name_impl, run_impl};
    PluginAPI* get_plugin() { return &api; }
}
// Compile: g++ -shared -fPIC -o my_plugin.so plugin.cpp
```

---

## Pitfalls

```cpp
// 1. Forgetting extern "C" → symbol not found
// C++ name mangling makes "void foo(int)" into "_ZN3foo1Ei" or similar
// dlsym("foo") fails; must use extern "C" or nm/objdump to find mangled name

// 2. dlerror() must be called ONCE immediately after dlsym
// It returns a static string that may be overwritten by subsequent calls
// Pattern: dlerror(); dlsym(); const char* err = dlerror();

// 3. Using symbol after dlclose
DynLib lib("libfoo.so");
auto fn = lib.func_ptr<void(*)()>("foo");
// lib goes out of scope here if it's a temporary → dlclose called → fn is dangling
// Always keep DynLib alive at least as long as you use the symbols

// 4. Linking errors: missing -ldl on Linux
// g++ -o app main.cpp -ldl

// 5. LD_LIBRARY_PATH not set → dlopen("libfoo.so") fails
// Either use full path, or set LD_LIBRARY_PATH, or use rpath:
// g++ -Wl,-rpath,/usr/local/lib/plugins ...

// 6. RTLD_GLOBAL vs RTLD_LOCAL
// RTLD_GLOBAL: symbols available to subsequently loaded libs (needed for plugins that share a common library)
// RTLD_LOCAL: symbols hidden; reduces symbol conflicts; prefer unless cross-lib sharing needed

// 7. Thread safety of dlerror
// dlerror() is NOT thread-safe in all implementations.
// Protect dlopen/dlsym/dlclose with a mutex in multi-threaded code.

// 8. Static destructors in dlclose'd library
// Static objects in the library have destructors called on dlclose.
// If they reference globals in the main exe or other libs, order matters.
// Use RTLD_NODELETE to keep library loaded (prevent destructor issues).
```

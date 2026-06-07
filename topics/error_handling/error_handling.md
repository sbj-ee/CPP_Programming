# Exceptions & Error Handling — Cheat Sheet

## `throw`, `try`, `catch`

```cpp
#include <stdexcept>

// Throw — any copyable type; prefer std::exception subclasses
throw std::runtime_error("disk full");
throw 42;                // valid but bad practice
throw std::string("oops");

// try / catch
try {
    risky_operation();
}
catch (const std::runtime_error& e) {   // specific first
    std::cerr << "runtime: " << e.what() << '\n';
}
catch (const std::exception& e) {       // base class
    std::cerr << "exception: " << e.what() << '\n';
}
catch (...) {                           // catch anything
    std::cerr << "unknown exception\n";
    throw;                              // re-throw preserving type
}
```

---

## Standard Exception Hierarchy

```
std::exception          (.what() returns const char*)
├── std::logic_error    (programming errors — should not happen)
│   ├── std::invalid_argument
│   ├── std::domain_error
│   ├── std::length_error
│   └── std::out_of_range
├── std::runtime_error  (external errors — can happen)
│   ├── std::range_error
│   ├── std::overflow_error
│   └── std::underflow_error
├── std::bad_alloc      (new failed)
├── std::bad_cast       (dynamic_cast to ref failed)
├── std::bad_typeid     (typeid on null pointer)
├── std::bad_function_call
└── std::ios::failure   (I/O error)
```

---

## Custom Exception Classes

```cpp
class DatabaseError : public std::runtime_error {
public:
    explicit DatabaseError(const std::string& msg, int code = 0)
        : std::runtime_error(msg), code_(code) {}
    int code() const { return code_; }
private:
    int code_;
};

// Hierarchy
class NetworkError : public std::runtime_error {
    using std::runtime_error::runtime_error;  // inherit constructor
};
class TimeoutError : public NetworkError {
    using NetworkError::NetworkError;
};

// Usage
try {
    throw DatabaseError("connection refused", 1003);
}
catch (const DatabaseError& e) {
    std::cerr << e.what() << " (code=" << e.code() << ")\n";
}
```

---

## Re-throw

```cpp
try {
    f();
}
catch (const std::exception& e) {
    log(e.what());
    throw;        // re-throw SAME exception (preserves type and info)
    // NOT: throw e;  ← slices to std::exception!
}

// Nested exceptions (C++11)
void handle() {
    try { f(); }
    catch (...) {
        std::throw_with_nested(std::runtime_error("wrapper"));
    }
}

// Retrieve nested exception
try { handle(); }
catch (const std::exception& e) {
    std::cout << e.what() << '\n';
    try { std::rethrow_if_nested(e); }
    catch (const std::exception& nested) {
        std::cout << "caused by: " << nested.what() << '\n';
    }
}
```

---

## `noexcept`

```cpp
void f() noexcept;           // guarantees no exception
void g() noexcept(true);     // same
void h() noexcept(false);    // may throw (default)

// Conditional noexcept
template<typename T>
void swap(T& a, T& b) noexcept(noexcept(T(std::move(a))));
// noexcept if move constructor is noexcept

// Check at compile time
static_assert(noexcept(f()));  // true

// Rule: destructors and move operations should be noexcept
// Violation: if noexcept function throws → std::terminate() called immediately
```

---

## Exception Safety Levels

| Level | Guarantee |
|-------|-----------|
| **No-throw** | Never throws; `noexcept` guaranteed |
| **Strong** | Either succeeds or has no effect (rollback) |
| **Basic** | Object stays valid; no resource leaks; state may change |
| **None** | Object may be in undefined state |

```cpp
// Strong guarantee via copy-and-swap idiom
Buffer& Buffer::operator=(Buffer other) {  // copy or move into temp
    swap(*this, other);                    // noexcept swap
    return *this;                          // old data cleaned up in temp's dtor
}

// Basic guarantee example — vector left valid but modified
void append_all(std::vector<int>& v, const std::vector<int>& src) {
    for (int x : src) v.push_back(x);  // if push_back throws, v is partly appended
}
```

---

## RAII (Resource Acquisition Is Initialisation) Ensures Cleanup on Exception

```cpp
void f() {
    std::ifstream file("data.txt");     // RAII: closes on scope exit
    auto lock = std::lock_guard(mtx);  // RAII: unlocks on scope exit
    auto mem = std::make_unique<int[]>(1000);  // RAII: frees on scope exit

    risky_call();   // if throws, all three are cleaned up correctly
}   // even if exception, file closed, mutex unlocked, memory freed

// Manual cleanup = ERROR-PRONE
void bad_f() {
    FILE* f = fopen("data.txt", "r");
    risky_call();   // throws → fclose never called → resource leak
    fclose(f);
}
```

---

## `std::optional<T>` (C++17)

```cpp
#include <optional>

// Represent absence without exceptions or sentinel values
std::optional<int> find_index(const std::vector<int>& v, int target) {
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == target) return i;      // implicit construction
    return std::nullopt;                   // absent
}

std::optional<int> result = find_index(v, 42);

// Access
result.has_value()          // or: (bool)result
result.value()              // throws std::bad_optional_access if empty
result.value_or(-1)         // default if empty
*result                     // UB (Undefined Behaviour) if empty; use has_value() first

// In-place construction
std::optional<std::string> os(std::in_place, "hello");
os.emplace("world");        // construct in place, destroy old

// Chaining (C++23 monadic)
auto upper = os.transform([](const std::string& s) { /* uppercase */ return s; });
auto len   = os.and_then([](const std::string& s) -> std::optional<size_t> { return s.size(); });
```

---

## `std::expected<T, E>` (C++23)

```cpp
#include <expected>  // C++23

std::expected<int, std::string> parse(const std::string& s) {
    try { return std::stoi(s); }
    catch (...) { return std::unexpected("not a number: " + s); }
}

auto r = parse("42");
if (r) { use(*r); }
else   { log(r.error()); }

r.value()           // throws std::bad_expected_access if error
r.value_or(0)       // default on error
r.error()           // the error (UB if has value)

// tl::expected — C++23 backport for C++17
// #include <tl/expected.hpp>
// tl::expected<int, std::string>  — same API
```

---

## `std::error_code` and `std::error_category`

```cpp
#include <system_error>

// Create from errno
std::error_code ec = std::make_error_code(std::errc::no_such_file_or_directory);
ec.value()       // errno value
ec.message()     // human-readable string
ec.category()    // std::system_category() or std::generic_category()

// Pass ec to avoid exceptions (most standard library functions have ec overloads)
std::error_code err;
std::filesystem::remove("x.txt", err);
if (err) { std::cerr << err.message() << '\n'; }

// Custom error category
enum class AppError { ok = 0, bad_input = 1, io_failure = 2 };

struct AppErrorCategory : std::error_category {
    const char* name() const noexcept override { return "app"; }
    std::string message(int ev) const override {
        switch (static_cast<AppError>(ev)) {
            case AppError::bad_input:  return "bad input";
            case AppError::io_failure: return "I/O failure";
            default:                   return "unknown";
        }
    }
};

const AppErrorCategory& app_error_category() {
    static AppErrorCategory cat;
    return cat;
}

std::error_code make_error_code(AppError e) {
    return {static_cast<int>(e), app_error_category()};
}
// Enable std::error_code construction from AppError:
template<> struct std::is_error_code_enum<AppError> : std::true_type {};
```

---

## Pitfalls

```cpp
// 1. Catch by value — slices
catch (std::exception e) { }   // BAD: copies and slices derived type
catch (const std::exception& e) { }  // GOOD

// 2. throw e — slices to static type of e
catch (const std::exception& e) {
    throw e;    // WRONG: throws copy of std::exception, not original Derived
    throw;      // RIGHT: re-throws original, preserving dynamic type
}

// 3. Swallowing exceptions — silent failure
try { f(); } catch (...) { }   // hides all errors; at least log!

// 4. Throwing in destructor — causes std::terminate during stack unwinding
~MyClass() {
    cleanup();   // if cleanup() throws while another exception is active → terminate
}
// Fix: destructor must be noexcept; wrap throwing code in try/catch

// 5. Exception from noexcept function — std::terminate (no unwinding)
void f() noexcept {
    throw std::runtime_error("oops");  // std::terminate called immediately
}

// 6. std::optional — accessing empty optional
std::optional<int> o;
int n = *o;            // UB
int m = o.value();     // throws std::bad_optional_access — safer
int k = o.value_or(0); // safest
```

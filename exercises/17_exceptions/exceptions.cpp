// =============================================================================
// Exercise 17: Exceptions
// =============================================================================
// Topics: throw/try/catch, standard hierarchy, custom exceptions, catch-all,
//         re-throw, noexcept, exception safety guarantees,
//         std::exception_ptr/current_exception, RAII under exceptions
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g exceptions.cpp -o exceptions
// =============================================================================

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <functional>

// =============================================================================
// SECTION 1: throw, try, catch — Basic Mechanics
// =============================================================================
//
// 'throw expr'  — throws an exception; unwinds the stack searching for a handler.
// 'try { } catch(Type& e) { }' — catches exceptions of Type (or derived).
// Catch clauses are checked top-to-bottom; more-derived types MUST come first.
// Catching by const reference is the standard convention — avoids slicing.

void demo_basic() {
    std::cout << "\n--- Section 1: Basic throw/try/catch ---\n";

    auto risky = [](int x) {
        if (x < 0) throw std::underflow_error("negative value: " + std::to_string(x));
        if (x > 100) throw std::overflow_error("value too large: " + std::to_string(x));
        std::cout << "  processed: " << x << "\n";
    };

    for (int val : {42, -5, 200}) {
        try {
            risky(val);
        } catch (const std::underflow_error& e) {
            std::cerr << "  [underflow] " << e.what() << "\n";
        } catch (const std::overflow_error& e) {
            std::cerr << "  [overflow]  " << e.what() << "\n";
        }
    }
}

// =============================================================================
// SECTION 2: Standard Exception Hierarchy
// =============================================================================
//
// std::exception (base — what())
//   ├── std::logic_error    — program-logic bugs (detectable at compile/design time)
//   │     ├── std::invalid_argument
//   │     ├── std::out_of_range
//   │     └── std::length_error
//   └── std::runtime_error  — conditions not detectable until runtime
//         ├── std::overflow_error
//         ├── std::underflow_error
//         └── std::range_error
//
// Catching a base type catches all derived types.

void demo_hierarchy() {
    std::cout << "\n--- Section 2: Standard Exception Hierarchy ---\n";

    std::vector<std::function<void()>> throwers = {
        []{ throw std::invalid_argument("bad arg"); },
        []{ throw std::out_of_range("index OOB"); },
        []{ throw std::runtime_error("runtime problem"); },
        []{ throw std::logic_error("logic bug"); }
    };

    for (auto& fn : throwers) {
        try {
            fn();
        } catch (const std::logic_error& e) {
            // Catches invalid_argument and out_of_range (both derive from logic_error)
            std::cerr << "  [logic_error]   " << e.what() << "\n";
        } catch (const std::runtime_error& e) {
            std::cerr << "  [runtime_error] " << e.what() << "\n";
        }
    }

    // Catching std::exception catches everything from the standard hierarchy
    try {
        throw std::out_of_range("caught as std::exception");
    } catch (const std::exception& e) {
        std::cerr << "  [std::exception] " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 3: Custom Exception Class
// =============================================================================
//
// Inherit from std::exception (or a more specific standard class) and override
// what().  Store the message in a std::string; what() returns a C-string.
// The noexcept on what() is required by std::exception's contract.

class DatabaseError : public std::runtime_error {
public:
    explicit DatabaseError(const std::string& msg, int error_code = 0)
        : std::runtime_error(msg)
        , error_code_(error_code)
    {}

    int error_code() const noexcept { return error_code_; }

    // what() is inherited from std::runtime_error — no need to override

private:
    int error_code_;
};

class ConnectionError : public DatabaseError {
public:
    explicit ConnectionError(const std::string& host)
        : DatabaseError("Cannot connect to: " + host, 503)
        , host_(host)
    {}

    const std::string& host() const noexcept { return host_; }

private:
    std::string host_;
};

void demo_custom_exceptions() {
    std::cout << "\n--- Section 3: Custom Exception Classes ---\n";

    try {
        throw ConnectionError("db.example.com");
    } catch (const ConnectionError& e) {
        // Most-derived handler: gets full detail
        std::cerr << "  [ConnectionError] host=" << e.host()
                  << " code=" << e.error_code()
                  << " what=" << e.what() << "\n";
    } catch (const DatabaseError& e) {
        // Would catch other DatabaseErrors but not ConnectionError (already caught)
        std::cerr << "  [DatabaseError] code=" << e.error_code()
                  << " what=" << e.what() << "\n";
    }

    try {
        throw DatabaseError("query timeout", 408);
    } catch (const DatabaseError& e) {
        std::cerr << "  [DatabaseError] code=" << e.error_code()
                  << " what=" << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 4: Catch-All and Re-throwing
// =============================================================================
//
// catch(...) catches any exception including non-std ones (e.g., integer throw).
// Use it as a last resort to log-and-rethrow or to prevent termination.
//
// 'throw;' (with no argument) re-throws the current exception preserving its
// full type — NEVER write 'throw e;' in a catch block because that would slice
// the exception to the catch type.

void inner() {
    throw ConnectionError("inner-host");
}

void middle() {
    try {
        inner();
    } catch (...) {
        std::cerr << "  [middle] caught something, re-throwing...\n";
        throw;   // re-throw: type ConnectionError is preserved
    }
}

void demo_catch_all_rethrow() {
    std::cout << "\n--- Section 4: catch(...) and Re-throw ---\n";

    try {
        middle();
    } catch (const ConnectionError& e) {
        // Type is preserved despite intermediate catch(...)
        std::cerr << "  [outer] ConnectionError: " << e.what() << "\n";
    }

    // catch-all with a non-std throw
    try {
        throw 42;   // throwing an int — bizarre but legal
    } catch (...) {
        std::cerr << "  caught non-std exception (int 42)\n";
    }
}

// =============================================================================
// SECTION 5: noexcept Specifier and std::terminate
// =============================================================================
//
// noexcept(true): declares the function will not throw.  If it does, the
// runtime calls std::terminate() immediately — no stack unwinding.
// noexcept is important for move operations (see exercise 20); the STL uses
// it to decide whether to move or copy during reallocation.
//
// noexcept(expr): conditionally noexcept based on a compile-time boolean.

void safe_print(const std::string& s) noexcept {
    // Guaranteed not to throw; if an internal operation throws, terminate() fires
    std::cout << "  safe_print: " << s << "\n";
}

// noexcept propagation
template <typename T>
void wrapper(T& obj) noexcept(noexcept(obj.do_something())) {
    obj.do_something();
}

void demo_noexcept() {
    std::cout << "\n--- Section 5: noexcept ---\n";

    safe_print("hello noexcept");

    // Check at compile time whether a function is noexcept
    std::cout << "safe_print is noexcept? "
              << (noexcept(safe_print(std::string{})) ? "yes" : "no") << "\n";

    auto lambda = []() noexcept { return 42; };
    std::cout << "lambda is noexcept? "
              << (noexcept(lambda()) ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 6: Exception Safety Guarantees
// =============================================================================
//
// No-throw guarantee: the function never throws (noexcept).
// Strong guarantee:   if the function throws, the program state is unchanged
//                     (commit-or-rollback semantics).  Often achieved via
//                     copy-and-swap.
// Basic guarantee:    if the function throws, the program is in a valid (but
//                     possibly different) state; no resources are leaked.
// No guarantee:       the function may leave the program in an inconsistent
//                     state — this is bad and should be avoided.

class BankAccount {
public:
    explicit BankAccount(double balance) : balance_(balance) {}

    // Basic guarantee: if withdraw throws, balance may have been modified —
    // but the object is still valid and no resources are leaked.
    void withdraw_basic(double amount) {
        if (amount <= 0) throw std::invalid_argument("amount must be positive");
        if (amount > balance_) throw std::runtime_error("insufficient funds");
        balance_ -= amount;
    }

    // Strong guarantee: copy-and-swap ensures all-or-nothing semantics.
    void transfer_strong(BankAccount& other, double amount) {
        // Work on copies first; only commit if no exception
        double new_this  = balance_  - amount;
        double new_other = other.balance_ + amount;

        if (new_this < 0) throw std::runtime_error("insufficient funds");

        // Both writes are noexcept — commit phase
        balance_       = new_this;
        other.balance_ = new_other;
    }

    double balance() const noexcept { return balance_; }

private:
    double balance_;
};

void demo_exception_safety() {
    std::cout << "\n--- Section 6: Exception Safety Guarantees ---\n";

    BankAccount acc(100.0);

    // Basic guarantee demo
    try {
        acc.withdraw_basic(-10);
    } catch (const std::invalid_argument& e) {
        std::cerr << "  basic: " << e.what() << "\n";
    }
    std::cout << "  balance after failed withdraw: " << acc.balance() << "\n";

    // Strong guarantee demo
    BankAccount alice(500.0), bob(100.0);
    try {
        alice.transfer_strong(bob, 200.0);
        std::cout << "  transfer ok: alice=" << alice.balance()
                  << " bob=" << bob.balance() << "\n";
    } catch (...) {}

    try {
        alice.transfer_strong(bob, 10000.0);  // will throw
    } catch (const std::runtime_error& e) {
        std::cerr << "  strong: " << e.what() << "\n";
        std::cout << "  after failed transfer: alice=" << alice.balance()
                  << " bob=" << bob.balance() << " (unchanged)\n";
    }
}

// =============================================================================
// SECTION 7: RAII Ensures Cleanup Even on Exception
// =============================================================================
//
// RAII (Resource Acquisition Is Initialisation) ties resource lifetime to
// object lifetime.  When an exception unwinds the stack, destructors still
// run — so RAII objects release their resources even when an exception occurs.
// This is why std::unique_ptr and std::lock_guard are preferred over raw new/delete.

class ManagedFile {
public:
    explicit ManagedFile(const std::string& name) : name_(name) {
        std::cout << "  [ManagedFile] opened:  " << name_ << "\n";
    }
    ~ManagedFile() {
        std::cout << "  [ManagedFile] closed:  " << name_ << "\n";
    }
    void write(const std::string& data) {
        std::cout << "  [ManagedFile] writing: " << data << "\n";
    }
    ManagedFile(const ManagedFile&)            = delete;
    ManagedFile& operator=(const ManagedFile&) = delete;

private:
    std::string name_;
};

void demo_raii_exception() {
    std::cout << "\n--- Section 7: RAII + Exceptions ---\n";

    try {
        ManagedFile f("report.txt");
        f.write("header");
        throw std::runtime_error("disk full");  // exception thrown mid-function
        f.write("footer");                       // never reached
    } catch (const std::runtime_error& e) {
        std::cerr << "  caught: " << e.what() << "\n";
    }
    // ManagedFile destructor ran despite the exception — file is "closed"

    // smart pointer example
    try {
        auto buf = std::make_unique<int[]>(1024);  // allocated
        throw std::logic_error("something went wrong");
        // unique_ptr destructor runs on unwind — memory is freed
    } catch (const std::logic_error& e) {
        std::cerr << "  caught: " << e.what() << " (smart ptr freed memory)\n";
    }
}

// =============================================================================
// SECTION 8: std::exception_ptr and std::current_exception (Brief)
// =============================================================================
//
// exception_ptr is a shared-ownership handle to an exception object.
// It allows you to capture an exception in one thread and rethrow it in another,
// or store it for later inspection.

void demo_exception_ptr() {
    std::cout << "\n--- Section 8: std::exception_ptr ---\n";

    std::exception_ptr ep;

    // Capture an exception without immediately rethrowing
    try {
        throw std::runtime_error("captured exception");
    } catch (...) {
        ep = std::current_exception();
        std::cout << "  exception captured (not rethrown yet)\n";
    }

    // Later, rethrow and handle it
    if (ep) {
        try {
            std::rethrow_exception(ep);
        } catch (const std::runtime_error& e) {
            std::cerr << "  rethrown: " << e.what() << "\n";
        }
    }

    std::cout << "  (useful for passing exceptions across thread boundaries)\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 17: Exceptions ===\n";

    demo_basic();
    demo_hierarchy();
    demo_custom_exceptions();
    demo_catch_all_rethrow();
    demo_noexcept();
    demo_exception_safety();
    demo_raii_exception();
    demo_exception_ptr();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Always catch by 'const Type&' — never by value (slicing)\n"
              << "   or by pointer.\n";
    std::cout << "2. Order catch handlers from most-derived to least-derived;\n"
              << "   catching std::exception first swallows everything.\n";
    std::cout << "3. Use 'throw;' (no argument) to re-throw — never 'throw e;'\n"
              << "   because that slices the exception.\n";
    std::cout << "4. Mark move constructors and destructors noexcept so the STL\n"
              << "   can use move semantics during reallocation.\n";
    std::cout << "5. RAII guarantees cleanup because destructors always run\n"
              << "   during stack unwinding, even on exceptions.\n";

    return 0;
}

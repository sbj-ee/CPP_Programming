// functions.cpp - Exercise 04: Functions
// Demonstrates: declaration vs definition, overloading, default args,
//               pass by value/ref/const-ref, return by value/ref,
//               inline functions, recursion

#include <iostream>
#include <string>
#include <vector>
#include <cmath>    // std::sqrt

// ── Section 1: Declaration vs Definition ─────────────────────────────────────

// Forward declaration (prototype): tells compiler the signature.
// Definition (body) can appear later in the file or in another .cpp.
double circle_area(double radius);          // declaration
double circle_perimeter(double radius);     // declaration

// ── Section 2: Function Overloading ──────────────────────────────────────────

// Same name, different parameter types/count.
// The compiler picks the right version at compile time.
void print_value(int    v) { std::cout << "int:    " << v << "\n"; }
void print_value(double v) { std::cout << "double: " << v << "\n"; }
void print_value(const std::string& v) { std::cout << "string: " << v << "\n"; }
void print_value(bool   v) { std::cout << "bool:   " << std::boolalpha << v << "\n"; }

// Overloading on parameter count
int  add(int a, int b)           { return a + b; }
int  add(int a, int b, int c)    { return a + b + c; }

// ── Section 3: Default Arguments ─────────────────────────────────────────────

// Defaults must appear in the declaration (or in the definition if only one).
// Defaults must be at the right side (once a default, all to the right must have defaults).
void greet(const std::string& name, const std::string& greeting = "Hello")
{
    std::cout << greeting << ", " << name << "!\n";
}

double power(double base, int exponent = 2)
{
    double result = 1.0;
    for (int i = 0; i < exponent; ++i)
        result *= base;
    return result;
}

// ── Section 4: Pass by value, by reference, by const-reference ───────────────

// Pass by value: function gets a copy; caller's variable is unchanged.
void increment_by_value(int x)
{
    x += 1;    // modifies local copy only
}

// Pass by reference: function can modify caller's variable.
void increment_by_ref(int& x)
{
    x += 1;    // modifies the original
}

// Pass by const reference: read-only access to caller's variable; no copy made.
// Preferred for large objects like std::string.
std::string to_upper(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (char c : s)
        result += static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c);
    return result;
}

// Pass a vector by reference to modify it in place
void double_elements(std::vector<int>& v)
{
    for (int& elem : v)
        elem *= 2;
}

// ── Section 5: Return by value vs. return by reference ───────────────────────

// Return by value: safe; returns a copy (or moves).
// RVO (Return Value Optimisation) often eliminates the copy.
std::string build_message(const std::string& who)
{
    return "Hello from " + who;   // returned by value (RVO applies)
}

// Return by reference: returns alias into some existing object.
// DANGER: never return a reference to a local variable (dangling ref).
// Safe when returning a reference to a member or parameter that outlives the call.
int& get_element(std::vector<int>& v, std::size_t idx)
{
    return v[idx];   // reference into the vector (valid; vector outlives call)
}

// ── Section 6: Inline functions ───────────────────────────────────────────────

// inline hints to the compiler to expand the call in place (no function call
// overhead). Modern compilers inline aggressively regardless of the keyword.
// Typically defined in headers so the definition is visible at call sites.
inline int square(int x) { return x * x; }
inline double clamp(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// ── Section 7: Recursion ──────────────────────────────────────────────────────

// Recursive factorial: n! = n * (n-1)!
long long factorial(int n)
{
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Recursive fibonacci (naive: exponential time, illustrative only)
int fibonacci(int n)
{
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Iterative fibonacci for comparison
long long fibonacci_iter(int n)
{
    if (n <= 1) return n;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        long long tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

// ── Definitions for forward-declared functions ─────────────────────────────

double circle_area(double radius)
{
    constexpr double PI = 3.14159265358979;
    return PI * radius * radius;
}

double circle_perimeter(double radius)
{
    constexpr double PI = 3.14159265358979;
    return 2.0 * PI * radius;
}

// ── main ───────────────────────────────────────────────────────────────────────

int main()
{
    // Section 1: Declaration vs Definition
    std::cout << "=== Section 1: Declaration vs Definition ===\n";
    std::cout << "circle_area(5.0)      = " << circle_area(5.0)      << "\n";
    std::cout << "circle_perimeter(5.0) = " << circle_perimeter(5.0) << "\n";

    // Section 2: Overloading
    std::cout << "\n=== Section 2: Function Overloading ===\n";
    print_value(42);
    print_value(3.14);
    print_value(std::string("hello"));
    print_value(true);
    std::cout << "add(1,2)   = " << add(1, 2)    << "\n";
    std::cout << "add(1,2,3) = " << add(1, 2, 3) << "\n";

    // Section 3: Default arguments
    std::cout << "\n=== Section 3: Default Arguments ===\n";
    greet("Alice");                    // uses default "Hello"
    greet("Bob", "Good morning");      // overrides default
    std::cout << "power(3)    = " << power(3)    << "\n";  // exponent defaults to 2
    std::cout << "power(2,10) = " << power(2,10) << "\n";

    // Section 4: Pass by value / reference / const-reference
    std::cout << "\n=== Section 4: Pass by Value / Reference / Const-Ref ===\n";
    int val = 10;
    increment_by_value(val);
    std::cout << "After increment_by_value(10): val = " << val << "  (unchanged)\n";
    increment_by_ref(val);
    std::cout << "After increment_by_ref(10):   val = " << val << "  (changed)\n";

    std::string word = "hello world";
    std::cout << "to_upper(\"" << word << "\") = \"" << to_upper(word) << "\"\n";

    std::vector<int> nums = {1, 2, 3, 4, 5};
    double_elements(nums);
    std::cout << "double_elements: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\n";

    // Section 5: Return by value vs. reference
    std::cout << "\n=== Section 5: Return by Value vs. Reference ===\n";
    std::string msg = build_message("main");
    std::cout << msg << "\n";

    std::vector<int> data = {10, 20, 30, 40};
    get_element(data, 2) = 999;   // assign through returned reference
    std::cout << "After get_element(data,2)=999: data[2]=" << data[2] << "\n";

    // Section 6: Inline functions
    std::cout << "\n=== Section 6: Inline Functions ===\n";
    std::cout << "square(7)       = " << square(7)               << "\n";
    std::cout << "clamp(1.5,0,1)  = " << clamp(1.5, 0.0, 1.0)   << "\n";
    std::cout << "clamp(-0.5,0,1) = " << clamp(-0.5, 0.0, 1.0)  << "\n";

    // Section 7: Recursion
    std::cout << "\n=== Section 7: Recursion ===\n";
    for (int i = 0; i <= 10; ++i)
        std::cout << i << "! = " << factorial(i) << "\n";

    std::cout << "\nFibonacci (recursive vs iterative):\n";
    for (int i = 0; i <= 12; ++i)
        std::cout << "  fib(" << i << ") = " << fibonacci(i)
                  << "  iter=" << fibonacci_iter(i) << "\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Forward declarations allow functions to be defined in any order.\n";
    std::cout << "  - Overloads must differ in parameter types/count, not return type.\n";
    std::cout << "  - Default args must appear from right to left.\n";
    std::cout << "  - Prefer const-ref for large objects; value for primitives.\n";
    std::cout << "  - Never return a reference to a local variable (dangling ref).\n";
    std::cout << "  - Recursive fibonacci is O(2^n); prefer iterative for large n.\n";

    return 0;
}

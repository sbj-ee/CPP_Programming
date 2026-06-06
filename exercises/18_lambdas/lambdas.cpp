// =============================================================================
// Exercise 18: Lambdas
// =============================================================================
// Topics: lambda syntax, capture modes, mutable lambda, generic lambda (C++14),
//         std::function, lambdas with STL, IILE, returning lambdas
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g lambdas.cpp -o lambdas
// =============================================================================

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <numeric>

// =============================================================================
// SECTION 1: Lambda Syntax
// =============================================================================
//
// Full form:  [capture-list](parameter-list) -> return-type { body }
//
// Simplified forms:
//   [captures](params) { body }  — return type deduced
//   [captures] { body }          — no parameters
//   [=] / [&]                    — default captures
//
// A lambda is syntactic sugar for an anonymous class with an operator().
// The capture list becomes the class's data members.

void demo_syntax() {
    std::cout << "\n--- Section 1: Lambda Syntax ---\n";

    // Simplest lambda: no capture, no parameters
    auto greet = []{ std::cout << "  Hello from a lambda!\n"; };
    greet();

    // With parameters and explicit return type
    auto add = [](int a, int b) -> int { return a + b; };
    std::cout << "  add(3, 4) = " << add(3, 4) << "\n";

    // Return type deduced (no -> needed for simple bodies)
    auto multiply = [](double x, double y) { return x * y; };
    std::cout << "  multiply(2.5, 4.0) = " << multiply(2.5, 4.0) << "\n";

    // Assigned to auto — the lambda's unique closure type
    auto is_even = [](int n) { return n % 2 == 0; };
    std::cout << "  is_even(6) = " << (is_even(6) ? "yes" : "no") << "\n";
    std::cout << "  is_even(7) = " << (is_even(7) ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 2: Capture Modes
// =============================================================================
//
// []       — capture nothing (error if body references local variables)
// [=]      — capture all locals by VALUE (snapshot at lambda creation)
// [&]      — capture all locals by REFERENCE (be careful of dangling refs)
// [x]      — capture specific variable x by value
// [&x]     — capture specific variable x by reference
// [=, &x]  — capture all by value, except x by reference
// [this]   — capture the 'this' pointer (for member lambdas)
// [*this]  — capture a COPY of the object (C++17, avoids dangling)

void demo_captures() {
    std::cout << "\n--- Section 2: Capture Modes ---\n";

    int base   = 10;
    int factor = 3;

    // [=] — value copy: changes to base/factor after lambda creation not seen
    auto by_value = [=](int x) { return base + factor * x; };
    base = 99;  // this change is NOT seen by by_value
    std::cout << "  [=] base=10,factor=3 captured: by_value(2) = " << by_value(2) << "\n";
    base = 10;  // restore

    // [&] — reference: sees live value of base
    auto by_ref = [&](int x) { return base + factor * x; };
    base = 20;
    std::cout << "  [&] after base=20:             by_ref(2) = " << by_ref(2) << "\n";
    base = 10;

    // [x] — capture only 'base' by value
    auto specific = [base](int x) { return base * x; };
    std::cout << "  [base] specific(5) = " << specific(5) << "\n";

    // [&x] — capture only 'base' by reference
    int counter = 0;
    auto increment = [&counter]{ ++counter; };
    increment(); increment(); increment();
    std::cout << "  [&counter] after 3 calls: counter = " << counter << "\n";

    // Mixed: capture all by value except counter by reference
    int offset = 100;
    auto mixed = [=, &counter]{ counter += offset; };
    mixed();
    std::cout << "  [=, &counter] counter = " << counter << "\n";
}

// =============================================================================
// SECTION 3: mutable Lambda
// =============================================================================
//
// By default, a lambda captures by-value as const — you cannot modify a
// captured copy.  Add 'mutable' to allow modification of the local copy.
// The modification does NOT affect the original variable.

void demo_mutable() {
    std::cout << "\n--- Section 3: mutable Lambda ---\n";

    int count = 0;

    // Without mutable: cannot modify captured value (compile error if you try)
    // With mutable: captured copy is non-const; original 'count' is unaffected
    auto counter = [count]() mutable -> int {
        return ++count;   // modifies the lambda's own copy of count
    };

    std::cout << "  counter(): " << counter() << "\n";  // 1
    std::cout << "  counter(): " << counter() << "\n";  // 2
    std::cout << "  counter(): " << counter() << "\n";  // 3
    std::cout << "  original count = " << count << "  (unchanged)\n";

    // Stateful generator: sequential IDs
    int next_id = 1000;
    auto gen_id = [next_id]() mutable { return next_id++; };
    for (int i = 0; i < 4; ++i) std::cout << "  id=" << gen_id() << "\n";
}

// =============================================================================
// SECTION 4: Generic Lambda (C++14)
// =============================================================================
//
// Using 'auto' for parameters makes the lambda a template: the compiler
// generates a different operator() for each argument type used.

void demo_generic_lambda() {
    std::cout << "\n--- Section 4: Generic Lambda (C++14) ---\n";

    // auto parameter — works like a function template
    auto print_twice = [](auto x) {
        std::cout << "  " << x << " " << x << "\n";
    };

    print_twice(42);
    print_twice(3.14);
    print_twice(std::string("hello"));

    // Generic comparator
    auto less_by_abs = [](auto a, auto b) {
        return (a < 0 ? -a : a) < (b < 0 ? -b : b);
    };

    std::vector<int> v = {-3, 1, -7, 4, -2, 5};
    std::sort(v.begin(), v.end(), less_by_abs);
    std::cout << "  sorted by abs value: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";
}

// =============================================================================
// SECTION 5: std::function — Type-Erased Callable Storage
// =============================================================================
//
// std::function<R(Args...)> can hold ANY callable with matching signature:
// lambdas, function pointers, functors, bind expressions.
// It uses type erasure (heap allocation) so there is overhead compared to
// a direct lambda stored in 'auto'.  Prefer 'auto' when the callable won't
// be stored polymorphically.

int regular_function(int x) { return x * 2; }

struct Doubler {
    int operator()(int x) const { return x * 2; }
};

void demo_std_function() {
    std::cout << "\n--- Section 5: std::function ---\n";

    // Uniform storage for different callable types
    std::vector<std::function<int(int)>> transforms;
    transforms.push_back(regular_function);         // function pointer
    transforms.push_back(Doubler{});                // functor
    transforms.push_back([](int x){ return x+10; });// lambda

    int val = 5;
    std::cout << "  transforms on " << val << ":\n";
    for (const auto& f : transforms) {
        std::cout << "    -> " << f(val) << "\n";
    }

    // std::function in a higher-order function
    auto apply_twice = [](std::function<int(int)> f, int x) {
        return f(f(x));
    };
    auto triple = [](int x){ return x * 3; };
    std::cout << "  apply_twice(triple, 2) = " << apply_twice(triple, 2) << "\n";

    // Storing a capturing lambda (impossible with raw function pointer)
    int offset = 7;
    std::function<int(int)> add_offset = [offset](int x){ return x + offset; };
    std::cout << "  add_offset(10) = " << add_offset(10) << "\n";
}

// =============================================================================
// SECTION 6: Lambdas with STL Algorithms
// =============================================================================
//
// Lambdas are the primary way to pass custom comparators and predicates to
// STL algorithms.  They replace verbose named functors.

void demo_stl_lambdas() {
    std::cout << "\n--- Section 6: Lambdas with STL ---\n";

    std::vector<int> v = {5, 3, 8, 1, 9, 2, 7, 4, 6};

    // sort with lambda comparator (descending)
    std::sort(v.begin(), v.end(), [](int a, int b){ return a > b; });
    std::cout << "  sorted desc: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    // find_if: first element > 7
    auto it = std::find_if(v.begin(), v.end(), [](int x){ return x > 7; });
    if (it != v.end()) std::cout << "  first > 7: " << *it << "\n";

    // transform: square each element
    std::vector<int> squares;
    std::transform(v.begin(), v.end(), std::back_inserter(squares),
                   [](int x){ return x * x; });
    std::cout << "  squares:    ";
    for (int x : squares) std::cout << x << " ";
    std::cout << "\n";

    // accumulate with lambda
    int sum = std::accumulate(v.begin(), v.end(), 0,
                              [](int acc, int x){ return acc + x; });
    std::cout << "  sum = " << sum << "\n";
}

// =============================================================================
// SECTION 7: Immediately-Invoked Lambda Expression (IILE)
// =============================================================================
//
// An IILE is a lambda that is defined and called in one expression.
// Useful for complex initialisation of a const variable without a named helper.

void demo_iile() {
    std::cout << "\n--- Section 7: Immediately-Invoked Lambda (IILE) ---\n";

    // Initialise a const from complex logic without a helper function
    const std::string greeting = []{
        std::string s;
        s += "Hello";
        s += ", ";
        s += "World";
        s += "!";
        return s;
    }();
    std::cout << "  " << greeting << "\n";

    // IILE with parameters (unusual but valid)
    const int factorial_5 = [](int n) -> int {
        int result = 1;
        for (int i = 2; i <= n; ++i) result *= i;
        return result;
    }(5);
    std::cout << "  5! = " << factorial_5 << "\n";

    // IILE inside a larger expression
    std::cout << "  computed: " << [](int a, int b){ return a * b + 1; }(6, 7)
              << "\n";
}

// =============================================================================
// SECTION 8: Returning Lambdas from Functions
// =============================================================================
//
// A function can return a lambda — either via 'auto' (preserves exact type,
// no overhead) or via std::function (type-erased, more flexible but slower).
// The lambda must not capture local variables by reference if it outlives them.

auto make_adder(int n) {
    return [n](int x) { return x + n; };   // captures n by value — safe
}

std::function<int(int)> make_multiplier(int factor) {
    return [factor](int x) { return x * factor; };
}

auto make_counter(int start = 0) {
    return [count = start]() mutable { return count++; };
}

void demo_returning_lambdas() {
    std::cout << "\n--- Section 8: Returning Lambdas ---\n";

    auto add5 = make_adder(5);
    auto add10 = make_adder(10);
    std::cout << "  add5(3)  = " << add5(3)  << "\n";
    std::cout << "  add10(3) = " << add10(3) << "\n";

    auto triple = make_multiplier(3);
    std::cout << "  triple(7) = " << triple(7) << "\n";

    auto cnt = make_counter(100);
    std::cout << "  counter: " << cnt() << " " << cnt() << " " << cnt() << "\n";

    // Compose two returned lambdas
    auto add_then_triple = [add5, triple](int x) { return triple(add5(x)); };
    std::cout << "  add5 then triple(4) = " << add_then_triple(4) << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 18: Lambdas ===\n";

    demo_syntax();
    demo_captures();
    demo_mutable();
    demo_generic_lambda();
    demo_std_function();
    demo_stl_lambdas();
    demo_iile();
    demo_returning_lambdas();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. [&] is dangerous if the lambda outlives the enclosing\n"
              << "   scope (e.g., stored in a std::function returned from\n"
              << "   a function) — prefer [=] or explicit [x] captures.\n";
    std::cout << "2. 'mutable' lets you modify a by-value captured copy but\n"
              << "   does NOT modify the original variable.\n";
    std::cout << "3. std::function has overhead (heap alloc, virtual dispatch);\n"
              << "   prefer 'auto' when the callable type is known statically.\n";
    std::cout << "4. Generic lambdas (auto params) generate template operator()\n"
              << "   — the compiler instantiates a separate version per type.\n";
    std::cout << "5. IILEs are a clean way to initialise const variables from\n"
              << "   complex logic without polluting the surrounding scope.\n";

    return 0;
}

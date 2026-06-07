// =============================================================================
// Exercise 13: Templates
// =============================================================================
// Topics: Function templates, class templates, template specialisation,
//         non-type template parameters, typename vs class, CTAD (Class Template Argument Deduction, C++17),
//         variadic templates and fold expressions
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g templates.cpp -o templates
// =============================================================================

#include <iostream>
#include <cassert>
#include <bitset>
#include <stdexcept>
#include <string>
#include <array>

// =============================================================================
// SECTION 1: Function Templates
// =============================================================================
//
// A function template describes a family of functions.  The compiler
// instantiates a concrete version for each distinct set of template arguments
// it encounters.  You may use 'typename' or 'class' — they are identical in a
// template parameter list; 'typename' is more common in modern code.

template <typename T>
T my_min(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template <typename T>
void my_swap(T& a, T& b) {
    T tmp = std::move(a);
    a     = std::move(b);
    b     = std::move(tmp);
}

// Non-type template parameter N: size known at compile time
template <typename T, std::size_t N>
void print_array(const T (&arr)[N], const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (std::size_t i = 0; i < N; ++i) {
        std::cout << arr[i];
        if (i + 1 < N) std::cout << ", ";
    }
    std::cout << " ]\n";
}

void demo_function_templates() {
    std::cout << "\n--- Section 1: Function Templates ---\n";

    int a = 7, b = 3;
    std::cout << "my_min(7, 3)   = " << my_min(a, b) << "\n";
    std::cout << "my_min(2.5,1.1)= " << my_min(2.5, 1.1) << "\n";
    std::cout << "my_min(\"cat\",\"ant\") = " << my_min(std::string("cat"), std::string("ant")) << "\n";

    int x = 10, y = 20;
    std::cout << "Before swap: x=" << x << " y=" << y << "\n";
    my_swap(x, y);
    std::cout << "After swap:  x=" << x << " y=" << y << "\n";

    int ints[]    = {5, 1, 3, 2, 4};
    double dbls[] = {1.1, 2.2, 3.3};
    print_array(ints, "ints");
    print_array(dbls, "doubles");
}

// =============================================================================
// SECTION 2: Class Template — Stack<T>
// =============================================================================
//
// Member functions defined outside the class body must repeat the template
// parameter list.  The class name inside the body is Stack (without <T>)
// because the compiler already knows the T context.

template <typename T>
class Stack {
public:
    void push(const T& value) {
        if (top_ >= MAX) throw std::overflow_error("Stack overflow");
        data_[top_++] = value;
    }

    void pop() {
        if (empty()) throw std::underflow_error("Stack underflow");
        --top_;
    }

    const T& top() const {
        if (empty()) throw std::underflow_error("Stack is empty");
        return data_[top_ - 1];
    }

    bool   empty() const { return top_ == 0; }
    size_t size()  const { return top_; }

private:
    static constexpr std::size_t MAX = 32;
    T      data_[MAX] {};
    size_t top_ = 0;
};

void demo_class_template() {
    std::cout << "\n--- Section 2: Class Template Stack<T> ---\n";

    Stack<int> si;
    si.push(10); si.push(20); si.push(30);
    std::cout << "Stack<int> top: " << si.top() << "  size: " << si.size() << "\n";
    si.pop();
    std::cout << "After pop, top: " << si.top() << "\n";

    Stack<std::string> ss;
    ss.push("alpha"); ss.push("beta");
    std::cout << "Stack<string> top: " << ss.top() << "\n";

    try {
        Stack<int> empty_stack;
        empty_stack.top();  // throws
    } catch (const std::underflow_error& e) {
        std::cout << "Caught underflow: " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 3: Template Specialisation — Stack<bool>
// =============================================================================
//
// A full specialisation provides an entirely different implementation for
// a specific type.  Here we pack booleans into a std::bitset to save memory.
// (A partial specialisation would specialise on a subset of parameters.)

template <>
class Stack<bool> {
public:
    void push(bool value) {
        if (top_ >= MAX) throw std::overflow_error("Stack<bool> overflow");
        bits_.set(top_++, value);
    }
    void pop() {
        if (empty()) throw std::underflow_error("Stack<bool> underflow");
        --top_;
    }
    bool   top()   const { return bits_[top_ - 1]; }
    bool   empty() const { return top_ == 0; }
    size_t size()  const { return top_; }

private:
    static constexpr std::size_t MAX = 64;
    std::bitset<64> bits_;
    std::size_t     top_ = 0;
};

void demo_specialisation() {
    std::cout << "\n--- Section 3: Template Specialisation Stack<bool> ---\n";

    Stack<bool> sb;
    sb.push(true); sb.push(false); sb.push(true);
    std::cout << "Stack<bool> size=" << sb.size()
              << " top=" << (sb.top() ? "true" : "false") << "\n";
    sb.pop();
    std::cout << "After pop, top=" << (sb.top() ? "true" : "false") << "\n";
    std::cout << "(bool bits stored compactly in a std::bitset<64>)\n";
}

// =============================================================================
// SECTION 4: Non-Type Template Parameters — Fixed-size Array
// =============================================================================
//
// Non-type parameters can be integral constants, pointers, or (C++20) floats.
// std::array<T,N> is the canonical example; here we build a minimal analogue.

template <typename T, std::size_t N>
class FixedArray {
public:
    constexpr std::size_t size() const { return N; }

    T& operator[](std::size_t i) {
        if (i >= N) throw std::out_of_range("FixedArray index out of range");
        return data_[i];
    }
    const T& operator[](std::size_t i) const {
        if (i >= N) throw std::out_of_range("FixedArray index out of range");
        return data_[i];
    }

    T*       begin()       { return data_; }
    T*       end()         { return data_ + N; }
    const T* begin() const { return data_; }
    const T* end()   const { return data_ + N; }

private:
    T data_[N] {};
};

void demo_nontype_params() {
    std::cout << "\n--- Section 4: Non-Type Template Parameters ---\n";

    FixedArray<int, 5> fa;
    for (std::size_t i = 0; i < fa.size(); ++i) fa[i] = static_cast<int>(i * i);

    std::cout << "FixedArray<int,5>: ";
    for (int v : fa) std::cout << v << " ";
    std::cout << "\n";

    // std::array is the standard equivalent
    std::array<double, 3> sa = {1.1, 2.2, 3.3};
    std::cout << "std::array<double,3>: ";
    for (double d : sa) std::cout << d << " ";
    std::cout << "\n";
}

// =============================================================================
// SECTION 5: typename vs class, and CTAD (C++17)
// =============================================================================
//
// In template parameter lists 'typename' and 'class' are identical.
// 'typename' is also required to disambiguate dependent names:
//   typename Container::iterator it = ...;  // 'typename' needed here
//
// Class Template Argument Deduction (CTAD, C++17): The compiler can deduce
// template arguments from constructor arguments, just like function templates.

template <class T>           // 'class' works exactly like 'typename' here
T square(T v) { return v * v; }

// Deduction guide (optional): lets the compiler deduce Stack<T> from push args.
// For built-in templates like std::vector you get this automatically.

void demo_typename_ctad() {
    std::cout << "\n--- Section 5: typename vs class, CTAD ---\n";

    // 'class' in template parameter — no difference from 'typename'
    std::cout << "square(5)   = " << square(5)   << "\n";
    std::cout << "square(3.0) = " << square(3.0) << "\n";

    // CTAD with std::array (C++17): no need to write std::array<int,3>
    std::array arr = {10, 20, 30};  // deduced as std::array<int,3>
    std::cout << "CTAD std::array: ";
    for (int v : arr) std::cout << v << " ";
    std::cout << "\n";

    // CTAD with std::pair
    std::pair p = {std::string("key"), 42};  // deduced as std::pair<string,int>
    std::cout << "CTAD std::pair: " << p.first << " -> " << p.second << "\n";
}

// =============================================================================
// SECTION 6: Variadic Templates and Fold Expressions (C++17)
// =============================================================================
//
// A variadic template accepts zero or more type/value parameters via '...'.
// 'sizeof...(args)' gives the pack size.
//
// C++17 fold expressions compactly expand a pack with an operator:
//   (args + ...)   — right fold:  arg0 + (arg1 + (arg2 + ...))
//   (... + args)   — left fold:   ((arg0 + arg1) + arg2) + ...
//   (args + ... + 0) — binary right fold with initial value

template <typename... Args>
void variadic_print(Args&&... args) {
    // Fold expression: expands to (std::cout << arg0 << " " << arg1 << " " ...)
    ((std::cout << args << " "), ...);
    std::cout << "\n";
}

template <typename... Args>
auto variadic_sum(Args... args) {
    return (args + ...);   // unary right fold
}

template <typename... Args>
std::size_t variadic_count(Args&&...) {
    return sizeof...(Args);
}

void demo_variadic() {
    std::cout << "\n--- Section 6: Variadic Templates + Fold Expressions ---\n";

    variadic_print(1, 2.5, "hello", 'Z');
    std::cout << "sum(1,2,3,4,5) = " << variadic_sum(1, 2, 3, 4, 5) << "\n";
    std::cout << "count(a,b,c)   = " << variadic_count('a', 'b', 'c') << "\n";

    // sizeof... at compile time
    auto demo = [](auto... xs) {
        std::cout << "pack size = " << sizeof...(xs) << "  sum = " << (xs + ...) << "\n";
    };
    demo(10, 20, 30);
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 13: Templates ===\n";

    demo_function_templates();
    demo_class_template();
    demo_specialisation();
    demo_nontype_params();
    demo_typename_ctad();
    demo_variadic();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Template code lives in headers (or .tpp files); the\n"
              << "   compiler needs the full definition at instantiation time.\n";
    std::cout << "2. 'typename' and 'class' are identical in template param\n"
              << "   lists; but only 'typename' can appear in dependent-name\n"
              << "   disambiguation inside a template body.\n";
    std::cout << "3. Full specialisations must appear after the primary template\n"
              << "   and are often placed in .cpp files to avoid ODR violations.\n";
    std::cout << "4. CTAD (C++17) requires either a matching constructor or an\n"
              << "   explicit deduction guide.\n";
    std::cout << "5. Fold expressions require C++17.  For older code, use\n"
              << "   recursive variadic templates (base case + recursive case).\n";

    return 0;
}

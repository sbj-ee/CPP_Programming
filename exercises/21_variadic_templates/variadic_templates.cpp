// =============================================================================
// Exercise 21: Variadic Templates
// =============================================================================
// Topics: parameter packs, sizeof..., fold expressions (C++17), recursive
//         variadic style, std::tuple, std::index_sequence, variadic print,
//         type-in-pack check
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g variadic_templates.cpp -o variadic_templates
// =============================================================================

#include <iostream>
#include <tuple>
#include <string>
#include <type_traits>
#include <utility>
#include <typeinfo>
#include <functional>

// =============================================================================
// SECTION 1: Parameter Packs and sizeof...
// =============================================================================
//
// A variadic template accepts zero or more type (or value) arguments.
// 'typename... Ts' declares a TYPE parameter pack.
// 'Ts... args' declares a FUNCTION parameter pack matching the types.
// 'sizeof...(Ts)' or 'sizeof...(args)' expands to the pack size as a
// compile-time constant (std::size_t).

template <typename... Ts>
void show_pack_size() {
    std::cout << "  sizeof...(Ts) = " << sizeof...(Ts) << "\n";
}

template <typename... Args>
void show_arg_count(Args&&...) {
    std::cout << "  sizeof...(Args) = " << sizeof...(Args) << "\n";
}

void demo_pack_size() {
    std::cout << "\n--- Section 1: Parameter Packs and sizeof... ---\n";

    show_pack_size<>();                          // 0
    show_pack_size<int>();                       // 1
    show_pack_size<int, double, std::string>();  // 3

    show_arg_count();                    // 0
    show_arg_count(1, 2.0, "three");     // 3
}

// =============================================================================
// SECTION 2: Fold Expressions (C++17)
// =============================================================================
//
// Fold expressions compactly reduce a parameter pack with a binary operator.
//
// Unary right fold:  (args op ...) => arg0 op (arg1 op (arg2 op ...))
// Unary left fold:   (... op args) => ((arg0 op arg1) op arg2) op ...
// Binary right fold: (args op ... op init) => arg0 op (... op (argN op init))
// Binary left fold:  (init op ... op args) => ((init op arg0) op ...) op argN
//
// Supported operators: +, -, *, /, %, &, |, ^, &&, ||, <<, >>, ',', etc.

template <typename... Args>
auto fold_sum(Args... args) {
    return (args + ...);   // unary right fold: sum of all args
}

template <typename... Args>
auto fold_product(Args... args) {
    return (args * ...);   // unary right fold
}

template <typename... Args>
bool fold_all_positive(Args... args) {
    return ((args > 0) && ...);  // binary fold: short-circuits on false
}

template <typename... Args>
void fold_print(Args&&... args) {
    // Comma fold: evaluates each sub-expression in order
    ((std::cout << "  " << args << "\n"), ...);
}

// Binary right fold with initial value
template <typename... Args>
auto fold_sum_with_init(Args... args) {
    return (args + ... + 0);  // binary right fold; 0 handles empty pack
}

void demo_fold_expressions() {
    std::cout << "\n--- Section 2: Fold Expressions (C++17) ---\n";

    std::cout << "  fold_sum(1,2,3,4,5)       = " << fold_sum(1,2,3,4,5)       << "\n";
    std::cout << "  fold_product(1,2,3,4,5)   = " << fold_product(1,2,3,4,5)   << "\n";
    std::cout << "  fold_sum_with_init()       = " << fold_sum_with_init()       << "\n"; // 0
    std::cout << "  fold_all_positive(1,2,3)   = " << fold_all_positive(1,2,3)   << "\n"; // true
    std::cout << "  fold_all_positive(1,-1,3)  = " << fold_all_positive(1,-1,3)  << "\n"; // false

    std::cout << "  fold_print:\n";
    fold_print(42, 3.14, "hello");
}

// =============================================================================
// SECTION 3: Recursive Variadic (Pre-C++17 Style)
// =============================================================================
//
// Before fold expressions, variadic templates were expanded through recursion:
// a base case function handles 0 or 1 arguments; the recursive case peels
// one argument and recurses on the rest.

// Base case: single argument
template <typename T>
T recursive_sum(T t) {
    return t;
}

// Recursive case: head + sum(rest...)
template <typename T, typename... Rest>
T recursive_sum(T head, Rest... rest) {
    return head + recursive_sum(rest...);
}

// Print all (pre-C++17 style)
void recursive_print() {}   // base case: empty pack

template <typename T, typename... Rest>
void recursive_print(T head, Rest... rest) {
    std::cout << "  " << head << "\n";
    recursive_print(rest...);
}

void demo_recursive_variadic() {
    std::cout << "\n--- Section 3: Recursive Variadic (Pre-C++17) ---\n";

    std::cout << "  recursive_sum(1,2,3,4) = " << recursive_sum(1,2,3,4) << "\n";
    std::cout << "  recursive_sum(1.0,2.5) = " << recursive_sum(1.0,2.5) << "\n";

    std::cout << "  recursive_print:\n";
    recursive_print(10, "hello", 3.14);
}

// =============================================================================
// SECTION 4: std::tuple<Ts...>
// =============================================================================
//
// tuple is the canonical compile-time heterogeneous container.
// std::get<N>(t) — access by index (0-based); index must be a compile-time constant.
// std::get<T>(t) — access by type (C++14); fails if T appears more than once.
// std::apply(f, t) — call f with the elements of t as arguments (C++17).

void demo_tuple() {
    std::cout << "\n--- Section 4: std::tuple ---\n";

    // Construction
    auto t = std::make_tuple(42, 3.14, std::string("hello"), true);

    // Access by index
    std::cout << "  get<0>=" << std::get<0>(t) << "\n";   // int
    std::cout << "  get<1>=" << std::get<1>(t) << "\n";   // double
    std::cout << "  get<2>=" << std::get<2>(t) << "\n";   // string
    std::cout << "  get<3>=" << std::get<3>(t) << "\n";   // bool

    // Access by type
    std::cout << "  get<int>=    " << std::get<int>(t)    << "\n";
    std::cout << "  get<double>= " << std::get<double>(t) << "\n";

    // Structured binding (C++17)
    auto [n, d, s, b] = t;
    std::cout << "  structured: n=" << n << " d=" << d << " s=" << s << " b=" << b << "\n";

    // std::apply: call function with tuple elements as arguments
    auto sum_fn = [](int i, double x, const std::string&, bool) {
        return i + static_cast<int>(x);
    };
    std::cout << "  apply result: " << std::apply(sum_fn, t) << "\n";

    // tuple_size, tuple_element
    std::cout << "  tuple_size= " << std::tuple_size<decltype(t)>::value << "\n";

    // Concatenate tuples
    auto t2 = std::tuple_cat(std::make_tuple(1, 2), std::make_tuple(3.0, 4.0));
    std::cout << "  tuple_cat size=" << std::tuple_size<decltype(t2)>::value << "\n";
}

// =============================================================================
// SECTION 5: std::index_sequence — Compile-Time Index Iteration
// =============================================================================
//
// std::index_sequence<0,1,2,...,N-1> is used to iterate over tuple elements
// or parameter packs by index.  std::make_index_sequence<N> generates it.

// Print all tuple elements using an index pack
template <typename Tuple, std::size_t... Is>
void print_tuple_impl(const Tuple& t, std::index_sequence<Is...>) {
    // Fold expression over comma: prints each element
    ((std::cout << "  [" << Is << "]=" << std::get<Is>(t) << "\n"), ...);
}

template <typename... Ts>
void print_tuple(const std::tuple<Ts...>& t) {
    print_tuple_impl(t, std::make_index_sequence<sizeof...(Ts)>{});
}

// Convert tuple to array (all elements must be the same type)
template <typename T, std::size_t N, std::size_t... Is>
std::array<T, N> tuple_to_array_impl(const std::tuple<T, T, T>& t,
                                      std::index_sequence<Is...>) {
    return {std::get<Is>(t)...};
}

void demo_index_sequence() {
    std::cout << "\n--- Section 5: std::index_sequence ---\n";

    auto t = std::make_tuple(10, 3.14, std::string("cpp"), true);
    std::cout << "  Printing tuple via index_sequence:\n";
    print_tuple(t);

    // index_sequence size at compile time — 5 elements for N=5
    constexpr std::size_t seq_size =
        std::tuple_size<std::tuple<int,int,int,int,int>>::value;
    std::cout << "  make_index_sequence<5> would yield " << seq_size
              << " indices (0..4)\n";
}

// =============================================================================
// SECTION 6: Variadic Print with Fold
// =============================================================================
//
// A clean, production-quality variadic print function using fold expressions.

template <typename... Args>
void vprint(Args&&... args) {
    std::size_t idx = 0;
    ((std::cout << (idx++ ? ", " : "") << args), ...);
    std::cout << "\n";
}

template <typename... Args>
void vprintln(const std::string& sep, Args&&... args) {
    bool first = true;
    auto print_one = [&](auto&& a) {
        if (!first) std::cout << sep;
        std::cout << a;
        first = false;
    };
    (print_one(std::forward<Args>(args)), ...);
    std::cout << "\n";
}

void demo_variadic_print() {
    std::cout << "\n--- Section 6: Variadic Print ---\n";

    vprint(1, 2.5, "hello", 'Z', true);
    vprintln(" | ", "alpha", 42, 3.14);
    vprint();   // empty pack: just a newline
}

// =============================================================================
// SECTION 7: Type-in-Pack Check
// =============================================================================
//
// A common compile-time query: is type T present in a parameter pack Ts...?
// We use recursive specialisation or, in C++17, a fold with std::is_same.

// Primary: false by default
template <typename T, typename... Ts>
struct type_in_pack : std::false_type {};

// Recursive case: T matches the head
template <typename T, typename Head, typename... Tail>
struct type_in_pack<T, Head, Tail...>
    : std::conditional_t<std::is_same_v<T, Head>,
                         std::true_type,
                         type_in_pack<T, Tail...>>
{};

// C++17 fold version (one-liner)
template <typename T, typename... Ts>
constexpr bool contains_type = (std::is_same_v<T, Ts> || ...);

void demo_type_in_pack() {
    std::cout << "\n--- Section 7: Type-in-Pack Check ---\n";

    constexpr bool a = type_in_pack<int, float, int, double>::value;
    constexpr bool b = type_in_pack<char, float, int, double>::value;
    std::cout << "  int in <float,int,double>?  " << (a ? "yes" : "no") << "\n";
    std::cout << "  char in <float,int,double>? " << (b ? "yes" : "no") << "\n";

    constexpr bool c = contains_type<double, int, float, double>;
    constexpr bool d = contains_type<std::string, int, float, double>;
    std::cout << "  contains double?     " << (c ? "yes" : "no") << "\n";
    std::cout << "  contains string?     " << (d ? "yes" : "no") << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 21: Variadic Templates ===\n";

    demo_pack_size();
    demo_fold_expressions();
    demo_recursive_variadic();
    demo_tuple();
    demo_index_sequence();
    demo_variadic_print();
    demo_type_in_pack();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. sizeof...(pack) is a compile-time constant — not a function\n"
              << "   call; can be used in constant expressions.\n";
    std::cout << "2. Fold expressions require C++17.  For C++11/14 use recursive\n"
              << "   variadic templates with an explicit base case.\n";
    std::cout << "3. std::get<N> requires a compile-time N; for runtime indexing\n"
              << "   you need std::apply or index_sequence techniques.\n";
    std::cout << "4. Recursive variadic templates can cause deep instantiation;\n"
              << "   fold expressions are generally faster to compile.\n";
    std::cout << "5. Empty pack in a unary fold is only valid for &&, ||, and ,\n"
              << "   operators.  Use a binary fold with an identity value (e.g.,\n"
              << "   + ... + 0) to handle the empty case for arithmetic.\n";

    return 0;
}

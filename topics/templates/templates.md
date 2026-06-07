# Templates — Cheat Sheet

## Function Templates

```cpp
// Basic
template<typename T>
T min_val(T a, T b) { return a < b ? a : b; }

// Multiple type params
template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b) { return a + b; }  // C++11
template<typename T, typename U>
auto add(T a, U b) { return a + b; }   // C++14 (auto return)

// Explicit instantiation
int r = min_val<int>(3, 5);   // explicit
int s = min_val(3, 5);        // deduced: T = int
```

---

## Class Templates

```cpp
template<typename T>
class Stack {
public:
    void push(const T& val) { data_.push_back(val); }
    void push(T&& val)      { data_.push_back(std::move(val)); }
    void pop()              { data_.pop_back(); }
    T& top()                { return data_.back(); }
    const T& top() const    { return data_.back(); }
    bool empty() const      { return data_.empty(); }
    size_t size() const     { return data_.size(); }
private:
    std::vector<T> data_;
};

Stack<int>         si;
Stack<std::string> ss;
```

---

## Non-Type Template Parameters

```cpp
template<typename T, std::size_t N>
class Array {
public:
    T& operator[](std::size_t i) { return data_[i]; }
    constexpr std::size_t size() const { return N; }
private:
    T data_[N];
};

Array<int, 10> arr;   // 10-element array on stack; N is compile-time constant

// C++20: floating-point and class NTTPs allowed
template<double Scale>
double scaled(double x) { return x * Scale; }
```

---

## Template Specialisation

### Full Specialisation

```cpp
template<typename T>
class TypeName { public: static const char* name() { return "unknown"; } };

// Full specialisation for int
template<>
class TypeName<int> { public: static const char* name() { return "int"; } };

// Full specialisation for a function template
template<typename T>
bool equal(T a, T b) { return a == b; }

template<>
bool equal<double>(double a, double b) {
    return std::abs(a - b) < 1e-9;
}
```

### Partial Specialisation (class templates only)

```cpp
// Primary
template<typename T, typename U>
class Pair { };

// Partial: both types same
template<typename T>
class Pair<T, T> {
public:
    void info() { std::cout << "same types\n"; }
};

// Partial: second type is pointer
template<typename T, typename U>
class Pair<T, U*> { };
```

---

## `typename` vs `class`

```cpp
template<typename T>  // identical in template parameter list
template<class    T>  // both mean "any type"

// typename required to disambiguate dependent types:
template<typename T>
void f() {
    typename T::iterator it;    // "iterator" is a type inside T
    // Without typename: compiler assumes it's a static member
}
```

---

## Template Argument Deduction (TAD) & CTAD (C++17)

```cpp
// TAD — function template deduction
template<typename T>
void f(T x);
f(42);      // T = int
f(3.14f);   // T = float

// Reference collapsing in deduction
template<typename T>
void g(T&& x);    // "forwarding reference" (universal reference)
int i = 5;
g(i);       // T = int&;  x is int&
g(5);       // T = int;   x is int&&

// CTAD (Class Template Argument Deduction, C++17) — compiler deduces template args
std::pair p(1, 3.14);        // pair<int,double> — deduced
std::vector v = {1,2,3};     // vector<int>
Stack s{42};                 // deduced via user-provided deduction guide
std::tuple t(1, 'a', 2.0);  // tuple<int,char,double>

// Custom deduction guide
template<typename T>
Stack(std::initializer_list<T>) -> Stack<T>;
```

---

## `constexpr if` (C++17)

```cpp
// Compile-time branching — branches not compiled for the current instantiation
template<typename T>
void process(T val) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "int: " << val << '\n';
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "float: " << val << '\n';
    } else {
        std::cout << "other\n";
    }
}

// Replaces SFINAE (Substitution Failure Is Not An Error) for simple cases
template<typename T>
std::string stringify(T x) {
    if constexpr (std::is_same_v<T, bool>) return x ? "true" : "false";
    else return std::to_string(x);
}
```

---

## Variadic Templates

```cpp
// Parameter pack
template<typename... Ts>
void print(Ts... args) {
    (std::cout << ... << args) << '\n';   // fold expression
}
print(1, " hello ", 3.14);   // "1 hello 3.14"

// Recursive unpacking (pre-C++17 style)
void print_r() {}   // base case
template<typename First, typename... Rest>
void print_r(First first, Rest... rest) {
    std::cout << first << ' ';
    print_r(rest...);   // recurse
}

// sizeof... — number of args in pack
template<typename... Ts>
constexpr size_t count() { return sizeof...(Ts); }
count<int, double, char>();  // 3
```

---

## Fold Expressions (C++17)

```cpp
// Unary right fold:  (pack op ...)
// Unary left fold:   (... op pack)
// Binary right fold: (pack op ... op init)
// Binary left fold:  (init op ... op pack)

template<typename... Ts>
auto sum(Ts... vals) { return (... + vals); }       // left fold: ((v1+v2)+v3)...
sum(1, 2, 3, 4);   // 10

template<typename... Ts>
bool all_true(Ts... vals) { return (... && vals); } // left fold with &&

template<typename... Ts>
void print_all(Ts... vals) { (std::cout << ... << vals); }  // left fold <<

// With separator:
template<typename... Ts>
void print_sep(Ts... vals) {
    bool first = true;
    ((std::cout << (first ? "" : ", ") << vals, first = false), ...);
}
```

---

## SFINAE (Substitution Failure Is Not An Error) & `std::enable_if`

```cpp
#include <type_traits>

// Enable function only if T is integral
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T>
    square(T x) { return x * x; }

// Using default template arg (less obtrusive)
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
T cube(T x) { return x * x * x; }

// std::conditional — choose type at compile time
template<bool B, typename T, typename F>
using conditional_t = std::conditional_t<B, T, F>;

conditional_t<true,  int, double>   // → int
conditional_t<false, int, double>   // → double
```

---

## Concepts (C++20)

```cpp
#include <concepts>

// Predefined concepts
template<std::integral T>         void f(T x);   // int, char, bool, ...
template<std::floating_point T>   void g(T x);
template<std::copyable T>         void h(T x);
template<std::ranges::range R>    void iterate(R&& r);

// Custom concept
template<typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template<Addable T>
T add(T a, T b) { return a + b; }

// requires clause
template<typename T>
requires std::is_integral_v<T> && (sizeof(T) >= 4)
T big_square(T x) { return x * x; }

// Abbreviated function template (C++20)
void process(std::integral auto x) { }
auto max_val(auto a, auto b) { return a > b ? a : b; }
```

---

## Common Pitfalls

```cpp
// 1. Must define templates in header (not .cpp)
// Linker can't find definition if template is defined in .cpp and used elsewhere.
// All template definitions must be visible at instantiation point.
// Exception: explicit instantiation in .cpp:
template class Stack<int>;   // instantiates in this TU (Translation Unit) only

// 2. Code bloat
// Each instantiation generates separate code.
// Stack<int>, Stack<double>, Stack<long> = 3 copies of all methods.

// 3. Unhelpful error messages (pre-concepts)
// SFINAE failures produce wall-of-text errors.
// Use concepts (C++20) or static_assert to improve:
template<typename T>
void f(T x) {
    static_assert(std::is_integral_v<T>, "T must be integral");
}

// 4. typename for dependent type names
template<typename T>
void f() {
    T::value_type x;         // ERROR: compiler assumes T::value_type is value
    typename T::value_type x; // OK
}

// 5. template keyword for dependent template names
template<typename T>
void f(T obj) {
    obj.func<int>();          // ERROR: < is less-than
    obj.template func<int>(); // OK
}

// 6. CTAD pitfalls
std::vector v{1, 2, 3};   // vector<int> — OK
std::vector v2{std::vector<int>{1,2,3}};  // vector<vector<int>>! not vector<int>
```

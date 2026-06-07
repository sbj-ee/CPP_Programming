# C++ Type System — Cheat Sheet

## Fundamental Types

| Category | Type | Size (typical) | Range / Notes |
|----------|------|---------------|---------------|
| Integer | `bool` | 1 byte | `true`/`false` |
| Integer | `char` | 1 byte | signed or unsigned (impl-defined) |
| Integer | `signed char` | 1 byte | -128 … 127 |
| Integer | `unsigned char` | 1 byte | 0 … 255 |
| Integer | `short` | 2 bytes | -32768 … 32767 |
| Integer | `unsigned short` | 2 bytes | 0 … 65535 |
| Integer | `int` | 4 bytes | -2147483648 … 2147483647 |
| Integer | `unsigned int` | 4 bytes | 0 … 4294967295 |
| Integer | `long` | 4 or 8 bytes | platform-dependent |
| Integer | `long long` | 8 bytes | ≥ 64 bits guaranteed |
| Integer | `unsigned long long` | 8 bytes | 0 … 1.8×10¹⁹ |
| Float | `float` | 4 bytes | ~7 sig digits, IEEE 754 |
| Float | `double` | 8 bytes | ~15 sig digits |
| Float | `long double` | 8, 10, or 16 bytes | impl-defined |
| Char | `char8_t` (C++20) | 1 byte | UTF-8 code unit |
| Char | `char16_t` | 2 bytes | UTF-16 code unit |
| Char | `char32_t` | 4 bytes | UTF-32 code point |
| Void | `void` | — | no value |
| Null | `std::nullptr_t` | sizeof(void*) | type of `nullptr` |

### Fixed-width integer types (`<cstdint>`)

```cpp
#include <cstdint>
int8_t   int16_t   int32_t   int64_t    // exact width, signed
uint8_t  uint16_t  uint32_t  uint64_t   // exact width, unsigned
int_fast32_t   uint_fast32_t            // fastest type >= width
int_least32_t  uint_least32_t           // smallest type >= width
intmax_t  uintmax_t                     // widest available
intptr_t  uintptr_t                     // can hold a pointer value
ptrdiff_t  size_t                       // in <cstddef>
```

---

## `auto` and Type Deduction

```cpp
auto x = 42;           // int
auto y = 3.14;         // double
auto z = 3.14f;        // float
auto s = "hello";      // const char*
auto& r = x;           // int& — reference preserved with &
const auto& cr = x;    // const int&
auto* p = &x;          // int*

// auto strips top-level cv-qualifiers and references:
const int ci = 5;
auto a = ci;    // int  (const stripped)
auto& b = ci;   // const int& (const preserved via reference)

// Return type deduction (C++14)
auto add(int a, int b) { return a + b; }  // returns int

// Trailing return type (C++11)
auto divide(double a, double b) -> double { return a / b; }
```

### `decltype`

```cpp
int x = 5;
decltype(x) y = 10;         // int  — same type as x
decltype((x)) z = x;        // int& — parenthesised expr = lvalue ref
decltype(x + 1.0) d = 0.0;  // double

// decltype(auto) — deduces including references
decltype(auto) f() { int x = 0; return x; }   // returns int
decltype(auto) g() { int x = 0; return (x); }  // returns int& (DANGER)
```

---

## `const`, `constexpr`, `consteval`

```cpp
const int N = 10;               // run-time or compile-time constant
constexpr int M = 20;           // must be compile-time constant
constexpr int sq(int x) { return x * x; }  // can be called at compile time

// consteval (C++20) — MUST be evaluated at compile time
consteval int cube(int x) { return x * x * x; }
constexpr int v = cube(3);      // OK — compile time
// int n = 5; int bad = cube(n); // ERROR — n not constexpr

// constinit (C++20) — variable initialised at compile time but not const
constinit int counter = 0;      // OK to modify later
```

---

## Integer Literal Suffixes

| Suffix | Type |
|--------|------|
| `42` | `int` |
| `42u` or `42U` | `unsigned int` |
| `42l` or `42L` | `long` |
| `42ul` / `42UL` | `unsigned long` |
| `42ll` / `42LL` | `long long` |
| `42ull` / `42ULL` | `unsigned long long` |
| `42uz` / `42UZ` (C++23) | `std::size_t` |
| `3.14f` / `3.14F` | `float` |
| `3.14l` / `3.14L` | `long double` |

```cpp
int   dec = 255;
int   hex = 0xFF;         // hexadecimal
int   oct = 0377;         // octal
int   bin = 0b1111'1111;  // binary (C++14); ' = digit separator
auto  sz  = 42uz;         // std::size_t (C++23)
```

---

## `std::numeric_limits<T>`

```cpp
#include <limits>

std::numeric_limits<int>::max()       // 2147483647
std::numeric_limits<int>::min()       // -2147483648 (most negative)
std::numeric_limits<int>::lowest()    // same as min() for integers
std::numeric_limits<double>::max()    // ~1.8e308
std::numeric_limits<double>::lowest() // ~-1.8e308 (negative)
std::numeric_limits<double>::min()    // ~2.2e-308 (smallest positive normal)
std::numeric_limits<double>::epsilon()// ~2.2e-16 (ULP at 1.0)
std::numeric_limits<double>::infinity()
std::numeric_limits<int>::is_signed   // true
std::numeric_limits<int>::digits      // 31 (binary digits, excluding sign)
```

---

## Implicit Conversions & Integer Promotion

**Integral promotion** (smaller → `int` or `unsigned int`):
- `bool`, `char`, `short`, `int bit-fields` → `int` (if fits) or `unsigned int`

**Usual arithmetic conversions** (common type for binary ops):
1. If either operand is `long double` → `long double`
2. Else if either is `double` → `double`
3. Else if either is `float` → `float`
4. Else apply integral promotions, then:
   - If same signedness → wider type wins
   - If unsigned wider → unsigned wins
   - If signed wider and can represent all values of unsigned → signed wins
   - Else → unsigned version of the signed type

```cpp
unsigned int u = 10;
int i = -1;
// i < u  →  FALSE! i converted to unsigned: wraps to UINT_MAX
bool trap = (i < u);   // trap == false — common bug
```

---

## Casts

```cpp
// static_cast — well-defined conversions; checked at compile time
double d = 3.7;
int i = static_cast<int>(d);           // 3 (truncates)
int* p = static_cast<int*>(nullptr);   // OK

// dynamic_cast — safe downcasting through inheritance (RTTI required)
Base* b = new Derived();
Derived* dp = dynamic_cast<Derived*>(b);  // returns nullptr if fails
Derived& dr = dynamic_cast<Derived&>(*b); // throws std::bad_cast if fails

// const_cast — add/remove const (only valid if object was non-const)
const int ci = 5;
int* pi = const_cast<int*>(&ci);  // UB if you write through pi!

// reinterpret_cast — raw bit reinterpretation; almost always unsafe
uint64_t bits = reinterpret_cast<uint64_t>(ptr);
// use memcpy for type-punning instead!
```

| Cast | Use | Safety |
|------|-----|--------|
| `static_cast` | related types, numeric conversions | compile-time checked |
| `dynamic_cast` | polymorphic downcast | run-time checked |
| `const_cast` | add/remove const | UB if used wrong |
| `reinterpret_cast` | pointer bit-level reinterpret | almost always unsafe |
| C-style `(T)x` | any of the above | avoid — hides intent |

---

## `sizeof`, `alignof`, `alignas`

```cpp
sizeof(int)          // 4 (bytes)
sizeof(double)       // 8
sizeof(arr)          // total bytes (only for arrays, not pointers!)
sizeof(arr)/sizeof(arr[0])   // element count — use std::size() in C++17

alignof(double)      // 8 — required alignment in bytes
alignof(std::max_align_t)    // maximum fundamental alignment

// alignas — request specific alignment
alignas(64) char cache_line[64];   // aligned to 64-byte boundary
alignas(double) char buf[8];       // align buf as double

struct alignas(16) Vec4 {
    float x, y, z, w;
};
```

---

## Type Aliases

```cpp
// Modern: using (preferred)
using Byte    = unsigned char;
using IntVec  = std::vector<int>;
using Compare = bool(*)(int, int);   // function pointer type

// Template alias (impossible with typedef)
template<typename T>
using Vec = std::vector<T>;
Vec<int> v;  // std::vector<int>

// Legacy: typedef (still valid)
typedef unsigned char Byte;
typedef void (*Callback)(int);   // function pointer — ugly
```

---

## Common Pitfalls

```cpp
// 1. Signed/unsigned comparison
for (int i = 0; i < vec.size(); ++i) { }   // warning: i vs size_t
// Fix:
for (size_t i = 0; i < vec.size(); ++i) { }
for (int i = 0; i < (int)vec.size(); ++i) { }

// 2. Narrowing conversion in {} initialisation — compile error
int x{3.14};    // ERROR: narrowing
int y = 3.14;   // OK (but lossy, warns with -Wconversion)

// 3. char signedness
char c = 200;   // UB if char is signed (overflow); use unsigned char

// 4. decltype((x)) is a reference — accidentally returning dangling ref
decltype(auto) bad() { int x = 0; return (x); }  // returns int& to local!

// 5. constexpr function called at runtime — result not guaranteed CT
constexpr int sq(int x) { return x*x; }
int n = sq(5);   // may be evaluated at run time; use consteval to force CT

// 6. Integer overflow (signed UB)
int big = INT_MAX + 1;   // undefined behaviour
unsigned ubig = UINT_MAX + 1;  // well-defined: wraps to 0
```

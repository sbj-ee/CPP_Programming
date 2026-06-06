// variables.cpp - Exercise 02: Variables and Types
// Demonstrates: fundamental types, auto, const/constexpr,
//               references, sizeof, numeric_limits, casts

#include <iostream>
#include <limits>     // std::numeric_limits
#include <string>     // std::string
#include <typeinfo>   // typeid

// ── Section 1: Fundamental types ─────────────────────────────────────────────

void section_fundamental_types()
{
    std::cout << "=== Section 1: Fundamental Types ===\n";

    int    i    = 42;
    double d    = 3.14159;
    bool   b    = true;
    char   c    = 'A';
    unsigned int u = 4000000000U;   // ~4 billion, no sign bit

    std::cout << "int      i = " << i << "\n";
    std::cout << "double   d = " << d << "\n";
    std::cout << "bool     b = " << std::boolalpha << b << "\n";
    std::cout << "char     c = " << c << "  (ASCII " << static_cast<int>(c) << ")\n";
    std::cout << "unsigned u = " << u << "\n";

    // Other common types
    long long  ll  = 9'223'372'036'854'775'807LL;   // max long long (digit sep)
    float      f   = 2.718f;
    std::cout << "long long ll = " << ll << "\n";
    std::cout << "float     f  = " << f  << "\n";
}

// ── Section 2: auto keyword ───────────────────────────────────────────────────

void section_auto()
{
    std::cout << "\n=== Section 2: auto Keyword and Type Deduction ===\n";

    auto x   = 10;         // deduced as int
    auto y   = 3.14;       // deduced as double
    auto z   = true;       // deduced as bool
    auto ch  = 'Z';        // deduced as char
    auto str = std::string("hello");  // deduced as std::string

    std::cout << "auto x   = " << x   << "  (type: " << typeid(x).name()   << ")\n";
    std::cout << "auto y   = " << y   << "  (type: " << typeid(y).name()   << ")\n";
    std::cout << "auto z   = " << std::boolalpha << z
              << "  (type: " << typeid(z).name()   << ")\n";
    std::cout << "auto ch  = " << ch  << "  (type: " << typeid(ch).name()  << ")\n";
    std::cout << "auto str = " << str << "  (type: std::string)\n";

    // auto with references
    int  base = 100;
    auto val  = base;    // copy
    auto& ref = base;    // reference (must say auto&)
    ref = 200;
    std::cout << "After ref=200: base=" << base << " val=" << val << "\n";
}

// ── Section 3: const and constexpr ───────────────────────────────────────────

void section_const_constexpr()
{
    std::cout << "\n=== Section 3: const and constexpr ===\n";

    // const: value cannot change after initialization (enforced at runtime).
    const int max_users = 100;
    std::cout << "const int max_users = " << max_users << "\n";
    // max_users = 200;  // error: assignment of read-only variable

    // constexpr: value must be known at compile time.
    constexpr double PI         = 3.14159265358979;
    constexpr int    ARRAY_SIZE = 5;
    std::cout << "constexpr PI         = " << PI         << "\n";
    std::cout << "constexpr ARRAY_SIZE = " << ARRAY_SIZE << "\n";

    // constexpr can be used where a compile-time constant is required
    int arr[ARRAY_SIZE] = {1, 2, 3, 4, 5};  // valid: ARRAY_SIZE is constexpr
    std::cout << "Array of size " << ARRAY_SIZE << ": ";
    for (int i = 0; i < ARRAY_SIZE; ++i)
        std::cout << arr[i] << " ";
    std::cout << "\n";

    // const vs constexpr summary
    std::cout << "const    -> cannot change; may be determined at runtime\n";
    std::cout << "constexpr -> compile-time constant; stricter guarantee\n";
}

// ── Section 4: References ─────────────────────────────────────────────────────

void section_references()
{
    std::cout << "\n=== Section 4: References ===\n";

    int x = 10;
    int& r = x;          // r is an alias for x

    std::cout << "x = " << x << ", r = " << r << "\n";
    r = 42;              // modifies x through r
    std::cout << "After r=42: x = " << x << "\n";
    std::cout << "Address of x: " << &x << "\n";
    std::cout << "Address of r: " << &r << "  (same as x)\n";

    // const reference: read-only alias, can bind to temporaries
    const int& cr = 99;  // binds to a temporary
    std::cout << "const int& cr = " << cr << " (bound to temporary 99)\n";

    // References cannot be reseated (always refer to same object)
    int y = 99;
    r = y;               // copies y's value INTO x; does NOT rebind r
    std::cout << "After r=y (y=99): x=" << x << " r=" << r << "\n";
}

// ── Section 5: sizeof and numeric_limits ─────────────────────────────────────

void section_sizeof_limits()
{
    std::cout << "\n=== Section 5: sizeof and std::numeric_limits ===\n";

    std::cout << "sizeof(char)      = " << sizeof(char)      << " byte(s)\n";
    std::cout << "sizeof(short)     = " << sizeof(short)     << " byte(s)\n";
    std::cout << "sizeof(int)       = " << sizeof(int)       << " byte(s)\n";
    std::cout << "sizeof(long)      = " << sizeof(long)      << " byte(s)\n";
    std::cout << "sizeof(long long) = " << sizeof(long long) << " byte(s)\n";
    std::cout << "sizeof(float)     = " << sizeof(float)     << " byte(s)\n";
    std::cout << "sizeof(double)    = " << sizeof(double)    << " byte(s)\n";
    std::cout << "sizeof(bool)      = " << sizeof(bool)      << " byte(s)\n";

    std::cout << "\nnumeric_limits:\n";
    std::cout << "  int  min = " << std::numeric_limits<int>::min()    << "\n";
    std::cout << "  int  max = " << std::numeric_limits<int>::max()    << "\n";
    std::cout << "  uint max = " << std::numeric_limits<unsigned int>::max() << "\n";
    std::cout << "  dbl  min = " << std::numeric_limits<double>::min() << "\n";
    std::cout << "  dbl  max = " << std::numeric_limits<double>::max() << "\n";
    std::cout << "  dbl  eps = " << std::numeric_limits<double>::epsilon() << "\n";
}

// ── Section 6: Implicit and explicit casts ────────────────────────────────────

void section_casts()
{
    std::cout << "\n=== Section 6: Implicit and Explicit Casts ===\n";

    // Implicit (widening): safe, no data loss
    int    i = 42;
    double d = i;   // int -> double (implicit, safe)
    std::cout << "Implicit int->double: " << i << " -> " << d << "\n";

    // Implicit (narrowing): may truncate — compiler may warn
    double pi   = 3.14159;
    int    trunc = static_cast<int>(pi);  // explicit: documents intent
    std::cout << "static_cast<int>(3.14159) = " << trunc << " (truncated)\n";

    // static_cast: compile-time checked, preferred C++ cast
    char   ch    = static_cast<char>(65);   // int -> char
    std::cout << "static_cast<char>(65) = '" << ch << "'\n";

    unsigned int u  = 4294967295U;
    int          si = static_cast<int>(u);  // wraps around
    std::cout << "static_cast<int>(UINT_MAX) = " << si << " (wrap-around)\n";

    // Signed / unsigned mismatch pitfall
    int    neg    = -1;
    unsigned int cmp = 0;
    // comparing neg < cmp with mixed types: neg is converted to unsigned!
    if (static_cast<unsigned int>(neg) > cmp)
        std::cout << "Pitfall: -1 as unsigned is a huge number (> 0 unsigned)\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Use static_cast<T> for most C++ casts (compile-time safe).\n";
    std::cout << "  - Avoid C-style casts (int)x — they bypass type checking.\n";
    std::cout << "  - auto deduces type from initializer; use auto& for references.\n";
    std::cout << "  - constexpr is stricter than const: guaranteed compile-time.\n";
    std::cout << "  - References are aliases; they cannot be reseated.\n";
}

int main()
{
    section_fundamental_types();
    section_auto();
    section_const_constexpr();
    section_references();
    section_sizeof_limits();
    section_casts();
    return 0;
}

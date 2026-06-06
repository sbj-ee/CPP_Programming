// =============================================================================
// Exercise 16: Enums, optional, variant
// =============================================================================
// Topics: enum class vs plain enum, underlying type, bit-flag enum class,
//         std::optional<T>, std::variant<Ts...>, std::monostate,
//         std::visit and the overloaded-lambda visitor pattern
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g enums_variants.cpp -o enums_variants
// =============================================================================

#include <iostream>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
// SECTION 1: enum class (Scoped) vs Plain enum
// =============================================================================
//
// Plain enum: enumerators pollute the enclosing namespace and implicitly
// convert to int.  This leads to accidental comparisons across enum types.
//
// enum class (scoped): enumerators live inside the enum's scope, do not
// implicitly convert to int, and two different enum classes cannot be
// accidentally compared.  Always prefer enum class.

enum Direction { NORTH, SOUTH, EAST, WEST };   // plain — pollutes namespace

enum class Color { Red, Green, Blue };          // scoped — clean namespace

void demo_enum_class() {
    std::cout << "\n--- Section 1: enum class vs plain enum ---\n";

    // Plain enum: implicit int conversion (often a pitfall)
    Direction d = NORTH;
    int raw = d;   // implicit conversion — allowed but rarely desired
    std::cout << "plain NORTH as int: " << raw << "\n";

    // enum class: must cast explicitly
    Color c = Color::Green;
    int ci = static_cast<int>(c);
    std::cout << "Color::Green as int: " << ci << "\n";

    // Switch on enum class
    switch (c) {
        case Color::Red:   std::cout << "Red\n";   break;
        case Color::Green: std::cout << "Green\n"; break;
        case Color::Blue:  std::cout << "Blue\n";  break;
    }

    // Comparison only within the same enum class
    Color c2 = Color::Red;
    std::cout << "Green == Red? " << (c == c2 ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 2: Underlying Type
// =============================================================================
//
// You can specify the underlying integer type of an enum class.
// This controls storage size and is useful for binary protocols or
// when interoperating with C APIs.

enum class Status : uint8_t {
    OK       = 0,
    NotFound = 1,
    Error    = 2,
    Timeout  = 3
};

void demo_underlying_type() {
    std::cout << "\n--- Section 2: Underlying Type ---\n";

    Status s = Status::NotFound;
    auto raw = static_cast<uint8_t>(s);
    std::cout << "Status::NotFound underlying value: "
              << static_cast<int>(raw) << "\n";
    std::cout << "sizeof(Status) = " << sizeof(Status) << " byte(s)\n";

    // Round-trip: cast from int to enum class
    Status from_int = static_cast<Status>(2);
    std::cout << "Status(2) == Error? "
              << (from_int == Status::Error ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 3: Bit-Flag enum class with operator| and operator&
// =============================================================================
//
// Plain enums were often used as bit flags, but enum class doesn't support
// bitwise operators by default.  We define them explicitly for type safety
// while retaining full flag semantics.

enum class Permission : uint8_t {
    None    = 0,
    Read    = 1 << 0,   // 0b0001 = 1
    Write   = 1 << 1,   // 0b0010 = 2
    Execute = 1 << 2    // 0b0100 = 4
};

// Operator overloads for flag combinations
inline Permission operator|(Permission a, Permission b) {
    return static_cast<Permission>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline Permission operator&(Permission a, Permission b) {
    return static_cast<Permission>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool has_flag(Permission set, Permission flag) {
    return (set & flag) != Permission::None;
}

void demo_bit_flags() {
    std::cout << "\n--- Section 3: Bit-Flag enum class ---\n";

    Permission perms = Permission::Read | Permission::Write;
    std::cout << "Read|Write raw: "
              << static_cast<int>(static_cast<uint8_t>(perms)) << "\n";

    std::cout << "has Read?    " << (has_flag(perms, Permission::Read)    ? "yes" : "no") << "\n";
    std::cout << "has Write?   " << (has_flag(perms, Permission::Write)   ? "yes" : "no") << "\n";
    std::cout << "has Execute? " << (has_flag(perms, Permission::Execute) ? "yes" : "no") << "\n";

    perms = perms | Permission::Execute;
    std::cout << "After adding Execute, has Execute? "
              << (has_flag(perms, Permission::Execute) ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 4: std::optional<T>
// =============================================================================
//
// optional represents a value that may or may not be present.  It avoids the
// need for sentinel values (e.g., -1 for "not found") or raw pointers.
// The contained object is stored inline — no heap allocation.
//
// Key API:
//   has_value() / operator bool — check if a value is present
//   value()                     — access (throws std::bad_optional_access if empty)
//   value_or(default)           — access with a fallback
//   *opt / opt->member          — unchecked access (UB if empty)

std::optional<int> safe_divide(int num, int den) {
    if (den == 0) return std::nullopt;
    return num / den;
}

std::optional<std::string> find_user(int id) {
    if (id == 42) return "Alice";
    if (id == 7)  return "Bob";
    return std::nullopt;
}

void demo_optional() {
    std::cout << "\n--- Section 4: std::optional ---\n";

    auto result = safe_divide(10, 2);
    if (result) {
        std::cout << "10/2 = " << *result << "\n";
    }

    auto bad = safe_divide(5, 0);
    std::cout << "5/0 has value? " << (bad.has_value() ? "yes" : "no") << "\n";
    std::cout << "5/0 value_or(-1): " << bad.value_or(-1) << "\n";

    auto user = find_user(42);
    std::cout << "User 42: " << user.value_or("not found") << "\n";
    std::cout << "User 99: " << find_user(99).value_or("not found") << "\n";

    // Constructing in-place
    std::optional<std::string> opt_str{"hello"};
    opt_str->append(" world");
    std::cout << "in-place: " << *opt_str << "\n";

    // Reset to empty
    opt_str.reset();
    std::cout << "after reset, has value? " << (opt_str ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 5: std::variant<Ts...> — Type-Safe Union
// =============================================================================
//
// variant holds exactly ONE value from a set of types at a time.
// It tracks the active type at runtime.  Unlike union, it calls constructors
// and destructors properly.
//
// Key API:
//   std::get<T>(v)     — access typed value (throws std::bad_variant_access)
//   std::get_if<T>(&v) — returns pointer; nullptr if wrong type (safe)
//   v.index()          — returns the 0-based index of the active type
//   std::holds_alternative<T>(v) — type check

void demo_variant_basics() {
    std::cout << "\n--- Section 5: std::variant basics ---\n";

    std::variant<int, double, std::string> v;

    v = 42;
    std::cout << "v = int 42, index=" << v.index() << "\n";
    std::cout << "get<int>: " << std::get<int>(v) << "\n";

    v = 3.14;
    std::cout << "v = double 3.14, index=" << v.index() << "\n";

    v = std::string("hello variant");
    std::cout << "v = string, get<string>: " << std::get<std::string>(v) << "\n";

    // get_if: safe access without exceptions
    auto* sp = std::get_if<std::string>(&v);
    auto* ip = std::get_if<int>(&v);
    std::cout << "get_if<string>: " << (sp ? *sp : "null") << "\n";
    std::cout << "get_if<int>:    " << (ip ? std::to_string(*ip) : "null") << "\n";

    // holds_alternative
    std::cout << "holds int? "
              << (std::holds_alternative<int>(v) ? "yes" : "no") << "\n";

    // Bad access throws
    try {
        std::get<int>(v);   // v holds string, not int
    } catch (const std::bad_variant_access& e) {
        std::cout << "Caught bad_variant_access: " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 6: std::monostate and std::visit with Overloaded Lambdas
// =============================================================================
//
// std::monostate is an empty struct used as the first alternative in a variant
// to make it default-constructible (otherwise the first type must be default-
// constructible).
//
// std::visit applies a visitor callable to the active alternative.
// The overloaded-lambda idiom creates a single visitor from multiple lambdas
// using struct inheritance and 'using' to bring all operator() into scope.

// Overloaded helper: inherits from all Lambdas, exposes all operator()
template <typename... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};
// Deduction guide (C++17: often needed)
template <typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

using Token = std::variant<std::monostate, int, double, std::string>;

void demo_visit() {
    std::cout << "\n--- Section 6: std::monostate + std::visit ---\n";

    std::vector<Token> tokens = {
        std::monostate{},     // "no value"
        42,
        3.14,
        std::string("hello"),
        100,
        std::string("world")
    };

    auto visitor = Overloaded{
        [](std::monostate)         { std::cout << "  [empty]\n"; },
        [](int i)                  { std::cout << "  int: "    << i << "\n"; },
        [](double d)               { std::cout << "  double: " << d << "\n"; },
        [](const std::string& s)   { std::cout << "  string: " << s << "\n"; }
    };

    std::cout << "Visiting all tokens:\n";
    for (const auto& tok : tokens) {
        std::visit(visitor, tok);
    }

    // visit can also return a value
    auto to_string = [](const Token& tok) -> std::string {
        return std::visit(Overloaded{
            [](std::monostate)       { return std::string("null"); },
            [](int i)                { return std::to_string(i); },
            [](double d)             { return std::to_string(d); },
            [](const std::string& s) { return s; }
        }, tok);
    };

    for (const auto& tok : tokens) {
        std::cout << "  as string: " << to_string(tok) << "\n";
    }
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 16: Enums, optional, variant ===\n";

    demo_enum_class();
    demo_underlying_type();
    demo_bit_flags();
    demo_optional();
    demo_variant_basics();
    demo_visit();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Always prefer 'enum class' over plain 'enum' for type\n"
              << "   safety and to avoid polluting the enclosing namespace.\n";
    std::cout << "2. std::optional avoids sentinel values and null pointers;\n"
              << "   use value_or() to provide a safe default.\n";
    std::cout << "3. std::variant is never empty (unlike plain union) unless\n"
              << "   std::monostate is listed as an alternative.\n";
    std::cout << "4. Prefer std::get_if over std::get to avoid exceptions\n"
              << "   when the active type is uncertain.\n";
    std::cout << "5. std::visit requires the visitor to handle ALL alternatives;\n"
              << "   the overloaded-lambda pattern is the cleanest way to do this.\n";

    return 0;
}

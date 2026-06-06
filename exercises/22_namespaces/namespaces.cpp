// =============================================================================
// Exercise 22: Namespaces
// =============================================================================
// Topics: namespace declaration/nesting, using declaration vs using namespace,
//         anonymous namespaces (internal linkage), namespace aliases,
//         Argument-Dependent Lookup (ADL/Koenig), inline namespaces,
//         extern "C" linkage, header organisation best practices
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g namespaces.cpp -o namespaces
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cmath>

// =============================================================================
// SECTION 1: Namespace Declaration and Nesting
// =============================================================================
//
// A namespace groups related names and prevents collisions.
// Namespaces can be reopened (extended) across multiple files/translation units.
// Nested namespaces are accessed with '::' (scope resolution).
// C++17 allows nested namespace definition in one line: namespace A::B::C { }

namespace math {
    namespace geometry {
        struct Point {
            double x, y;
        };

        double distance(Point a, Point b) {
            double dx = a.x - b.x;
            double dy = a.y - b.y;
            return std::sqrt(dx*dx + dy*dy);
        }
    }  // namespace geometry

    constexpr double PI = 3.14159265358979;

    double circle_area(double r) { return PI * r * r; }
}  // namespace math

// C++17 nested namespace shorthand
namespace app::core::util {
    std::string upper(std::string s) {
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }
}

void demo_namespaces() {
    std::cout << "\n--- Section 1: Namespace Declaration and Nesting ---\n";

    // Fully qualified access
    math::geometry::Point p1{0, 0}, p2{3, 4};
    std::cout << "  distance(p1,p2)  = " << math::geometry::distance(p1, p2) << "\n";
    std::cout << "  circle_area(5)   = " << math::circle_area(5) << "\n";
    std::cout << "  PI               = " << math::PI << "\n";

    // C++17 nested namespace
    std::cout << "  upper('hello')   = " << app::core::util::upper("hello") << "\n";
}

// =============================================================================
// SECTION 2: using Declaration vs using namespace
// =============================================================================
//
// 'using ns::name;' — introduces ONLY that one name into the current scope.
//    Safe to use in a function body.  Prefer this over 'using namespace'.
//
// 'using namespace ns;' — introduces ALL names from the namespace.
//    Causes name collisions if the namespace is large (e.g., std).
//    NEVER put 'using namespace std;' at file scope in a header.
//
// 'using namespace ns;' in a function body is acceptable but still be careful.

void demo_using() {
    std::cout << "\n--- Section 2: using declaration vs using namespace ---\n";

    // using declaration: brings in only 'distance' from math::geometry
    using math::geometry::distance;
    using math::geometry::Point;

    Point a{1, 1}, b{4, 5};
    std::cout << "  via 'using math::geometry::distance': "
              << distance(a, b) << "\n";

    // using namespace inside a function: OK but be cautious
    {
        using namespace math;
        std::cout << "  via 'using namespace math' PI="
                  << PI << " area=" << circle_area(3) << "\n";
    }
    // math names are no longer in scope here

    // Contrast: 'using namespace std;' at file scope would bring in
    // hundreds of names — avoid it in headers and usually in .cpp files too.
}

// =============================================================================
// SECTION 3: Anonymous / Unnamed Namespaces (Internal Linkage)
// =============================================================================
//
// An unnamed namespace gives its contents INTERNAL LINKAGE — equivalent to
// the 'static' keyword on a free function or variable at file scope.
// Names in an unnamed namespace are local to the translation unit and cannot
// be accessed from other .cpp files.
// Modern C++ prefers unnamed namespaces over 'static' for file-local symbols.

namespace {
    // Only visible in THIS translation unit
    int file_local_counter = 0;

    void increment_counter() {
        ++file_local_counter;
    }

    std::string internal_format(int n) {
        return "[internal:" + std::to_string(n) + "]";
    }
}  // anonymous namespace

void demo_anonymous_namespace() {
    std::cout << "\n--- Section 3: Anonymous Namespace (Internal Linkage) ---\n";

    increment_counter();
    increment_counter();
    increment_counter();
    std::cout << "  file_local_counter = " << file_local_counter << "\n";
    std::cout << "  internal_format(7) = " << internal_format(7) << "\n";
    std::cout << "  (These symbols are NOT visible to other translation units)\n";
}

// =============================================================================
// SECTION 4: Namespace Aliases
// =============================================================================
//
// 'namespace alias = long::qualified::name;' creates a short alias.
// Particularly useful with deeply nested namespaces or long library names.
// std::filesystem is the canonical C++17 example.

namespace fs = std::filesystem;

// Alias for a project-specific namespace
namespace geom = math::geometry;

void demo_aliases() {
    std::cout << "\n--- Section 4: Namespace Aliases ---\n";

    // std::filesystem via alias
    fs::path cwd = fs::current_path();
    std::cout << "  current path: " << cwd.string() << "\n";

    // Project alias
    geom::Point q{5, 12};
    geom::Point origin{0, 0};
    std::cout << "  distance to origin: " << geom::distance(q, origin) << "\n";
}

// =============================================================================
// SECTION 5: Argument-Dependent Lookup (ADL / Koenig Lookup)
// =============================================================================
//
// When you call an unqualified function, the compiler looks for it not only in
// the current scope but ALSO in the namespaces of the function's argument types.
// This is ADL (Argument-Dependent Lookup), also called Koenig lookup.
//
// This is how 'std::cout << x' works: operator<< lives in namespace std, but
// you don't need 'std::operator<<(std::cout, x)' — ADL finds it because
// std::cout is of type std::ostream which lives in std.

namespace shapes {
    struct Circle {
        double radius;
    };

    // This operator<< lives in 'shapes', not at global scope
    std::ostream& operator<<(std::ostream& os, const Circle& c) {
        return os << "Circle(r=" << c.radius << ")";
    }

    void describe(const Circle& c) {
        std::cout << "  shapes::describe: " << c << "\n";  // ADL finds operator<<
    }

    double area(const Circle& c) { return math::PI * c.radius * c.radius; }
}

void demo_adl() {
    std::cout << "\n--- Section 5: Argument-Dependent Lookup (ADL) ---\n";

    shapes::Circle c{4.0};

    // Unqualified call — ADL finds shapes::operator<< because c is in shapes
    std::cout << "  ADL finds operator<<: " << c << "\n";

    // Unqualified call — ADL finds shapes::area because c is in shapes
    // (if we brought it into scope we could call area(c) without qualification)
    std::cout << "  area: " << shapes::area(c) << "\n";

    // Explicit qualification always works too
    std::cout << "  shapes::describe: ";
    shapes::describe(c);

    std::cout << "  (ADL is why 'std::cout << x' works without qualification)\n";
}

// =============================================================================
// SECTION 6: Inline Namespaces for Versioning
// =============================================================================
//
// An inline namespace's members are visible in the enclosing namespace, as if
// they were declared there.  This allows versioning of an API: the current
// version is 'inline', older versions are accessible via explicit qualification.
// Users who write 'lib::Widget' automatically get the inline (current) version;
// code that needs the old version can write 'lib::v1::Widget' explicitly.

namespace lib {
    namespace v1 {
        struct Widget {
            int value;
            void report() const {
                std::cout << "  v1::Widget value=" << value << "\n";
            }
        };
    }

    inline namespace v2 {   // current version; pulled into lib:: scope
        struct Widget {
            int value;
            std::string label;
            void report() const {
                std::cout << "  v2::Widget value=" << value
                          << " label=" << label << "\n";
            }
        };
    }
}  // namespace lib

void demo_inline_namespace() {
    std::cout << "\n--- Section 6: Inline Namespaces for Versioning ---\n";

    // lib::Widget resolves to lib::v2::Widget (the inline version)
    lib::Widget current{42, "modern"};
    current.report();

    // Explicit old version still accessible
    lib::v1::Widget old{7};
    old.report();

    std::cout << "  lib::Widget refers to the 'inline' (current) version\n";
}

// =============================================================================
// SECTION 7: extern "C" — C Linkage
// =============================================================================
//
// C++ uses name mangling to encode function signatures (overloads, templates).
// C does not.  'extern "C"' tells the compiler to use C linkage — no mangling.
// Essential when:
//   - Exporting a C++ function to be called from C.
//   - Calling a C function from C++ (wrapping a C header).
// A C header wrapper idiom uses #ifdef __cplusplus guards to be valid in both.

extern "C" {
    // This function has C linkage — its symbol name is simply "c_add"
    int c_add(int a, int b) {
        return a + b;
    }

    // Typedef for a callback compatible with C's calling convention
    typedef void (*c_callback)(int);
}

// C++ wrapper that accepts a C-linkage callback
void run_with_callback(c_callback cb, int value) {
    cb(value);
}

// A compatible callback (must use extern "C" for ABI correctness in real use)
extern "C" void my_callback(int x) {
    std::cout << "  C callback received: " << x << "\n";
}

void demo_extern_c() {
    std::cout << "\n--- Section 7: extern \"C\" Linkage ---\n";

    std::cout << "  c_add(3,4) = " << c_add(3, 4) << "\n";
    run_with_callback(my_callback, 99);

    std::cout << "  (In practice, C headers are wrapped in:\n"
              << "   #ifdef __cplusplus\n"
              << "   extern \"C\" {\n"
              << "   #endif\n"
              << "   ... declarations ...\n"
              << "   #ifdef __cplusplus\n"
              << "   }\n"
              << "   #endif\n"
              << "   to be valid in both C and C++ compilations.)\n";
}

// =============================================================================
// SECTION 8: Header Organisation Best Practices
// =============================================================================
//
// This section is commentary + a brief demo — single-file exercise limitation.
//
// Include guard vs #pragma once:
//   Traditional:  #ifndef MY_HEADER_H / #define MY_HEADER_H / #endif
//   Modern:       #pragma once  (compiler extension, widely supported)
//   Both prevent multiple inclusion. #pragma once is shorter; include guards
//   are strictly standard-conforming.
//
// Best practices:
//   1. Put only declarations in headers (.h/.hpp): class definitions, function
//      declarations, inline functions, templates, constexpr.
//   2. Put definitions in source files (.cpp): non-inline function bodies,
//      static member definitions.
//   3. Use forward declarations to reduce #include chains in headers.
//   4. Never put 'using namespace X;' at file scope in a header — it pollutes
//      every translation unit that includes it.
//   5. Self-contained headers: each header should #include its own dependencies.

// Forward declaration — avoids including a heavy header
class HeavyClass;
void process(HeavyClass* hc);  // declared here; defined in .cpp

// Using declaration restricted to a function scope — safe in headers
void demo_header_practices() {
    std::cout << "\n--- Section 8: Header Organisation Best Practices ---\n";

    std::cout << "  Good header discipline:\n";
    std::cout << "  - #pragma once (or include guards) at the top\n";
    std::cout << "  - Forward-declare types to avoid heavy includes\n";
    std::cout << "  - Never 'using namespace X' at file scope in a header\n";
    std::cout << "  - Include only what you use\n";
    std::cout << "  - Use 'inline' for functions defined in headers\n";
    std::cout << "    (otherwise ODR violation if included in multiple TUs)\n";

    // Scoped using declaration — fine even in header function bodies
    using std::string;
    string msg = "header best practices demo";
    std::cout << "  " << msg << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 22: Namespaces ===\n";

    demo_namespaces();
    demo_using();
    demo_anonymous_namespace();
    demo_aliases();
    demo_adl();
    demo_inline_namespace();
    demo_extern_c();
    demo_header_practices();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Never 'using namespace std;' at file scope — it imports\n"
              << "   hundreds of names and causes hard-to-diagnose collisions.\n";
    std::cout << "2. Prefer unnamed namespaces over 'static' for file-local\n"
              << "   symbols — they work for types and classes too.\n";
    std::cout << "3. ADL makes unqualified calls to operators (<<, +, ==) work\n"
              << "   naturally; define operators in the same namespace as the type.\n";
    std::cout << "4. Inline namespaces are a forward-compatible versioning tool;\n"
              << "   clients use 'lib::X' and silently get the current version.\n";
    std::cout << "5. extern \"C\" is essential at the C/C++ ABI boundary; without\n"
              << "   it, name mangling makes the symbol invisible to a C linker.\n";

    return 0;
}

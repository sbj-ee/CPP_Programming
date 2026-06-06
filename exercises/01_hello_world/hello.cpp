// hello.cpp - Exercise 01: Hello World
// Demonstrates: std::cout, std::cerr, main return, comments,
//               #include, compile-time string concatenation, constexpr

#include <iostream>   // std::cout, std::cerr, std::endl
#include <string>     // std::string

// ── Section 1: std::cout and std::endl vs "\n" ──────────────────────────────

// constexpr string literal - evaluated at compile time
constexpr const char* PROGRAM_NAME = "hello";
constexpr const char* VERSION      = "1.0";

// Compile-time string concatenation: adjacent string literals are joined by
// the compiler (not at runtime).  The two halves below become one string.
constexpr const char* GREETING = "Hello, "
                                  "World!";

int main()
{
    std::cout << "=== Section 1: std::cout / std::endl vs \"\\n\" ===\n";

    // std::endl flushes the output buffer AND writes '\n'.
    // '\n' alone does NOT flush — faster for large output.
    std::cout << "Hello with std::endl" << std::endl;   // flush + newline
    std::cout << "Hello with \"\\n\"\n";                // newline only

    // Chaining operator<< calls
    std::cout << "Program : " << PROGRAM_NAME
              << "  version " << VERSION << "\n";

    // Compile-time concatenated literal
    std::cout << GREETING << "\n";

    // ── Section 2: std::cerr ─────────────────────────────────────────────────
    std::cout << "\n=== Section 2: std::cerr (standard error) ===\n";

    // std::cerr goes to stderr (file descriptor 2).  It is always unbuffered.
    // Use it for diagnostics/error messages so they appear even if stdout
    // is redirected.
    std::cerr << "[stderr] This error message goes to standard error.\n";
    std::cout << "[stdout] This line goes to standard output.\n";

    // ── Section 3: Comments ──────────────────────────────────────────────────
    std::cout << "\n=== Section 3: Comments ===\n";

    // Single-line comment: everything after // to end of line is ignored.

    /* Multi-line comment:
       spans multiple lines.
       Useful for block descriptions.
       NOTE: block comments cannot be nested - be careful closing them.
    */

    std::cout << "Both // and /* */ comment styles are valid in C++.\n";

    // ── Section 4: #include and standard headers ─────────────────────────────
    std::cout << "\n=== Section 4: #include and standard headers ===\n";

    std::cout << "<iostream> provides: std::cout, std::cin, std::cerr, std::clog\n";
    std::cout << "<string>   provides: std::string\n";
    std::cout << "Angle brackets <>  = system/standard headers\n";
    std::cout << "Quotes     \"\"  = project-local headers\n";

    // ── Section 5: Compile-time string concatenation ─────────────────────────
    std::cout << "\n=== Section 5: Compile-time string concatenation ===\n";

    // Adjacent string literals are merged by the compiler (preprocessor stage).
    // This is useful for splitting long strings across lines for readability.
    const char* message = "This is a very long string that is split "
                          "across two source lines "
                          "but stored as one contiguous array.";
    std::cout << message << "\n";

    // std::string also supports + for runtime concatenation:
    std::string part1 = "Runtime ";
    std::string part2 = "concatenation";
    std::cout << part1 + part2 + " with std::string\n";

    // ── Section 6: main return value ─────────────────────────────────────────
    std::cout << "\n=== Section 6: main return value ===\n";

    std::cout << "main() must return int (never void in C++).\n";
    std::cout << "Return 0  -> success (EXIT_SUCCESS)\n";
    std::cout << "Return non-zero -> failure (EXIT_FAILURE)\n";
    std::cout << "The OS reads this exit code (echo $? in bash).\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Prefer '\\n' over std::endl for performance (no flush).\n";
    std::cout << "  - Use std::cerr for errors; it is always unbuffered.\n";
    std::cout << "  - Adjacent string literals are concatenated at compile time.\n";
    std::cout << "  - constexpr values are computed at compile time, not runtime.\n";
    std::cout << "  - main() must return int; omitting return gives implicit 0.\n";

    return 0;
}

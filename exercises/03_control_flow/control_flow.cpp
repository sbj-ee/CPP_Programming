// control_flow.cpp - Exercise 03: Control Flow
// Demonstrates: if/else, switch with enum class, for/while/do-while,
//               range-based for, break/continue, structured bindings

#include <iostream>
#include <string>
#include <vector>
#include <utility>    // std::pair, std::make_pair
#include <array>

// ── Section 1: if / else if / else ───────────────────────────────────────────

void section_if_else()
{
    std::cout << "=== Section 1: if / else if / else ===\n";

    int score = 78;

    if (score >= 90) {
        std::cout << "Grade: A\n";
    } else if (score >= 80) {
        std::cout << "Grade: B\n";
    } else if (score >= 70) {
        std::cout << "Grade: C  (score=" << score << ")\n";
    } else if (score >= 60) {
        std::cout << "Grade: D\n";
    } else {
        std::cout << "Grade: F\n";
    }

    // C++17: initializer in if statement
    if (int remainder = score % 10; remainder > 5) {
        std::cout << "Score rounds up (remainder=" << remainder << ")\n";
    } else {
        std::cout << "Score rounds down (remainder=" << remainder << ")\n";
    }

    // Ternary operator (single expression)
    std::string label = (score >= 60) ? "passing" : "failing";
    std::cout << "Result: " << label << "\n";
}

// ── Section 2: switch / case / default with enum class ───────────────────────

enum class Direction { North, South, East, West };

void section_switch()
{
    std::cout << "\n=== Section 2: switch / case / default (with enum class) ===\n";

    Direction dir = Direction::East;

    switch (dir) {
        case Direction::North:
            std::cout << "Heading North\n";
            break;
        case Direction::South:
            std::cout << "Heading South\n";
            break;
        case Direction::East:
            std::cout << "Heading East\n";
            break;
        case Direction::West:
            std::cout << "Heading West\n";
            break;
        default:
            std::cout << "Unknown direction\n";
            break;
    }

    // enum class prevents implicit conversion to int (safer than plain enum)
    // int x = dir;          // error: cannot implicitly convert
    int x = static_cast<int>(dir);   // explicit cast required
    std::cout << "East as int: " << x << "\n";

    // Demonstrating fall-through (intentional, needs [[fallthrough]])
    int code = 2;
    std::cout << "Switch fall-through demo (code=" << code << "):\n";
    switch (code) {
        case 1:
        case 2:
            [[fallthrough]];      // C++17 attribute: suppress compiler warning
        case 3:
            std::cout << "  code is 1, 2, or 3\n";
            break;
        default:
            std::cout << "  other code\n";
    }
}

// ── Section 3: for / while / do-while ────────────────────────────────────────

void section_loops()
{
    std::cout << "\n=== Section 3: for / while / do-while ===\n";

    // Traditional for loop
    std::cout << "for loop (0..4): ";
    for (int i = 0; i < 5; ++i)
        std::cout << i << " ";
    std::cout << "\n";

    // Counting down
    std::cout << "countdown:       ";
    for (int i = 5; i > 0; --i)
        std::cout << i << " ";
    std::cout << "\n";

    // while loop
    std::cout << "while (collatz 27): ";
    int n = 27;
    int steps = 0;
    while (n != 1) {
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
        ++steps;
    }
    std::cout << steps << " steps to reach 1\n";

    // do-while (always executes body at least once)
    std::cout << "do-while: ";
    int val = 1;
    do {
        std::cout << val << " ";
        val *= 2;
    } while (val <= 16);
    std::cout << "\n";
}

// ── Section 4: Range-based for ────────────────────────────────────────────────

void section_range_for()
{
    std::cout << "\n=== Section 4: Range-based for ===\n";

    // Over a std::vector (copy)
    std::vector<int> nums = {10, 20, 30, 40, 50};
    std::cout << "vector by value:     ";
    for (int v : nums)
        std::cout << v << " ";
    std::cout << "\n";

    // By const reference (efficient, read-only)
    std::cout << "vector by const-ref: ";
    for (const int& v : nums)
        std::cout << v << " ";
    std::cout << "\n";

    // Modifying elements (by reference)
    for (int& v : nums)
        v += 1;
    std::cout << "vector after +=1:    ";
    for (const int& v : nums)
        std::cout << v << " ";
    std::cout << "\n";

    // Over a raw C-style array
    int arr[] = {5, 10, 15, 20, 25};
    std::cout << "raw array:           ";
    for (int v : arr)
        std::cout << v << " ";
    std::cout << "\n";

    // Over a std::string (character by character)
    std::string word = "hello";
    std::cout << "string chars:        ";
    for (char ch : word)
        std::cout << ch << '-';
    std::cout << "\n";
}

// ── Section 5: break and continue ────────────────────────────────────────────

void section_break_continue()
{
    std::cout << "\n=== Section 5: break and continue ===\n";

    // break: exit loop immediately
    std::cout << "break at 5: ";
    for (int i = 0; i < 10; ++i) {
        if (i == 5) break;
        std::cout << i << " ";
    }
    std::cout << "\n";

    // continue: skip rest of body, go to next iteration
    std::cout << "skip evens: ";
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) continue;
        std::cout << i << " ";
    }
    std::cout << "\n";

    // Nested loops: break only exits innermost loop
    std::cout << "nested break (inner only):\n";
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (j == 1) break;
            std::cout << "  (" << i << "," << j << ")\n";
        }
    }
}

// ── Section 6: Structured bindings with std::pair ────────────────────────────

void section_structured_bindings()
{
    std::cout << "\n=== Section 6: Structured Bindings (C++17) ===\n";

    // std::pair holds two values
    std::pair<std::string, int> person = std::make_pair("Alice", 30);

    // Traditional access
    std::cout << "Traditional: " << person.first << ", " << person.second << "\n";

    // C++17 structured binding: auto [a, b] = pair
    auto [name, age] = person;
    std::cout << "Structured:  " << name << ", " << age << "\n";

    // Useful in range-based for over a vector of pairs
    std::vector<std::pair<std::string, int>> scores = {
        {"Alice", 95}, {"Bob", 82}, {"Carol", 91}
    };

    std::cout << "Leaderboard:\n";
    for (const auto& [student, score] : scores) {
        std::cout << "  " << student << ": " << score << "\n";
    }

    // Structured binding also works with std::array and plain structs
    std::array<int, 3> rgb = {255, 128, 0};
    auto [r, g, b] = rgb;
    std::cout << "RGB: r=" << r << " g=" << g << " b=" << b << "\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Prefer ++i over i++ (avoids unnecessary copy for iterators).\n";
    std::cout << "  - enum class prevents accidental int conversions.\n";
    std::cout << "  - Always add break to switch cases unless fall-through is intentional.\n";
    std::cout << "  - Use [[fallthrough]] to silence the compiler on intentional fall-through.\n";
    std::cout << "  - Structured bindings (auto [a,b]=x) require C++17.\n";
    std::cout << "  - Range-for copies by default; use const auto& to avoid copies.\n";
}

int main()
{
    section_if_else();
    section_switch();
    section_loops();
    section_range_for();
    section_break_continue();
    section_structured_bindings();
    return 0;
}

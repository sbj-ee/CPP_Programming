// strings_vectors.cpp - Exercise 05: Strings and Vectors
// Demonstrates: std::string, std::string_view, std::vector<T>,
//               std::array<T,N>, string splitting, numeric conversions

#include <iostream>
#include <string>
#include <string_view>   // C++17
#include <vector>
#include <array>
#include <sstream>       // std::ostringstream (for conversion demo)

// ── Section 1: std::string ────────────────────────────────────────────────────

void section_string()
{
    std::cout << "=== Section 1: std::string ===\n";

    // Construction
    std::string s1 = "Hello";
    std::string s2("World");
    std::string s3(5, '*');              // "*****" (5 stars)
    std::string s4 = s1;                 // copy construction

    std::cout << "s1=\"" << s1 << "\"  s2=\"" << s2
              << "\"  s3=\"" << s3 << "\"\n";

    // size() / length() / empty()
    std::cout << "s1.size()   = " << s1.size()   << "\n";
    std::cout << "s1.empty()  = " << std::boolalpha << s1.empty() << "\n";
    std::cout << "\"\".empty()  = " << std::string{}.empty() << "\n";

    // Concatenation with +
    std::string joined = s1 + ", " + s2 + "!";
    std::cout << "joined = \"" << joined << "\"\n";

    // Append in place
    std::string sentence = "C++ is ";
    sentence += "great";
    std::cout << "sentence = \"" << sentence << "\"\n";

    // at() - bounds-checked access; operator[] - unchecked
    std::cout << "s1.at(0)  = '" << s1.at(0) << "'\n";
    std::cout << "s1[4]     = '" << s1[4]     << "'\n";

    // substr(pos, len)
    std::string lang = "C++ Programming";
    std::cout << "substr(0,3)  = \"" << lang.substr(0, 3)  << "\"\n";
    std::cout << "substr(4)    = \"" << lang.substr(4)     << "\"\n";  // to end

    // find() - returns pos or std::string::npos
    std::size_t pos = lang.find("Prog");
    if (pos != std::string::npos)
        std::cout << "\"Prog\" found at index " << pos << "\n";
    else
        std::cout << "\"Prog\" not found\n";

    // c_str() - null-terminated C string pointer (for C APIs)
    const char* cstr = lang.c_str();
    std::cout << "c_str() = \"" << cstr << "\"\n";

    // Comparison
    std::cout << "(s1 == \"Hello\") = " << (s1 == "Hello") << "\n";
    std::cout << "(s1 <  \"World\") = " << (s1 < s2) << "\n";
}

// ── Section 2: std::string_view (C++17) ──────────────────────────────────────

// string_view is a lightweight, non-owning reference to a string.
// Use it as a function parameter instead of const std::string& to avoid
// creating a temporary string when the caller passes a string literal.
std::size_t count_vowels(std::string_view sv)
{
    std::size_t count = 0;
    for (char c : sv)
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
            c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U')
            ++count;
    return count;
}

void section_string_view()
{
    std::cout << "\n=== Section 2: std::string_view (C++17) ===\n";

    std::string_view sv1 = "Hello, World!";
    std::cout << "sv1 = \"" << sv1 << "\"\n";
    std::cout << "sv1.size()    = " << sv1.size()    << "\n";
    std::cout << "sv1.substr(0,5) = \"" << sv1.substr(0, 5) << "\"\n";

    // Works with std::string too
    std::string word = "programming";
    std::string_view sv2 = word;     // zero-copy view of word
    std::cout << "sv2 (view of string) = \"" << sv2 << "\"\n";

    std::cout << "Vowels in \"programming\": " << count_vowels(word) << "\n";
    std::cout << "Vowels in literal:        " << count_vowels("extraordinary") << "\n";

    std::cout << "Warning: string_view does NOT own its data.\n";
    std::cout << "         Never return a string_view to a local string!\n";
}

// ── Section 3: std::vector<T> ─────────────────────────────────────────────────

void section_vector()
{
    std::cout << "\n=== Section 3: std::vector<T> ===\n";

    std::vector<int> v;
    std::cout << "Initial: size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // reserve: allocate memory without changing size
    v.reserve(8);
    std::cout << "After reserve(8): size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // push_back: copy into vector
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    // emplace_back: construct in-place (more efficient for complex types)
    v.emplace_back(40);
    v.emplace_back(50);

    std::cout << "After 5 pushes: size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // Access with at() (bounds-checked) and [] (unchecked)
    std::cout << "v.at(2)  = " << v.at(2) << "\n";
    std::cout << "v[4]     = " << v[4]    << "\n";
    std::cout << "v.front()= " << v.front() << "  v.back()=" << v.back() << "\n";

    // Range-based for
    std::cout << "elements: ";
    for (const int& elem : v)
        std::cout << elem << " ";
    std::cout << "\n";

    // Initialization with a list
    std::vector<std::string> names = {"Alice", "Bob", "Carol"};
    std::cout << "names: ";
    for (const auto& n : names)
        std::cout << n << " ";
    std::cout << "\n";

    // 2D vector
    std::vector<std::vector<int>> matrix(3, std::vector<int>(3, 0));
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            matrix[r][c] = r * 3 + c;
    std::cout << "3x3 matrix:\n";
    for (const auto& row : matrix) {
        std::cout << "  ";
        for (int val : row) std::cout << val << " ";
        std::cout << "\n";
    }

    // Erase
    std::vector<int> ev = {1, 2, 3, 4, 5};
    ev.erase(ev.begin() + 2);      // removes element at index 2 (value 3)
    std::cout << "After erase index 2: ";
    for (int x : ev) std::cout << x << " ";
    std::cout << "\n";
}

// ── Section 4: std::array<T,N> ────────────────────────────────────────────────

void section_array()
{
    std::cout << "\n=== Section 4: std::array<T,N> ===\n";

    // Fixed-size, stack-allocated, bounds checking via at()
    std::array<int, 5> arr = {10, 20, 30, 40, 50};

    std::cout << "arr.size() = " << arr.size() << "\n";
    std::cout << "arr[2]     = " << arr[2] << "\n";
    std::cout << "arr.at(4)  = " << arr.at(4) << "\n";
    std::cout << "front/back = " << arr.front() << "/" << arr.back() << "\n";

    // Range-based for
    std::cout << "elements: ";
    for (const int& v : arr) std::cout << v << " ";
    std::cout << "\n";

    // Unlike raw arrays, std::array knows its size and supports assignment
    std::array<int, 5> arr2 = arr;   // copy
    arr2[0] = 999;
    std::cout << "arr[0]=" << arr[0] << " arr2[0]=" << arr2[0] << " (independent copies)\n";

    // Aggregate initialization - partially specified: rest zeroed
    std::array<double, 4> temps = {98.6, 101.3};
    std::cout << "temps: ";
    for (double t : temps) std::cout << t << " ";
    std::cout << "\n";
}

// ── Section 5: String splitting ───────────────────────────────────────────────

std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> tokens;
    std::string token;
    for (char c : s) {
        if (c == delim) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

void section_split()
{
    std::cout << "\n=== Section 5: String Splitting ===\n";

    std::string csv = "Alice,Bob,,Carol,Dave";
    std::vector<std::string> parts = split(csv, ',');
    // Note: split() skips empty tokens — the double comma yields 4 results, not 5.
    std::cout << "Split \"" << csv << "\" by ',':\n";
    for (std::size_t i = 0; i < parts.size(); ++i)
        std::cout << "  [" << i << "] \"" << parts[i] << "\"\n";

    std::string path = "/usr/local/bin/program";
    auto segments = split(path, '/');
    std::cout << "Path segments:\n";
    for (const auto& seg : segments)
        std::cout << "  \"" << seg << "\"\n";
}

// ── Section 6: String <-> number conversions ──────────────────────────────────

void section_conversions()
{
    std::cout << "\n=== Section 6: String <-> Number Conversions ===\n";

    // String to number
    std::string si = "42";
    std::string sd = "3.14159";
    int    i = std::stoi(si);      // string to int
    double d = std::stod(sd);      // string to double
    long   l = std::stol("100000"); // string to long
    std::cout << "stoi(\"42\")      = " << i << "\n";
    std::cout << "stod(\"3.14159\") = " << d << "\n";
    std::cout << "stol(\"100000\")  = " << l << "\n";

    // Number to string
    std::string from_int = std::to_string(i);
    std::string from_dbl = std::to_string(d);
    std::cout << "to_string(42)      = \"" << from_int << "\"\n";
    std::cout << "to_string(3.14159) = \"" << from_dbl << "\"\n";

    // Use std::ostringstream for more formatting control
    std::ostringstream oss;
    oss << "pi ~ " << d << " (approx)";
    std::cout << "ostringstream: \"" << oss.str() << "\"\n";

    // Error handling: stoi throws std::invalid_argument or std::out_of_range
    try {
        int bad = std::stoi("not_a_number");
        (void)bad;
    } catch (const std::invalid_argument& e) {
        std::cout << "stoi(\"not_a_number\") threw: " << e.what() << "\n";
    }

    std::cout << "\nNotes:\n";
    std::cout << "  - std::string_view avoids copies; do not outlive the source string.\n";
    std::cout << "  - vector capacity grows geometrically; reserve() avoids reallocations.\n";
    std::cout << "  - std::array is zero-overhead vs raw arrays, but has size() and at().\n";
    std::cout << "  - stoi/stod throw on invalid input; wrap in try/catch.\n";
    std::cout << "  - to_string(double) has limited precision; prefer ostringstream.\n";
}

int main()
{
    section_string();
    section_string_view();
    section_vector();
    section_array();
    section_split();
    section_conversions();
    return 0;
}

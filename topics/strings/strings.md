# std::string & string_view — Cheat Sheet

## Construction

```cpp
#include <string>

std::string s1;                    // empty
std::string s2("hello");           // from C string
std::string s3 = "world";          // same
std::string s4(5, 'x');            // "xxxxx"
std::string s5(s2, 1, 3);         // substr from index 1, len 3: "ell"
std::string s6(s2.begin(), s2.begin() + 3);  // from iterators: "hel"

// String literals (C++14)
using namespace std::string_literals;
auto s7 = "hello"s;               // std::string, not const char*
auto s8 = u8"utf8"s;              // std::u8string (C++20)
```

---

## Size & Capacity

```cpp
s.size()       // number of chars (same as length())
s.length()     // same
s.empty()      // true if size() == 0
s.capacity()   // allocated capacity (>= size)
s.max_size()   // theoretical max
s.resize(n)    // grow (fill with '\0') or shrink
s.resize(n,'x')// grow filling with 'x'
s.reserve(n)   // pre-allocate; avoids repeated reallocation
s.shrink_to_fit()  // release excess capacity (hint)
s.clear()      // empty string; capacity unchanged
```

---

## Concatenation & Comparison

```cpp
std::string a = "hello";
std::string b = " world";

a + b           // "hello world" (new string)
a += b          // appends b to a in place
a += " !";      // append C string
a.append(b);                  // same as +=
a.append(b, 1, 3);            // append b[1..3]
a.append(3, '!');             // append 3 '!'

// Comparison (lexicographic)
a == b;   a != b;   a < b;   a <= b;   a > b;   a >= b;
a.compare(b);     // <0, 0, >0 (like strcmp)
a.compare(pos, len, b);       // compare substring
```

---

## Element Access

```cpp
s[0]         // no bounds check; UB (Undefined Behaviour) if out of range
s.at(0)      // bounds-checked; throws std::out_of_range
s.front()    // first char
s.back()     // last char
s.data()     // const char* (or char* since C++17)
s.c_str()    // always null-terminated const char*
```

---

## Search

```cpp
s.find("lo")            // first occurrence; returns index or std::string::npos
s.find("lo", 3)         // start search from index 3
s.rfind("lo")           // last occurrence
s.find_first_of("aeiou")    // first char that IS in set
s.find_last_of("aeiou")     // last char that IS in set
s.find_first_not_of("aeiou")// first char NOT in set
s.find_last_not_of("aeiou")

// Check result:
size_t pos = s.find("xyz");
if (pos != std::string::npos) { /* found at pos */ }
```

---

## Substr, Replace, Insert, Erase

```cpp
// substr — returns new string
s.substr(pos)           // from pos to end
s.substr(pos, len)      // from pos, length len

// replace
s.replace(pos, len, "new");         // replace s[pos..pos+len) with "new"
s.replace(pos, len, other, opos, olen);  // replace with substring of other

// insert
s.insert(pos, "inserted");     // insert before pos
s.insert(pos, 3, 'x');        // insert 3 'x' before pos
s.insert(it, 'c');            // insert before iterator

// erase
s.erase(pos);          // erase from pos to end
s.erase(pos, len);     // erase len chars starting at pos
s.erase(it);           // erase single char at iterator
s.erase(first, last);  // erase range [first, last)

// push_back / pop_back
s.push_back('!');
s.pop_back();
```

---

## Conversions

```cpp
#include <string>

// To number
std::stoi("42")        // → int
std::stol("42")        // → long
std::stoll("42")       // → long long
std::stoul("42")       // → unsigned long
std::stoull("42")      // → unsigned long long
std::stof("3.14")      // → float
std::stod("3.14")      // → double
std::stold("3.14")     // → long double

// With position (how many chars consumed)
size_t idx;
int n = std::stoi("42abc", &idx);  // idx == 2

// From number
std::to_string(42)      // "42"
std::to_string(3.14)    // "3.140000"

// Case conversion (no stdlib — use loop or <cctype>)
for (char& c : s) c = std::toupper(static_cast<unsigned char>(c));
```

---

## `std::stringstream`

```cpp
#include <sstream>

// Building strings
std::ostringstream oss;
oss << "value=" << 42 << " pi=" << 3.14;
std::string result = oss.str();

// Parsing strings
std::istringstream iss("10 20 30");
int a, b, c;
iss >> a >> b >> c;   // a=10, b=20, c=30

// General stringstream
std::stringstream ss;
ss << "hello " << 123;
std::string s = ss.str();
ss.str("");          // clear content
ss.clear();          // reset error flags

// Tokenise by delimiter
std::string line = "one,two,three";
std::istringstream tok(line);
std::string token;
while (std::getline(tok, token, ',')) {
    std::cout << token << '\n';
}
```

---

## `std::string_view` (C++17)

Non-owning, read-only view over a contiguous char sequence. Zero-copy.

```cpp
#include <string_view>

std::string_view sv1 = "hello world";      // view of string literal
std::string str = "hello";
std::string_view sv2 = str;                // view of std::string

// Same API as string (read-only)
sv1.size();   sv1.empty();   sv1[0];   sv1.front();   sv1.back();
sv1.substr(0, 5);        // returns string_view (no allocation!)
sv1.find("world");       // returns pos or npos
sv1.starts_with("he");   // C++20
sv1.ends_with("lo");     // C++20
sv1.remove_prefix(6);    // advance start by 6
sv1.remove_suffix(1);    // shrink end by 1

// Convert to std::string when needed
std::string owned(sv1);          // explicit copy
std::string owned2{sv1};         // same

// Preferred for read-only parameters
void process(std::string_view sv);  // accepts string, string literal, char*
```

### `string_view` vs `const string&`

| | `const std::string&` | `std::string_view` |
|-|---------------------|--------------------|
| Accepts `std::string` | Yes (no copy) | Yes (no copy) |
| Accepts `const char*` | Yes (allocates!) | Yes (no alloc) |
| Accepts substring | No | Yes (via `sv.substr()`) |
| Null-terminated? | Always | Not guaranteed |

---

## Iteration

```cpp
// Range-for
for (char c : s) { }
for (char& c : s) { c = toupper(c); }

// Index
for (size_t i = 0; i < s.size(); ++i) { s[i]; }

// Iterators
for (auto it = s.begin(); it != s.end(); ++it) { *it; }
for (auto it = s.rbegin(); it != s.rend(); ++it) { }  // reverse
```

---

## Common Pitfalls

```cpp
// 1. std::string::npos not checked
size_t pos = s.find("x");
char c = s[pos];   // UB if not found: pos = SIZE_MAX

// 2. string_view dangling — backing string destroyed
std::string_view bad() {
    std::string s = "temp";
    return s;   // UB: s destroyed, view dangles
}

// 3. string_view not null-terminated — can't pass to C API
std::string_view sv = "hello world";
sv = sv.substr(0, 5);
// printf("%s", sv.data()); // DANGER: data() may not be null-terminated
// Fix: printf("%.*s", (int)sv.size(), sv.data());  // explicit length

// 4. Iterator invalidation on modification
auto it = s.begin();
s.push_back('!');   // may reallocate; it is now invalid

// 5. += with single char — use push_back
s += 'x';      // OK in modern compilers
s.push_back('x');  // slightly clearer

// 6. SSO (Small String Optimisation)
// Short strings (typically <=15 chars) stored inline in the string object itself.
// Taking &s[0] is valid, but if SSO is active, moving the string object to a new
// address (e.g., into a vector on reallocation) relocates that inline buffer,
// leaving any raw pointer taken before the move dangling.

// 7. stoi/stod throw on invalid input
try {
    int n = std::stoi("abc");   // throws std::invalid_argument
} catch (const std::exception& e) { }

// 8. to_string(double) — limited precision
std::to_string(1.0/3.0);  // "0.333333" — use stringstream for control
```

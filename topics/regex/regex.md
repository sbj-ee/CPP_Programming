# std::regex — Cheat Sheet

## Headers & Construction

```cpp
#include <regex>
#include <string>

// Construction — EXPENSIVE; compile once, reuse many times
std::regex re("pattern");
std::regex re(R"(\d{4}-\d{2}-\d{2})");  // raw string: no double-escaping

// Flags (can be OR'd)
std::regex re("hello",
    std::regex::icase       // case-insensitive
  | std::regex::multiline   // ^ and $ match line boundaries
  | std::regex::ECMAScript  // ECMAScript syntax (default)
);
```

### Grammar Flags

| Flag | Grammar |
|------|---------|
| `std::regex::ECMAScript` | ECMAScript/Perl-like (default) |
| `std::regex::basic` | POSIX (Portable Operating System Interface) BRE (Basic Regular Expression) |
| `std::regex::extended` | POSIX ERE (Extended Regular Expression) |
| `std::regex::grep` | POSIX grep |
| `std::regex::egrep` | POSIX egrep |
| `std::regex::awk` | POSIX awk |
| `std::regex::icase` | Case-insensitive |
| `std::regex::multiline` | `^`/`$` match line ends |
| `std::regex::nosubs` | Don't capture groups (faster) |
| `std::regex::optimize` | Hint to optimise matching speed |

---

## `regex_match` vs `regex_search`

```cpp
std::string s = "hello world 42";
std::regex  re(R"(\d+)");

// regex_match — ENTIRE string must match
std::regex_match("12345", re);    // true
std::regex_match("abc12", re);    // false — not full match

// regex_search — find pattern ANYWHERE in string
std::regex_search(s, re);         // true — "42" found
```

---

## `std::smatch` / `std::cmatch`

```cpp
std::string  text = "2024-06-15";
std::regex   date_re(R"((\d{4})-(\d{2})-(\d{2}))");
std::smatch  m;     // for std::string; cmatch for const char*

if (std::regex_search(text, m, date_re)) {
    m[0].str()   // "2024-06-15"  — whole match
    m[1].str()   // "2024"        — group 1
    m[2].str()   // "06"          — group 2
    m[3].str()   // "15"          — group 3
    m.size()     // 4             — 1 + number of groups
    m.prefix().str()  // text before match
    m.suffix().str()  // text after match
    m.position(0)     // offset of whole match in original string
    m.position(1)     // offset of group 1
    m.length(0)       // length of whole match
}

// regex_match fills m too (must match entire string)
if (std::regex_match(text, m, date_re)) { /* ... */ }
```

---

## `regex_replace`

```cpp
std::string s = "foo 2024-06-15 bar";
std::regex  re(R"((\d{4})-(\d{2})-(\d{2}))");

// Format string: $0=whole, $1=group1, $2=group2, ...
std::string result = std::regex_replace(s, re, "$3/$2/$1");
// → "foo 15/06/2024 bar"

// Replace all occurrences
std::string cleaned = std::regex_replace("aaa bbb aaa", std::regex("aaa"), "X");
// → "X bbb X"

// Replace flags
std::regex_replace(s, re, "X",
    std::regex_constants::format_first_only);  // replace only first match

// format_no_copy — only output matched portions (strip unmatched)
// format_sed     — use sed-style replacement ($1 → \1 syntax)
```

---

## Iterating All Matches: `sregex_iterator`

```cpp
std::string text = "one 1, two 22, three 333";
std::regex  re(R"(\d+)");

for (auto it = std::sregex_iterator(text.begin(), text.end(), re);
     it != std::sregex_iterator(); ++it) {
    std::smatch& m = *it;
    std::cout << m[0].str() << '\n';   // "1", "22", "333"
}

// Collect all matches
std::vector<std::string> matches;
auto begin = std::sregex_iterator(text.begin(), text.end(), re);
auto end   = std::sregex_iterator();
for (auto it = begin; it != end; ++it)
    matches.push_back((*it)[0].str());

// Count matches
auto count = std::distance(begin, end);  // 3
```

---

## Splitting: `sregex_token_iterator`

```cpp
std::string s = "one, two,three ,four";
std::regex delim(R"(\s*,\s*)");

// -1 = return the parts BETWEEN matches (the tokens)
std::vector<std::string> tokens(
    std::sregex_token_iterator(s.begin(), s.end(), delim, -1),
    std::sregex_token_iterator()
);
// tokens = {"one", "two", "three", "four"}

// Return specific capture groups instead
std::regex kv(R"((\w+)=(\w+))");
std::string data = "a=1 b=2 c=3";
// index {1,2} = return group 1 then group 2 for each match
for (std::sregex_token_iterator it(data.begin(), data.end(), kv, {1,2});
     it != std::sregex_token_iterator(); ++it) {
    std::cout << *it << ' ';
}
// "a 1 b 2 c 3"
```

---

## ERE / ECMAScript Syntax Reference

| Syntax | Meaning |
|--------|---------|
| `.` | Any char except newline |
| `^` / `$` | Start/end of string (or line with multiline) |
| `*` / `+` / `?` | 0+, 1+, 0-1 (greedy) |
| `*?` / `+?` / `??` | Non-greedy (ECMAScript) |
| `{n}` / `{n,}` / `{n,m}` | Exact / at-least / range repetition |
| `[abc]` / `[^abc]` | Character class / negated |
| `[a-z]` | Range |
| `(...)` | Capture group |
| `(?:...)` | Non-capturing group (ECMAScript) |
| `(?=...)` | Lookahead (ECMAScript) |
| `(?!...)` | Negative lookahead (ECMAScript) |
| `\d` / `\D` | Digit / non-digit |
| `\w` / `\W` | Word char / non-word |
| `\s` / `\S` | Whitespace / non-whitespace |
| `\b` | Word boundary |
| `\1` ... `\9` | Back-reference to group n |
| `a\|b` | Alternation |

---

## Common Patterns

```cpp
// IPv4 address
std::regex ipv4(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");

// ISO date: YYYY-MM-DD
std::regex iso_date(R"(\b(\d{4})-(\d{2})-(\d{2})\b)");

// C identifier
std::regex c_ident(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)");

// Email (simplified)
std::regex email(R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})");

// URL (simplified)
std::regex url(R"(https?://[^\s/$.?#].[^\s]*)");

// Integer literal (C++)
std::regex int_literal(R"([+-]?(?:0x[0-9a-fA-F]+|0[0-7]*|[1-9]\d*))");

// Quoted string (no escaped quotes)
std::regex quoted(R"("([^"]*)")");

// Whitespace-only line
std::regex blank_line(R"(^\s*$)", std::regex::multiline);
```

---

## Exception Handling

```cpp
try {
    std::regex bad("(unclosed");   // throws std::regex_error on bad pattern
}
catch (const std::regex_error& e) {
    std::cerr << "regex error: " << e.what()
              << " code=" << e.code() << '\n';
    // error codes: std::regex_constants::error_*
}

// Error code constants (std::regex_constants::error_xxx):
// error_badbrace, error_badrepeat, error_brace,
// error_brack, error_collate, error_complexity,
// error_ctype, error_escape, error_paren, error_range,
// error_space, error_stack, error_back, error_badrepeat
```

---

## Pitfalls

```cpp
// 1. Constructing regex in a loop — very expensive
for (auto& s : data) {
    std::regex re("pattern");   // BAD: compiles regex every iteration
    std::regex_search(s, re);
}
// Fix:
static const std::regex re("pattern");  // compile once
for (auto& s : data) { std::regex_search(s, re); }

// 2. std::regex is NOT constexpr — cannot use in constexpr context
// No compile-time regex in standard C++ (use CTRE library for constexpr)

// 3. Catastrophic backtracking
std::regex bad(R"((a+)+b)");
std::regex_search("aaaaaaaaaaaaaaac", bad);  // exponential time!
// Avoid: nested quantifiers over the same chars; use atomic groups (not in std::regex)

// 4. raw string literals for backslashes — use R"(...)"
std::regex r1("\\d+");          // OK: \\ in regular string = one backslash
std::regex r2(R"(\d+)");        // Better: no escaping needed

// 5. regex_match requires FULL string match — easy to confuse with regex_search
std::regex re("\\d+");
std::regex_match("abc123", re);    // FALSE — whole string not just digits
std::regex_search("abc123", re);   // TRUE — digits found within string

// 6. smatch becomes invalid when source string is destroyed
std::smatch m;
{
    std::string s = "hello 42";
    std::regex_search(s, m, std::regex("\\d+"));
}
m[0].str();   // UB: s destroyed; m references s's data

// 7. Thread safety: regex objects are safe to share across threads (read-only)
// smatch objects must NOT be shared — one per thread

// 8. std::regex performance
// std::regex is notably slow in many implementations (especially libstdc++ prior to GCC 14)
// Alternatives: RE2, PCRE2, Hyperscan, or compile-time CTRE for production use
```

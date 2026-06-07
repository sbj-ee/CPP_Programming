// =============================================================================
// Exercise 32: C++ Regular Expressions (<regex>)
// =============================================================================
// Topics: std::regex / std::smatch / std::cmatch, ERE (Extended Regular Expression) by default,
//         regex_search, capture groups, regex_replace, flags (icase, multiline),
//         sregex_iterator scan loop, common patterns (IPv4, ISO date, C ident)
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g regex_demo.cpp -o regex_demo
// =============================================================================

#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <iomanip>

// =============================================================================
// SECTION 1: Concepts — std::regex, ERE, smatch vs cmatch
// =============================================================================

static void section1_concepts() {
    std::cout << "\n=== Section 1: C++ <regex> concepts ===\n";

    std::cout << "  std::regex      — compiled regular expression pattern.\n";
    std::cout << "  std::smatch     — match results against std::string.\n";
    std::cout << "  std::cmatch     — match results against const char*.\n";
    std::cout << "  std::ssub_match — single capture group result (sub-match).\n\n";

    std::cout << "  Default syntax: ECMAScript (similar to ERE but with extensions).\n";
    std::cout << "  Other syntaxes: std::regex::extended (POSIX ERE),\n";
    std::cout << "                  std::regex::grep, std::regex::awk\n\n";

    std::cout << "  Key free functions:\n";
    std::cout << "    regex_search(str, match, re) — find pattern anywhere in str\n";
    std::cout << "    regex_match(str, match, re)  — entire str must match pattern\n";
    std::cout << "    regex_replace(str, re, fmt)  — replace all matches\n\n";

    std::cout << "  Iterators:\n";
    std::cout << "    std::sregex_iterator — iterate over all non-overlapping matches\n";
    std::cout << "    std::sregex_token_iterator — split on pattern\n";

    // Compile a simple pattern and check
    std::regex pat("hello", std::regex::icase);
    std::string text = "Say Hello!";
    bool found = std::regex_search(text, pat);
    std::cout << "\n  Quick demo: regex_search(\"Say Hello!\", /hello/i) = "
              << (found ? "true" : "false") << "\n";
}

// =============================================================================
// SECTION 2: Basic match / no-match with regex_search
// =============================================================================

static void section2_search() {
    std::cout << "\n=== Section 2: regex_search — match / no-match ===\n";

    struct TestCase {
        std::string pattern;
        std::string input;
    };

    const std::vector<TestCase> cases = {
        { R"(\d+)",         "order 42 arrived"              },
        { R"(\d+)",         "no digits here"                },
        { R"(\b\w{5}\b)",   "find five-letter words please" },
        { R"(foo.*bar)",    "foo hello bar baz"             },
        { R"(foo.*bar)",    "foo hello baz"                 },
        { R"(^start)",      "start of line"                 },
        { R"(^start)",      "not at start"                  },
    };

    for (const auto& tc : cases) {
        std::regex re(tc.pattern);
        std::smatch m;
        bool found = std::regex_search(tc.input, m, re);
        std::cout << "  pattern=" << std::setw(14) << std::left << tc.pattern
                  << "  input=\"" << tc.input << "\"\n"
                  << "    -> " << (found ? "MATCH" : "no match");
        if (found) std::cout << " (\"" << m[0].str() << "\")";
        std::cout << "\n";
    }
}

// =============================================================================
// SECTION 3: Capture groups with std::smatch
// =============================================================================
//
// Parentheses create capture groups.  match[0] is the whole match;
// match[1], match[2], ... are the captured sub-expressions.

static void section3_captures() {
    std::cout << "\n=== Section 3: Capture groups with std::smatch ===\n";

    // Parse "YYYY-MM-DD" date
    std::regex date_re(R"((\d{4})-(\d{2})-(\d{2}))");
    std::string text = "Project deadline: 2026-06-15 and review on 2026-07-01";

    auto it  = std::sregex_iterator(text.begin(), text.end(), date_re);
    auto end = std::sregex_iterator{};
    for (; it != end; ++it) {
        const std::smatch& m = *it;
        std::cout << "  Full match: " << m[0].str()
                  << "  year=" << m[1].str()
                  << "  month=" << m[2].str()
                  << "  day=" << m[3].str() << "\n";
    }

    // Named-style extraction: HTTP status line
    std::regex http_re(R"(HTTP/(\d\.\d) (\d{3}) (.+))");
    const std::string status = "HTTP/1.1 404 Not Found";
    std::smatch sm;
    if (std::regex_search(status, sm, http_re)) {
        std::cout << "\n  HTTP parse:\n";
        std::cout << "    version=" << sm[1].str() << "\n";
        std::cout << "    code="    << sm[2].str() << "\n";
        std::cout << "    reason="  << sm[3].str() << "\n";
    }

    // Email (simplified — not RFC-5321 complete)
    std::regex email_re(R"(([a-zA-Z0-9._%+-]+)@([a-zA-Z0-9.-]+\.[a-zA-Z]{2,}))");
    std::string emails = "Contact alice@example.com or bob@corp.org for info";
    auto eit = std::sregex_iterator(emails.begin(), emails.end(), email_re);
    std::cout << "\n  Emails found:\n";
    for (; eit != std::sregex_iterator{}; ++eit) {
        const std::smatch& m = *eit;
        std::cout << "    user=" << m[1].str() << "  domain=" << m[2].str() << "\n";
    }
}

// =============================================================================
// SECTION 4: regex_replace
// =============================================================================
//
// regex_replace returns a new string with all matches replaced.
// The replacement format string can reference capture groups:
//   $& — entire match
//   $1, $2, ... — capture group 1, 2, ...
//   $` — text before match; $' — text after match

static void section4_replace() {
    std::cout << "\n=== Section 4: regex_replace ===\n";

    // Redact credit card numbers (16 digits, optionally grouped)
    {
        std::regex cc_re(R"(\b(\d{4})[- ]?(\d{4})[- ]?(\d{4})[- ]?(\d{4})\b)");
        std::string text = "Card: 4111 1111 1111 1111 and 5500000000000004 confirmed.";
        std::string redacted = std::regex_replace(text, cc_re, "****-****-****-$4");
        std::cout << "  Original:  " << text << "\n";
        std::cout << "  Redacted:  " << redacted << "\n";
    }

    // Normalize whitespace (collapse multiple spaces to one)
    {
        std::regex ws_re(R"(\s{2,})");
        std::string text = "too   many   spaces   here";
        std::string norm = std::regex_replace(text, ws_re, " ");
        std::cout << "\n  Whitespace norm: \"" << norm << "\"\n";
    }

    // Swap first and last name (e.g., "Doe, Jane" -> "Jane Doe")
    {
        std::regex name_re(R"((\w+),\s*(\w+))");
        std::string text = "Doe, Jane; Smith, John; Jones, Alice";
        std::string swapped = std::regex_replace(text, name_re, "$2 $1");
        std::cout << "  Name swap: \"" << swapped << "\"\n";
    }
}

// =============================================================================
// SECTION 5: Flags — icase, multiline
// =============================================================================
//
// Flags are passed as the second argument to the std::regex constructor.
// Common flags:
//   std::regex::icase      — case-insensitive matching
//   std::regex::multiline  — ^ and $ match start/end of each line (not just string)
//   std::regex::ECMAScript — (default) ECMAScript syntax
//   std::regex::extended   — POSIX ERE syntax

static void section5_flags() {
    std::cout << "\n=== Section 5: regex flags (icase, multiline) ===\n";

    // --- icase ---
    std::regex re_icase("hello", std::regex::icase);
    const std::vector<std::string> inputs = {
        "hello world", "HELLO!", "Hello, C++", "no greeting here"
    };
    std::cout << "  icase /hello/:\n";
    for (const auto& s : inputs)
        std::cout << "    \"" << s << "\" -> "
                  << (std::regex_search(s, re_icase) ? "match" : "no match") << "\n";

    // --- multiline: ^ matches start of each line ---
    std::regex re_ml("^\\w+", std::regex::multiline);
    std::string text = "first line\nsecond line\nthird line";
    std::cout << "\n  multiline /^\\w+/ (first word of each line):\n";
    auto it  = std::sregex_iterator(text.begin(), text.end(), re_ml);
    for (; it != std::sregex_iterator{}; ++it)
        std::cout << "    \"" << (*it)[0].str() << "\"\n";

    // --- icase + multiline combined ---
    std::regex re_both("^SECOND", std::regex::icase | std::regex::multiline);
    std::smatch m;
    std::string found_str;
    if (std::regex_search(text, m, re_both))
        found_str = m[0].str();
    std::cout << "\n  icase|multiline /^SECOND/: \""
              << (found_str.empty() ? "(not found)" : found_str) << "\"\n";
}

// =============================================================================
// SECTION 6: sregex_iterator scan + common patterns
// =============================================================================
//
// std::sregex_iterator traverses all non-overlapping matches of a regex in a
// string.  Combine with regex_match (full-string match) to validate input.

static void section6_patterns() {
    std::cout << "\n=== Section 6: Common patterns + sregex_iterator ===\n";

    // IPv4 address (simple heuristic — not fully RFC-correct)
    const std::string IPV4 =
        R"(\b(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\b)";

    // ISO date YYYY-MM-DD
    const std::string ISO_DATE = R"(\b(\d{4})-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])\b)";

    // C identifier
    const std::string C_IDENT = R"([_a-zA-Z][_a-zA-Z0-9]*)";

    // --- Validate with regex_match (entire string must match) ---
    std::cout << "  Validation with regex_match:\n";

    auto validate = [](const std::string& label, const std::string& pat,
                       const std::vector<std::string>& tests) {
        std::regex re(pat);
        std::cout << "    " << label << ":\n";
        for (const auto& t : tests)
            std::cout << "      \"" << t << "\" -> "
                      << (std::regex_match(t, re) ? "valid" : "invalid") << "\n";
    };

    validate("IPv4", IPV4, {"192.168.1.1", "10.0.0.256", "0.0.0.0", "1.2.3"});
    validate("ISO date", ISO_DATE,
             {"2026-06-15", "2026-13-01", "2026-00-01", "26-06-15"});
    validate("C ident", C_IDENT,
             {"my_var", "_private", "2bad", "valid123", "hello world"});

    // --- Scan loop: extract all IPv4 addresses from a log line ---
    std::regex ipv4_re(IPV4);
    std::string log = "Connection from 192.168.0.1 to 10.10.0.5; deny 172.16.254.1";
    std::cout << "\n  IPv4 addresses in log:\n";
    auto it  = std::sregex_iterator(log.begin(), log.end(), ipv4_re);
    for (; it != std::sregex_iterator{}; ++it)
        std::cout << "    " << (*it)[0].str() << "\n";

    // --- Tokenize: split CSV ---
    std::regex comma_re(",\\s*");
    std::string csv = "alpha, beta, gamma, delta";
    std::sregex_token_iterator tok_it(csv.begin(), csv.end(), comma_re, -1);
    std::sregex_token_iterator tok_end;
    std::cout << "\n  CSV tokens from \"" << csv << "\":\n";
    for (; tok_it != tok_end; ++tok_it)
        std::cout << "    \"" << tok_it->str() << "\"\n";

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. Raw string literals R\"(...)\" avoid double-escaping backslashes.\n";
    std::cout << "     \"\\\\d+\" == R\"(\\d+)\" — use raw strings for clarity.\n";
    std::cout << "  2. regex_match requires the ENTIRE string to match; regex_search\n";
    std::cout << "     finds the pattern ANYWHERE in the string.\n";
    std::cout << "  3. Compiling a std::regex is expensive; cache it if called in a loop.\n";
    std::cout << "  4. std::regex::multiline makes ^ and $ match line boundaries.\n";
    std::cout << "     Without it, ^ and $ match only start/end of the full string.\n";
    std::cout << "  5. std::regex may throw std::regex_error on a bad pattern.\n";
    std::cout << "     Wrap regex construction in try/catch for user-provided patterns.\n";
    std::cout << "  6. C++ <regex> performance is often slower than PCRE2 or RE2;\n";
    std::cout << "     for very hot paths prefer those libraries.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 32: C++ Regular Expressions ===\n";

    section1_concepts();
    section2_search();
    section3_captures();
    section4_replace();
    section5_flags();
    section6_patterns();

    std::cout << "\nDone.\n";
    return 0;
}

// =============================================================================
// Exercise 33: C++ <filesystem> (C++17)
// =============================================================================
// Topics: path operations, file status, directory iteration,
//         create/remove/copy, temp files, error_code vs exceptions
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g filesystem_demo.cpp -o filesystem_demo
//        (add -lstdc++fs on older GCC < 9 if needed)
// =============================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <system_error>
#include <chrono>
#include <iomanip>
#include <cstdio>       // std::tmpnam (controlled use — see section 5)
#include <map>

#include <filesystem>

namespace fs = std::filesystem;

// =============================================================================
// SECTION 1: Concepts — std::filesystem::path, portable separators
// =============================================================================
//
// std::filesystem::path is a portable representation of a file system path.
// It normalises separators ('/' on POSIX, '\\' on Windows) and provides
// components (root, parent, filename, stem, extension).
//
// Constructing a path from a string is cheap; no OS calls are made.

static void section1_concepts() {
    std::cout << "\n=== Section 1: std::filesystem::path concepts ===\n";

    fs::path p("/home/user/docs/report.txt");

    std::cout << "  path:        " << p                << "\n";
    std::cout << "  root_path:   " << p.root_path()    << "\n";
    std::cout << "  root_name:   " << p.root_name()    << "\n";
    std::cout << "  parent_path: " << p.parent_path()  << "\n";
    std::cout << "  filename:    " << p.filename()     << "\n";
    std::cout << "  stem:        " << p.stem()         << "\n";
    std::cout << "  extension:   " << p.extension()    << "\n";

    std::cout << "\n  Iterate components:\n";
    for (const auto& part : p)
        std::cout << "    " << part << "\n";

    // portable_string returns forward-slashes on all platforms
    std::cout << "\n  Portable string: " << p.generic_string() << "\n";

    // Path concatenation with /=
    fs::path base("/tmp");
    base /= "myapp";
    base /= "data.bin";
    std::cout << "\n  Concatenated path: " << base << "\n";

    // Comparing paths
    fs::path a("/tmp/a/../b");
    fs::path b("/tmp/b");
    std::cout << "  /tmp/a/../b == /tmp/b ?  "
              << (a == b ? "yes" : "no")                           // not equal: no lexical normalisation
              << "  (lexical; use lexically_normal() to normalize)\n";
    std::cout << "  lexically_normal: "
              << a.lexically_normal() << "\n";
}

// =============================================================================
// SECTION 2: Path operations
// =============================================================================

static void section2_path_ops() {
    std::cout << "\n=== Section 2: Path operations ===\n";

    fs::path p = fs::current_path();
    std::cout << "  current_path:  " << p << "\n";

    // Append vs concat
    fs::path base("/var/log");
    fs::path appended  = base / "myapp" / "app.log";   // /= inserts separator
    fs::path concat_op = base;
    concat_op += "_old";                                // += does NOT insert separator

    std::cout << "  base / \"myapp/app.log\":    " << appended  << "\n";
    std::cout << "  base += \"_old\":             " << concat_op << "\n";

    // Replacing filename / extension
    fs::path f("/etc/nginx/nginx.conf");
    std::cout << "\n  Original:                  " << f << "\n";
    std::cout << "  replace_extension(.bak):   " << fs::path(f).replace_extension(".bak") << "\n";
    std::cout << "  replace_filename(new.conf): " << fs::path(f).replace_filename("new.conf") << "\n";

    // absolute / relative
    fs::path rel("exercises/33_filesystem");
    std::cout << "\n  relative:  " << rel << "\n";
    std::cout << "  absolute:  " << fs::absolute(rel) << "\n";

    // Checking emptiness
    fs::path empty;
    std::cout << "\n  empty path.empty(): " << empty.empty() << "\n";
}

// =============================================================================
// SECTION 3: File status — exists, is_directory, file_size, last_write_time
// =============================================================================

static void section3_status() {
    std::cout << "\n=== Section 3: File status ===\n";

    struct Check {
        fs::path   path;
        const char* label;
    };

    const std::vector<Check> checks = {
        { "/tmp",             "tmp dir"          },
        { "/etc/hostname",    "/etc/hostname"     },
        { "/nonexistent_xyz", "nonexistent"       },
    };

    for (const auto& c : checks) {
        std::error_code ec;
        bool ex  = fs::exists(c.path, ec);
        bool dir = ex && fs::is_directory(c.path, ec);
        bool reg = ex && fs::is_regular_file(c.path, ec);

        std::cout << "  " << c.label << " (" << c.path << "):\n";
        std::cout << "    exists=" << ex << "  is_dir=" << dir
                  << "  is_file=" << reg;

        if (reg) {
            uintmax_t sz = fs::file_size(c.path, ec);
            std::cout << "  size=" << sz << "B";

            // last_write_time returns a file_time_type (std::chrono)
            auto lwt = fs::last_write_time(c.path, ec);
            // Convert to system_clock time_point for display
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                lwt - fs::file_time_type::clock::now()
                    + std::chrono::system_clock::now());
            std::time_t t = std::chrono::system_clock::to_time_t(sctp);
            char tbuf[32];
            std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            std::cout << "  mtime=" << tbuf;
        }
        std::cout << "\n";
    }

    // file_type enum
    std::cout << "\n  file_type of /dev/null: ";
    fs::file_status st = fs::status("/dev/null");
    switch (st.type()) {
        case fs::file_type::character: std::cout << "character device\n"; break;
        case fs::file_type::regular:   std::cout << "regular file\n";     break;
        case fs::file_type::directory: std::cout << "directory\n";        break;
        default:                       std::cout << "other\n";             break;
    }
}

// =============================================================================
// SECTION 4: Directory iteration
// =============================================================================
//
// directory_iterator: non-recursive; yields one entry per call.
// recursive_directory_iterator: descends into subdirectories.
// Both yield fs::directory_entry objects.

static void section4_iteration() {
    std::cout << "\n=== Section 4: Directory iteration ===\n";

    // Non-recursive: list /tmp entries (limit output for sanity)
    {
        std::cout << "  /tmp entries (first 8):\n";
        int count = 0;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator("/tmp", ec)) {
            if (count++ >= 8) { std::cout << "    ...\n"; break; }
            std::string type = entry.is_directory()    ? "dir" :
                               entry.is_regular_file() ? "file" : "other";
            std::cout << "    [" << type << "] " << entry.path().filename() << "\n";
        }
    }

    // Create a small sandbox tree for recursive demo
    fs::path sandbox = fs::temp_directory_path() / "ex33_sandbox";
    std::error_code ec;
    fs::remove_all(sandbox, ec);
    fs::create_directories(sandbox / "a/b", ec);
    fs::create_directories(sandbox / "a/c", ec);
    fs::create_directories(sandbox / "d",   ec);

    // Create some files
    for (const char* rel : {"a/b/file1.txt", "a/c/file2.cpp", "d/file3.md"}) {
        std::ofstream ofs(sandbox / rel);
        ofs << "content\n";
    }

    std::cout << "\n  Recursive listing of sandbox " << sandbox << ":\n";
    for (const auto& entry : fs::recursive_directory_iterator(sandbox, ec)) {
        // Compute depth from difference in path components
        int depth = static_cast<int>(std::distance(sandbox.begin(), sandbox.end()));
        int edepth = static_cast<int>(std::distance(entry.path().begin(), entry.path().end()));
        std::string indent(static_cast<size_t>(2 * (edepth - depth)), ' ');
        std::string type = entry.is_directory() ? "[dir] " : "[file]";
        std::cout << "  " << indent << type << " " << entry.path().filename() << "\n";
    }

    // Count files by extension
    std::cout << "\n  Counting files by extension:\n";
    std::map<std::string, int> ext_count;
    for (const auto& entry : fs::recursive_directory_iterator(sandbox, ec)) {
        if (entry.is_regular_file())
            ++ext_count[entry.path().extension().string()];
    }
    for (const auto& [ext, cnt] : ext_count)
        std::cout << "    \"" << ext << "\" -> " << cnt << "\n";

    fs::remove_all(sandbox, ec);
}

// =============================================================================
// SECTION 5: Create / remove / copy + temp files
// =============================================================================

static void section5_create_remove_copy() {
    std::cout << "\n=== Section 5: Create, remove, copy, temp files ===\n";

    fs::path tmp = fs::temp_directory_path();
    std::cout << "  temp_directory_path(): " << tmp << "\n";

    // Create a hierarchy
    fs::path sandbox = tmp / "ex33_ops";
    std::error_code ec;
    fs::remove_all(sandbox, ec);

    bool ok = fs::create_directories(sandbox / "sub1" / "deep", ec);
    std::cout << "  create_directories(" << sandbox / "sub1/deep" << "): "
              << (ok ? "ok" : ("failed: " + ec.message())) << "\n";

    // Write a file
    fs::path src = sandbox / "src.txt";
    { std::ofstream ofs(src); ofs << "Hello filesystem!\n"; }
    std::cout << "  Created file: " << src << "\n";
    std::cout << "  File size:    " << fs::file_size(src) << " bytes\n";

    // Copy file
    fs::path dst = sandbox / "dst.txt";
    fs::copy(src, dst, fs::copy_options::overwrite_existing, ec);
    std::cout << "  Copied to:    " << dst << "  (exists=" << fs::exists(dst) << ")\n";

    // Remove a single file
    fs::remove(src, ec);
    std::cout << "  remove(src):  src exists=" << fs::exists(src) << "\n";

    // Temp file alternative: use mkstemp (POSIX) or generate unique path
    fs::path tmpfile = tmp / fs::path("ex33_tmp_XXXXXX");
    {
        // std::tmpnam is deprecated; use a combination of temp_directory_path +
        // unique name.  Here we simulate with a fixed name for the demo.
        std::string name = (tmp / "ex33_tmpfile.dat").string();
        std::ofstream ofs(name);
        ofs << "temporary data\n";
        std::cout << "  Temp file: " << name << "\n";
        fs::remove(name, ec);
        std::cout << "  After remove, exists=" << fs::exists(name) << "\n";
    }

    // remove_all: recursively delete
    uintmax_t removed = fs::remove_all(sandbox, ec);
    std::cout << "  remove_all(" << sandbox << "): "
              << removed << " entries removed\n";
}

// =============================================================================
// SECTION 6: Error handling — error_code overloads vs exceptions
// =============================================================================
//
// Every filesystem function has two overloads:
//   (a) Throws std::filesystem::filesystem_error on failure.
//   (b) Takes a std::error_code& argument; sets it on failure instead of throwing.
//
// Use (a) in code where exceptions are acceptable.
// Use (b) in performance-critical or noexcept code.

static void section6_error_handling() {
    std::cout << "\n=== Section 6: Error handling — error_code vs exceptions ===\n";

    // --- Exception-based ---
    std::cout << "  Exception-based:\n";
    try {
        [[maybe_unused]] auto sz2 = fs::file_size("/this/path/does/not/exist");  // throws
        std::cout << "  ERROR: should not reach here\n";
    } catch (const fs::filesystem_error& e) {
        std::cout << "  Caught filesystem_error: " << e.what() << "\n";
        std::cout << "  path1: " << e.path1() << "\n";
        std::cout << "  error_code: " << e.code().message() << "\n";
    }

    // --- error_code-based (no throw) ---
    std::cout << "\n  error_code-based (noexcept style):\n";
    std::error_code ec;
    uintmax_t sz = fs::file_size("/nonexistent", ec);
    if (ec) {
        std::cout << "  file_size failed: " << ec.message()
                  << " (value=" << ec.value() << ")\n";
        std::cout << "  returned value: " << sz << " (use is_error to detect)\n";
    }

    // Checking multiple operations
    std::cout << "\n  Chained error_code checks:\n";
    fs::path p = fs::temp_directory_path() / "ex33_test";
    fs::remove_all(p, ec);

    fs::create_directory(p, ec);
    std::cout << "    create_directory: " << (ec ? ec.message() : "ok") << "\n";

    fs::copy_file("/etc/hostname", p / "hostname", ec);
    std::cout << "    copy_file:        " << (ec ? ec.message() : "ok") << "\n";

    auto size = fs::file_size(p / "hostname", ec);
    std::cout << "    file_size:        " << (ec ? ec.message() : std::to_string(size) + "B") << "\n";

    fs::remove_all(p, ec);

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. path comparison is lexicographic, not canonical.\n";
    std::cout << "     /tmp/a/../b != /tmp/b  without lexically_normal() or canonical().\n";
    std::cout << "     canonical() resolves symlinks and requires the path to exist.\n";
    std::cout << "  2. directory_iterator order is unspecified (filesystem order).\n";
    std::cout << "     Sort entries explicitly if order matters.\n";
    std::cout << "  3. file_size is undefined for directories; use error_code overload.\n";
    std::cout << "  4. recursive_directory_iterator skips permission-denied dirs by\n";
    std::cout << "     default; pass directory_options::skip_permission_denied.\n";
    std::cout << "  5. last_write_time uses file_clock (C++20 feature) — converting\n";
    std::cout << "     to system_clock may require clock_cast or arithmetic workaround.\n";
    std::cout << "  6. On GCC < 9 you may need to link -lstdc++fs explicitly.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 33: C++ <filesystem> ===\n";

    section1_concepts();
    section2_path_ops();
    section3_status();
    section4_iteration();
    section5_create_remove_copy();
    section6_error_handling();

    std::cout << "\nDone.\n";
    return 0;
}

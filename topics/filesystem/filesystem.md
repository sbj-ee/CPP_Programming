# std::filesystem (C++17)

`#include <filesystem>` — no extra defines needed.  
Link: usually built into libstdc++/libc++ with `-std=c++17`; on older GCC add `-lstdc++fs`.

```cpp
namespace fs = std::filesystem;
```

---

## std::filesystem::path

```cpp
fs::path p = "/home/user/docs/report.txt";

p.root_path()    // "/"
p.root_name()    // "" (non-empty on Windows: "C:")
p.parent_path()  // "/home/user/docs"
p.filename()     // "report.txt"
p.stem()         // "report"
p.extension()    // ".txt"
p.is_absolute()  // true
p.is_relative()  // false

// Build paths
fs::path q = fs::path("/home") / "user" / "file.txt";
q /= "sub";                          // append component in-place

// Convert to string
std::string s = p.string();          // native encoding
std::string g = p.generic_string();  // forward slashes everywhere
```

### Iterating path components

```cpp
for (const auto& part : p)
    std::cout << part << "\n";   // "/", "home", "user", "docs", "report.txt"
```

---

## Status queries

```cpp
fs::path p = "myfile.txt";

fs::exists(p)              // true if path exists (any type)
fs::is_regular_file(p)     // true for ordinary files
fs::is_directory(p)        // true for directories
fs::is_symlink(p)          // true for symlinks
fs::file_size(p)           // size in bytes (regular file only)
fs::last_write_time(p)     // fs::file_time_type (C++ clock type)

// Error-code overloads (no exception on failure)
std::error_code ec;
bool ok = fs::exists(p, ec);
if (ec) std::cerr << ec.message() << "\n";
```

### file_time_type → readable

```cpp
auto ftime = fs::last_write_time(p);
// Pre-C++20 workaround: file_clock and system_clock are different;
// subtract file_clock::now() and add system_clock::now() to convert.
// Note: the two ::now() calls happen at different instants — slight drift possible.
// C++20 alternative: std::chrono::clock_cast<std::chrono::system_clock>(ftime)
auto sctp  = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                 ftime - fs::file_time_type::clock::now()
                       + std::chrono::system_clock::now());
std::time_t t = std::chrono::system_clock::to_time_t(sctp);
std::cout << std::ctime(&t);
```

---

## Create / remove / copy

```cpp
fs::create_directory("newdir");            // one level; fails if parent missing
fs::create_directories("a/b/c");           // all intermediate dirs

fs::remove("file.txt");                    // remove file or empty dir
fs::remove_all("dir/");                    // recursive delete; returns count

fs::copy("src.txt", "dst.txt");                        // copy file
fs::copy("src/", "dst/", fs::copy_options::recursive); // copy directory tree
// fs::copy_options values:
//   none                — fail if dest exists (default)
//   skip_existing       — silently skip
//   overwrite_existing  — overwrite
//   update_existing     — overwrite only if src newer
//   recursive           — recurse into subdirs

fs::rename("old.txt", "new.txt");          // move / rename (same filesystem = atomic)
```

---

## Directory iteration

```cpp
// Non-recursive
for (const fs::directory_entry& e : fs::directory_iterator("mydir")) {
    std::cout << e.path() << "  " << e.file_size() << "\n";
}

// Recursive
for (const auto& e : fs::recursive_directory_iterator("mydir")) {
    std::cout << std::string(e.depth() * 2, ' ') << e.path().filename() << "\n";
}

// Sorted listing
std::vector<fs::path> entries(fs::directory_iterator("mydir"), {});
std::sort(entries.begin(), entries.end());
```

### directory_entry members

```cpp
e.path()           // fs::path
e.is_regular_file()
e.is_directory()
e.file_size()      // cached — no extra syscall if iterator populated it
e.last_write_time()
e.symlink_status() // status of the link itself (not target)
```

---

## Temporary files and paths

```cpp
fs::path tmp = fs::temp_directory_path();   // /tmp on Linux

// Create a unique temp file (C++17 has no std::tmpfile equivalent)
// Common approach: generate a name with random suffix
fs::path tmp_file = tmp / ("myapp_" + std::to_string(getpid()) + ".tmp");
std::ofstream f(tmp_file);
// ... use ...
fs::remove(tmp_file);
```

---

## Absolute / canonical / relative

```cpp
fs::path abs = fs::absolute("relative/path");   // prepend cwd
fs::path can = fs::canonical("../a/b/../c");     // resolve all symlinks and ".."; path must exist
fs::path rel = fs::relative("/a/b/c", "/a");     // "b/c"
fs::path prox = fs::proximate("/a/b", "/a/x");   // "../b" (like relative but no throw if can't make relative)
```

---

## Disk space

```cpp
fs::space_info si = fs::space("/");
std::cout << "capacity: " << si.capacity  / 1e9 << " GB\n";
std::cout << "free:     " << si.free      / 1e9 << " GB\n";
std::cout << "available:" << si.available / 1e9 << " GB\n"; // free minus reserved
```

---

## Error handling

| Style | How |
|-------|-----|
| Exception (default) | `fs::remove("x")` throws `fs::filesystem_error` on failure |
| Error-code overload | `fs::remove("x", ec)` sets `ec`; never throws |

```cpp
try {
    fs::remove_all("nonexistent");   // no-op; returns 0
    fs::canonical("no_such_path");   // throws filesystem_error
} catch (const fs::filesystem_error& e) {
    std::cerr << e.what() << "\n";       // human-readable
    std::cerr << e.path1() << "\n";      // first path involved
    std::cerr << e.code() << "\n";       // std::error_code
}

// Error-code form — avoids exception overhead in hot paths
std::error_code ec;
fs::remove_all("dir", ec);
if (ec) { /* handle */ }
```

---

## Common pitfalls

| Pitfall | Fix |
|---------|-----|
| `fs::canonical` on non-existent path | Use `fs::weakly_canonical` instead (doesn't require existence) |
| `file_size` on directory | Returns 0 or implementation-defined; don't rely on it |
| Symlink vs target confusion | `is_symlink` + `read_symlink`; `status` follows links, `symlink_status` doesn't |
| TOCTOU (Time-Of-Check Time-Of-Use): check then act | Prefer error-code overloads and handle errors, not pre-checking |
| `remove_all` on wrong path | Double-check `is_directory` and review path before calling |
| Platform path separators | Use `generic_string()` for portable display; `string()` for OS APIs |
| Old GCC requires `-lstdc++fs` | GCC 9+ includes it in the default link; GCC 8 needs explicit flag |
| `last_write_time` clock type | Not `system_clock`; convert via duration arithmetic to get `time_t` |

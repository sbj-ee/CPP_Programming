# File I/O (fstream) — Cheat Sheet

## Stream Classes

| Class | Direction | Header |
|-------|-----------|--------|
| `std::ifstream` | Input (read) | `<fstream>` |
| `std::ofstream` | Output (write) | `<fstream>` |
| `std::fstream` | Input + Output | `<fstream>` |
| `std::istringstream` | Input from string | `<sstream>` |
| `std::ostringstream` | Output to string | `<sstream>` |
| `std::stringstream` | I/O on string | `<sstream>` |

---

## Open Modes

| Flag | Meaning |
|------|---------|
| `ios::in` | Open for reading |
| `ios::out` | Open for writing (creates if absent) |
| `ios::app` | Append; seek to end before each write |
| `ios::ate` | Seek to end on open (but can write anywhere) |
| `ios::trunc` | Truncate to zero on open |
| `ios::binary` | Binary mode (no newline translation) |

```cpp
// Default modes:
std::ifstream  → ios::in
std::ofstream  → ios::out | ios::trunc
std::fstream   → (no default — must specify)

// Combinations:
std::ofstream f("log.txt", std::ios::out | std::ios::app);
std::fstream  g("data.bin", std::ios::in | std::ios::out | std::ios::binary);
```

---

## Opening and Checking

```cpp
#include <fstream>
#include <stdexcept>

// Open in constructor (preferred — RAII)
std::ifstream in("data.txt");
if (!in.is_open()) {
    throw std::runtime_error("Cannot open data.txt");
}

// Or open later
std::ofstream out;
out.open("out.txt", std::ios::out | std::ios::trunc);
if (!out) { /* checks fail/bad state; not always same as !is_open() */ }

// File closed automatically by destructor — no need for explicit close()
// But you can close early:
in.close();
```

---

## Text Mode: Read

```cpp
std::ifstream in("input.txt");

// Read single values
int n; double d; std::string word;
in >> n >> d >> word;   // whitespace-delimited

// Read full line
std::string line;
while (std::getline(in, line)) {
    // process line
}

// Read all content into string
std::string content(
    std::istreambuf_iterator<char>(in),
    std::istreambuf_iterator<char>()
);

// Read char by char
char c;
while (in.get(c)) { /* c */ }
in.peek();     // peek at next char without consuming
in.unget();    // put back last char
in.putback(c); // put back specific char
```

---

## Text Mode: Write

```cpp
std::ofstream out("output.txt");

out << "Hello, " << "world!\n";
out << 42 << ' ' << 3.14 << '\n';

// Formatting
#include <iomanip>
out << std::setw(10) << std::left  << "Name"
    << std::setw(6)  << std::right << "Score" << '\n';
out << std::fixed << std::setprecision(2) << 3.14159 << '\n';
out << std::hex << 255 << '\n';         // "ff"
out << std::boolalpha << true << '\n';  // "true"
```

---

## Binary Mode

```cpp
std::ofstream out("data.bin", std::ios::binary);

// Write raw bytes
int val = 0x12345678;
out.write(reinterpret_cast<const char*>(&val), sizeof(val));

// Write array
double arr[4] = {1.0, 2.0, 3.0, 4.0};
out.write(reinterpret_cast<const char*>(arr), sizeof(arr));

// Read raw bytes
std::ifstream in("data.bin", std::ios::binary);
int v;
in.read(reinterpret_cast<char*>(&v), sizeof(v));
if (in.gcount() != sizeof(v)) { /* short read */ }

double buf[4];
in.read(reinterpret_cast<char*>(buf), sizeof(buf));

// Read struct
struct Header { uint32_t magic; uint32_t version; };
Header h;
in.read(reinterpret_cast<char*>(&h), sizeof(h));
```

---

## Seeking

```cpp
// Read position: seekg / tellg
in.seekg(0);                          // go to beginning
in.seekg(0, std::ios::end);           // go to end
std::streampos size = in.tellg();     // get current position (bytes)
in.seekg(-4, std::ios::cur);          // back 4 bytes from current

// Write position: seekp / tellp
out.seekp(0, std::ios::end);          // seek write head to end
std::streampos wpos = out.tellp();

// Get file size
std::ifstream fin("file.bin", std::ios::binary);
fin.seekg(0, std::ios::end);
std::streamsize sz = fin.tellg();
fin.seekg(0, std::ios::beg);
```

---

## Error State

```cpp
// State bits:
in.good()    // no errors, not EOF
in.eof()     // EOF reached
in.fail()    // logical error (bad format, conversion failure)
in.bad()     // I/O error (read/write failed)

// Operator bool — true if good() (no fail or bad)
if (!in) { /* error */ }
while (in >> val) { }    // loop ends on fail or eof

// Clear errors (necessary to continue after error)
in.clear();
in.seekg(0);   // seek to retry

// Exceptions — disabled by default; can enable:
in.exceptions(std::ios::failbit | std::ios::badbit);
// now throws std::ios::failure on error
```

---

## `std::stringstream` for Building / Parsing

```cpp
#include <sstream>

// Build string
std::ostringstream oss;
oss << "x=" << 42 << " y=" << 3.14;
std::string s = oss.str();

// Parse string
std::istringstream iss("10 20 30");
int a, b, c;
iss >> a >> b >> c;

// Round-trip: format then parse
std::stringstream ss;
ss << 42;
int n;
ss >> n;

// Clear and reuse
ss.str("");     // clear content
ss.clear();     // clear error flags (BOTH needed!)
ss << "new content";
```

---

## `std::filesystem` Quick Reference (C++17)

```cpp
#include <filesystem>
namespace fs = std::filesystem;

fs::path p = "/home/user/data.txt";
p.filename()        // "data.txt"
p.stem()            // "data"
p.extension()       // ".txt"
p.parent_path()     // "/home/user"
p /= "subdir";      // append path component

fs::exists(p)
fs::is_regular_file(p)
fs::is_directory(p)
fs::file_size(p)          // bytes
fs::last_write_time(p)    // file_time_type

fs::create_directory(p)
fs::create_directories(p) // like mkdir -p
fs::copy("src.txt", "dst.txt")
fs::rename("old", "new")
fs::remove("file.txt")
fs::remove_all("dir/")    // like rm -rf

// Directory iteration
for (const auto& entry : fs::directory_iterator(".")) {
    std::cout << entry.path() << '\n';
}
```

---

## Pitfalls

```cpp
// 1. Not checking is_open() before reading
std::ifstream in("missing.txt");
std::string line;
std::getline(in, line);   // silently reads nothing; line is empty

// 2. Missing ios::binary for binary files
std::ofstream out("data.bin");   // text mode!
// On Windows: '\n' → "\r\n"; raw bytes mangled

// 3. Forgetting to clear() after EOF before seeking
in.seekg(0);   // seek does nothing while eofbit set
// Fix:
in.clear();
in.seekg(0);

// 4. Mixing >> and getline (leftover '\n' in buffer)
int n;
std::cin >> n;
std::string line;
std::getline(std::cin, line);  // reads empty string ('\n' left by >>)
// Fix:
std::cin >> n;
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
std::getline(std::cin, line);

// 5. seekg on write-only stream (ofstream) — use seekp
std::ofstream out2("f.txt");
out2.seekg(0);   // error: no read head

// 6. Struct padding in binary I/O
struct Data { char c; int n; };   // likely has 3 bytes padding
// Write full struct — padding bytes are junk but harmless if reader matches
// Safer: write each field separately for portability

// 7. Endianness — binary files written on x86 (LE (Little Endian)) are not portable to BE (Big Endian) systems
// Use explicit byte-swapping or serialisation format (e.g. network byte order) for portability

// 8. ofstream truncates by default — use ios::app to append
std::ofstream log("log.txt");       // TRUNCATES existing log!
std::ofstream log2("log.txt", std::ios::app);  // appends
```

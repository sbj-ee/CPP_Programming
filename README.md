# C++ Programming

*Topics: cpp, cplusplus, learning, education, systems-programming, tutorial, exercises*

A structured C++ programming learning project covering fundamentals through modern systems programming. Designed for someone who already knows C and wants to learn idiomatic C++.

## Layout

```
CPP_Programming/
├── Introduction.md  # Dev environment, build tools, exercise progression
├── Foreword.md      # Homage to Stroustrup, the C++ committee, and the language's roots
├── exercises/       # Progressive programs, each building on the last
│   ├── 01_hello_world/ … 33_filesystem/
└── topics/              # Markdown reference sheets by concept
    ├── types/               auto, constexpr, numeric_limits, casts
    ├── pointers_and_references/  T&, T&&, const T*, nullptr, function pointers
    ├── memory/              new/delete, RAII, unique_ptr, shared_ptr, weak_ptr
    ├── strings/             std::string, string_view, stringstream, conversions
    ├── classes/             constructors, virtual, override, abstract, slicing
    ├── file_io/             fstream, binary mode, seekg/seekp, stringstream
    ├── templates/           function/class templates, variadic, fold, CTAD (Class Template Argument Deduction), concepts
    ├── error_handling/      exceptions, noexcept, optional, expected, error_code
    ├── concurrency/         thread, mutex, lock_guard, condition_variable, atomic
    ├── processes/           fork/exec/waitpid, pipes, RAII wrappers (POSIX)
    ├── sockets/             TCP/UDP/AF_UNIX, RAII Socket class, getaddrinfo
    ├── mmap/                MmapRegion RAII, MAP_SHARED/PRIVATE, msync
    ├── io_multiplexing/     select/poll/epoll, EpollFd RAII, O_NONBLOCK
    ├── atomics/             std::atomic, memory_order, CAS, lock-free stack
    ├── semaphores/          POSIX sem_t + C++20 counting_semaphore
    ├── dynamic_loading/     dlopen/dlsym, DynLib RAII, plugin pattern
    ├── regex/               std::regex, smatch, regex_replace, sregex_iterator
    └── filesystem/          std::filesystem::path, directory_iterator, error_code
```

## Build

```bash
# Build everything
make

# Build a single exercise
g++ -Wall -Wextra -std=c++17 -g exercises/01_hello_world/hello.cpp -o hello

# Remove all compiled binaries
make clean
```

## Requirements

- `g++` (GCC 10+) or `clang++` (11+) — exercise 30 is built with `-std=c++20`,
  which GCC 9 does not accept
- `make`
- C++17 support required; C++20 used for exercise 30 (semaphores)
- `valgrind` (optional, Linux) for `make valgrind`

### Platform notes

Exercises 01–22 and 32–33 use only standard C++ (33 reads a couple of
Linux-style paths such as `/etc/hostname` but handles their absence).
Exercises 23–31 use POSIX APIs and are developed and tested on Linux:

- 28 (`epoll`) and 30 (`sem_timedwait`, unnamed `sem_t`) are Linux-specific
  and do not build on macOS as written.
- 27 (`mmap`) defines `_DARWIN_C_SOURCE` on macOS so that `MAP_ANONYMOUS`
  stays visible; the other POSIX exercises should work on macOS but are not
  routinely tested there.
- On Windows, use WSL for exercises 23–31.

## Exercises

| # | Topic | Key Concepts |
|---|-------|--------------|
| 01 | Hello World | `std::cout`, `std::cerr`, `constexpr`, headers |
| 02 | Variables & Types | `auto`, `const`/`constexpr`, references, `static_cast`, `numeric_limits` |
| 03 | Control Flow | `if/else`, `switch` with `enum class`, range-based `for`, structured bindings |
| 04 | Functions | overloading, default args, pass by value/ref/const-ref, recursion |
| 05 | Strings & Vectors | `std::string`, `string_view`, `vector`, `array`, `stoi`/`to_string` |
| 06 | Pointers & References | `nullptr`, `T&` vs `T*`, const variants, function pointers, `reinterpret_cast` |
| 07 | Classes | constructors, destructor, `const` members, `this`, access specifiers |
| 08 | Memory Management | `new`/`delete`, RAII, `unique_ptr`, `shared_ptr`, `weak_ptr` |
| 09 | File I/O | `ifstream`/`ofstream`, text/binary modes, `seekg`/`seekp`, `stringstream` |
| 10 | Linked Lists | class template `LinkedList<T>`, destructor, `std::list` comparison |
| 11 | Operator Overloading | `operator+`, `+=`, `==`, `<<`, `[]`, pre/post `++` |
| 12 | Inheritance | `virtual`, `override`, `final`, abstract base, `dynamic_cast`, slicing |
| 13 | Templates | function/class templates, non-type params, specialisation, variadic, fold, CTAD (Class Template Argument Deduction) |
| 14 | STL Containers | `vector`, `deque`, `list`, `map`, `unordered_map`, `set`, adapters |
| 15 | STL Algorithms | `sort`, `find_if`, `transform`, `accumulate`, `copy_if`, `lower_bound` |
| 16 | Enums & Variants | `enum class`, `std::optional`, `std::variant`, `std::visit` |
| 17 | Exceptions | `throw`/`try`/`catch`, custom exceptions, `noexcept`, exception safety |
| 18 | Lambdas | capture modes, `mutable`, generic lambdas, `std::function`, IILE (Immediately-Invoked Lambda Expression) |
| 19 | Smart Pointers | `unique_ptr`/`shared_ptr`/`weak_ptr` deep dive, `enable_shared_from_this` |
| 20 | Move Semantics | lvalue/rvalue refs, Rule of Five, `std::move`, `std::forward`, NRVO (Named Return Value Optimization) |
| 21 | Variadic Templates | parameter packs, fold expressions, `std::tuple`, `std::index_sequence` |
| 22 | Namespaces | nesting, anonymous, `using`, aliases, ADL (Argument-Dependent Lookup), inline namespaces, `extern "C"` |
| 23 | Signal Handling | `sig_atomic_t`, `signal()`, `sigaction()`, `sigprocmask`, `SIGALRM` |
| 24 | std::thread | `thread`, `mutex`, `lock_guard`, `condition_variable`, bounded queue |
| 25 | Process Control | `fork`, `waitpid`, `execvp`, pipes, RAII pipe class, `popen` |
| 26 | Sockets | TCP/UDP/AF_UNIX, RAII `Socket` class, `send_all`/`recv_all`, `getaddrinfo` |
| 27 | Memory-Mapped Files | `MmapRegion` RAII, file-backed/anonymous, `MAP_SHARED`, `msync` |
| 28 | I/O Multiplexing | `select`/`poll`/`epoll`, RAII wrappers, `O_NONBLOCK`, multi-client server |
| 29 | std::atomic | `atomic<T>`, `fetch_add`, CAS (Compare-And-Swap), `memory_order`, `atomic_flag`, lock-free stack |
| 30 | Semaphores | POSIX `sem_t` + C++20 `counting_semaphore`/`binary_semaphore` |
| 31 | Dynamic Loading | `dlopen`/`dlsym`, RAII `DynLib`, plugin pattern, `RTLD_DEFAULT` |
| 32 | std::regex | `regex_search/match/replace`, `smatch`, `sregex_iterator`, ECMAScript (default) and POSIX ERE (Extended Regular Expression) grammars |
| 33 | std::filesystem | `path`, `directory_iterator`, `create_directories`, `remove_all`, `error_code` |

---

## Appendix A: Makefile

The root `Makefile` builds any standalone `.cpp` file found under `exercises/` and
`topics/`, and delegates to per-directory Makefiles (currently every exercise has
one). A simplified sketch:

```makefile
CXX      = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g

SRCS := $(shell find exercises topics -name '*.cpp')
BINS := $(SRCS:.cpp=)

VALGRIND = valgrind --leak-check=full --error-exitcode=1

.PHONY: all clean valgrind

all: $(BINS)

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

valgrind: all
	@for bin in $(BINS); do \
		echo "--- $$bin ---"; \
		$(VALGRIND) $$bin || exit 1; \
	done

clean:
	@find exercises topics -type f ! -name '*.cpp' ! -name '*.hpp' ! -name '*.h' ! -name '*.md' ! -name 'Makefile' -delete
```

> **Note:** This is a simplified illustration. The actual root `Makefile` also manages per-exercise sub-Makefiles via a `_MANAGED` variable and delegates `all`/`clean`/`valgrind` to them, stopping at the first failing directory.

### Flags explained

| Flag | Meaning |
|------|---------|
| `-Wall` | Common warnings |
| `-Wextra` | Extra warnings beyond `-Wall` |
| `-Wpedantic` | Warn about non-ISO C++ extensions (does not make the compiler fully strict) |
| `-std=c++17` | C++17 (structured bindings, `string_view`, `filesystem`, `optional`, `variant`) |
| `-g` | Debug symbols for `gdb` / valgrind |
| `-pthread` | Thread support — per-exercise flag (exercises 24, 29, 30), not in root `CXXFLAGS` |
| `-ldl` | Dynamic loading — per-exercise flag (exercise 31), not in root `CXXFLAGS` |

### Per-exercise Makefiles

Every exercise carries its own `Makefile` (with `all`, `clean` and `valgrind`
targets); the ones that need extra flags (threads, C++20, dynamic loading, plugins)
set them there. The root Makefile delegates to them via `$(MAKE) -C <dir> all`
(and likewise for `clean` and `valgrind`).

---

## Appendix B: C vs C++ Quick Reference

| C | C++ Equivalent | Notes |
|---|---------------|-------|
| `printf` | `std::cout <<` | Use `\n` not `std::endl` for performance |
| `char[]` | `std::string` | Value type, safe, movable |
| `malloc`/`free` | `new`/`delete` or smart ptr | Prefer `make_unique`/`make_shared` |
| `struct` | `class` (private by default) | `struct` = public default |
| Function pointer | `std::function<R(A)>` or lambda | Lambdas are usually clearer |
| `void *` | Templates or `std::variant` | Type-safe alternatives |
| `#define MAX 100` | `constexpr int MAX = 100;` | Has type, scope, debuggable |
| `union` | `std::variant<Ts...>` | Type-safe tagged union |
| `errno` + `perror` | Exceptions or `std::error_code` | RAII cleans up automatically |
| `qsort` | `std::sort` | Type-safe, inlines comparator |
| `pthread_t` | `std::thread` | RAII join on destruction (`jthread`) |

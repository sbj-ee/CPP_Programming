# Introduction

## Who This Is For

This project is for programmers who already know C and want to learn idiomatic C++. If you
can read a `struct`, understand pointers, and write a `Makefile`, you are ready. You do not
need prior C++ experience.

The exercises do not teach C from scratch — they assume you already understand memory,
the stack, the preprocessor, and POSIX. What they teach is how C++ changes, extends, and
improves on that foundation.

## Philosophy

Each exercise is a self-contained program that compiles, runs, and produces observable
output. There are no stub files to fill in. Read the source, build it, run it, then
change something. Understanding comes from reading and breaking working code, not from
filling in blanks.

Comments in the exercises explain the *why*, not the *what*. The code itself explains
the what.

## Development Environment

```bash
# Compiler
g++ --version   # GCC 9+ required; GCC 12+ recommended for C++20 features
clang++ --version   # Clang 11+ also works

# Build tools
make --version

# Debugger and memory checker
gdb --version
valgrind --version   # Linux only
```

## Building

```bash
# Build everything
make

# Build one exercise
cd exercises/07_classes
make
./classes

# Clean all compiled binaries
make clean

# Valgrind all exercises
make valgrind
```

Every exercise directory has its own `Makefile`. Exercises that require extra flags
(`-pthread`, `-ldl`, `-std=c++20`) carry those flags in their local Makefile.

The root `Makefile` discovers and delegates to all of them automatically.

## Exercise Progression

The 33 exercises form five progressive tiers.

### Tier 1 — C++ Fundamentals (01–09)

Direct analogs to C concepts, with C++ idioms introduced at each step.

| # | Topic | What Changes from C |
|---|-------|---------------------|
| 01 | Hello World | `std::cout` instead of `printf`; `#include <iostream>` |
| 02 | Variables & Types | `auto`, references, `constexpr`, `static_cast` |
| 03 | Control Flow | `enum class` in `switch`; range-based `for`; structured bindings |
| 04 | Functions | Overloading; default arguments; pass by const-reference |
| 05 | Strings & Vectors | `std::string`, `string_view`, `vector`, `array` |
| 06 | Pointers & References | References as safer pointers; `nullptr`; const variants |
| 07 | Classes | `class` with constructors, destructor, `const` member functions |
| 08 | Memory Management | RAII; `unique_ptr`, `shared_ptr`, `weak_ptr` |
| 09 | File I/O | `fstream` replaces `FILE*`; RAII auto-closes |

### Tier 2 — Object-Oriented C++ (10–18)

The features that make C++ distinct from C.

| # | Topic | Core Idea |
|---|-------|-----------|
| 10 | Linked Lists | Class template; destructor; comparison with `std::list` |
| 11 | Operator Overloading | Natural syntax for user-defined types |
| 12 | Inheritance | `virtual`, `override`, abstract base, polymorphism |
| 13 | Templates | Compile-time generics; variadic; fold expressions |
| 14 | STL Containers | `vector`, `map`, `unordered_map`, `set`, adapters |
| 15 | STL Algorithms | `sort`, `transform`, `find_if`, `accumulate`, iterators |
| 16 | Enums & Variants | `enum class`; `std::optional`; `std::variant` + `visit` |
| 17 | Exceptions | `throw`/`catch`; `noexcept`; exception safety levels |
| 18 | Lambdas | Capture modes; `std::function`; lambdas with STL |

### Tier 3 — Modern C++ (19–22)

C++11 and later features that change how you write every line.

| # | Topic | Core Idea |
|---|-------|-----------|
| 19 | Smart Pointers | Ownership semantics; `enable_shared_from_this`; cycles |
| 20 | Move Semantics | lvalue/rvalue; Rule of Five; `std::move`; `std::forward` |
| 21 | Variadic Templates | Parameter packs; fold expressions; `std::tuple` |
| 22 | Namespaces | Nesting; ADL; inline namespaces; `extern "C"` |

### Tier 4 — Systems Programming with C++ (23–27)

The same POSIX APIs as C, wrapped in C++ idioms (RAII, `std::string`, typed wrappers).

| # | Topic | C++ Addition |
|---|-------|--------------|
| 23 | Signal Handling | `volatile std::sig_atomic_t`; why `std::cout` is unsafe in handlers |
| 24 | std::thread | `mutex`, `lock_guard`, `condition_variable`; bounded queue |
| 25 | Process Control | RAII pipe class; `std::string` for paths and arguments |
| 26 | Sockets | RAII `Socket` class; `send_all`/`recv_all` as free functions |
| 27 | Memory-Mapped Files | RAII `MmapRegion` class; `munmap` in destructor |

### Tier 5 — Advanced C++ (28–33)

High-performance I/O, concurrency primitives, and C++17/20 library features.

| # | Topic | Core Idea |
|---|-------|-----------|
| 28 | I/O Multiplexing | RAII `EpollFd`, `PipePair`; multi-client echo server |
| 29 | std::atomic | `atomic<T>`, CAS, `memory_order`, lock-free Treiber stack |
| 30 | Semaphores | C++20 `counting_semaphore` + POSIX `sem_t` fallback |
| 31 | Dynamic Loading | RAII `DynLib` wrapping `dlopen`/`dlclose`; typed `dlsym` |
| 32 | std::regex | `smatch`, `regex_replace`, `sregex_iterator` scan loop |
| 33 | std::filesystem | `path`, `directory_iterator`, `create_directories`, `error_code` |

## Key Differences from C_Programming

The C project (in `../C_Programming`) covers the same systems topics. Here is what
changes in the C++ version:

| Concern | C | C++ |
|---------|---|-----|
| Output | `printf` | `std::cout` |
| Strings | `char[]` + `strlen` | `std::string` + `string_view` |
| Dynamic memory | `malloc`/`free` | RAII + smart pointers |
| Error handling | `errno` + return codes | Exceptions or `std::optional` |
| Generics | `void *` + macros | Templates |
| Polymorphism | Function pointer tables | `virtual` + `override` |
| Sync primitives | `pthread_mutex_t` | `std::mutex` + `std::lock_guard` |
| Resource cleanup | `goto cleanup` | RAII destructors |
| I/O | `FILE *` | `std::fstream` |

## Every File Has

- A single `.cpp` source file with six named sections
- A local `Makefile` that builds it with the correct flags
- Zero compiler warnings under `-Wall -Wextra -Wpedantic`
- No `using namespace std;` at file scope

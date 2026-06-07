# Pointers & References — Cheat Sheet

## Pointer Basics

```cpp
int x = 42;
int* p = &x;       // pointer to int; & = address-of operator
*p = 100;          // dereference; x is now 100
int** pp = &p;     // pointer to pointer

// Declaration styles (all equivalent):
int* p1;   int *p2;   int * p3;   // * binds to variable, not type
int *a, b;  // a is int*, b is int — be explicit
int* a2, *b2; // both int*
```

## `nullptr` vs `NULL` vs `0`

| | Type | C++11+ |
|-|------|--------|
| `nullptr` | `std::nullptr_t` | preferred |
| `NULL` | macro, usually `0` or `(void*)0` | C legacy |
| `0` | `int` | avoid as pointer |

```cpp
int* p = nullptr;        // safe null pointer
if (p == nullptr) { }   // explicit check
if (!p) { }             // also idiomatic

void f(int);
void f(int*);
f(NULL);     // calls f(int) — SURPRISE! NULL is 0
f(nullptr);  // calls f(int*) — correct
```

---

## References

### Lvalue Reference `T&`

```cpp
int x = 10;
int& r = x;    // r is an alias for x
r = 20;        // x == 20
int& bad;      // ERROR: references must be initialised

// As function parameter (in/out):
void increment(int& n) { ++n; }

// As const reference (read-only, extends temp lifetime):
void print(const std::string& s);   // no copy, no mutation
const int& cr = 42;   // binds to temporary; lifetime extended
```

### Rvalue Reference `T&&`

```cpp
int&& rv = 42;     // binds to temporary (rvalue)
rv = 50;           // OK — rv is itself an lvalue expression

// Move semantics:
void sink(std::string&& s) {
    store_ = std::move(s);   // pilfer s's buffer
}
sink(std::string("hello"));  // passes rvalue
std::string t = "world";
sink(std::move(t));          // cast t to rvalue; t is now valid-but-unspecified
```

### Reference vs Pointer

| Feature | Reference `T&` | Pointer `T*` |
|---------|---------------|--------------|
| Must be initialised | Yes | No |
| Can be null | No | Yes (`nullptr`) |
| Reseatable | No | Yes |
| Pointer arithmetic | No | Yes |
| `sizeof` | sizeof(T) | sizeof(void*) |
| Syntax for access | `r.member` | `p->member` or `(*p).member` |

---

## `const` Placement Rules

```cpp
const int* p1      // pointer to const int   (can't modify *p1)
int const* p2      // same as above
int* const p3      // const pointer to int   (can't modify p3 itself)
const int* const p4// const pointer to const int

// Mnemonic: read right-to-left
// "p3 is a const pointer to int"
// "p1 is a pointer to const int"
```

```cpp
void f(const int* p);  // read-only access through pointer
void g(int* const p);  // pointer itself is const — unusual at call site
```

---

## Pointer Arithmetic

```cpp
int arr[] = {10, 20, 30, 40};
int* p = arr;       // points to arr[0]
p++;                // points to arr[1]
p += 2;             // points to arr[3]
ptrdiff_t diff = p - arr;  // 3 — difference in elements, not bytes

// Iterating:
for (int* q = arr; q != arr + 4; ++q) { /* *q */ }

// One-past-the-end is valid to form but not dereference:
int* end = arr + 4;  // valid; *end is UB (Undefined Behaviour)
```

---

## Pointer to Function

```cpp
// Declaration: return_type (*name)(param_types)
int (*cmp)(int, int);         // pointer to function taking 2 ints, returning int
cmp = [](int a, int b) { return a - b; };  // assign (no captures allowed)

// Simpler with typedef / using:
using Comparator = int(*)(int, int);
// Comparator c = strcmp;  // won't compile — strcmp is int(const char*, const char*)
// Use a matching signature instead:
int cmp_ints(int a, int b) { return a - b; }
Comparator c = cmp_ints;

// Call:
int result = cmp(3, 5);  // or (*cmp)(3, 5)
```

### Pointer to Member Function

```cpp
struct Foo {
    int bar(int x) { return x * 2; }
};

int (Foo::*pmf)(int) = &Foo::bar;   // pointer to member function
Foo obj;
(obj.*pmf)(10);        // call via object
Foo* ptr = &obj;
(ptr->*pmf)(10);       // call via pointer

// using alias:
using FooMF = int(Foo::*)(int);
FooMF mf = &Foo::bar;
```

### Pointer to Data Member

```cpp
struct Point { int x; int y; };
int Point::*px = &Point::x;
Point p = {3, 4};
p.*px = 10;   // p.x == 10
```

---

## `void*` and `reinterpret_cast`

```cpp
void* vp = &x;          // any pointer → void*; information about type lost
int* ip = static_cast<int*>(vp);  // must cast back explicitly

// reinterpret_cast — raw bit reinterpret (almost always unsafe)
uintptr_t addr = reinterpret_cast<uintptr_t>(ip);
int* back = reinterpret_cast<int*>(addr);

// Type punning — use memcpy, not reinterpret_cast:
double d = 3.14;
uint64_t bits;
std::memcpy(&bits, &d, sizeof(d));  // defined behaviour
```

---

## Smart Pointer Overview

| Smart Pointer | Ownership | Copyable | Header |
|---------------|-----------|----------|--------|
| `std::unique_ptr<T>` | exclusive | No (move only) | `<memory>` |
| `std::shared_ptr<T>` | shared (ref-count) | Yes | `<memory>` |
| `std::weak_ptr<T>` | non-owning observer | Yes | `<memory>` |

```cpp
#include <memory>

// unique_ptr
auto up = std::make_unique<int>(42);
*up = 100;
// delete called automatically when up goes out of scope

// shared_ptr
auto sp1 = std::make_shared<int>(7);
auto sp2 = sp1;          // both own; ref count = 2
sp2.reset();             // ref count = 1; not deleted yet

// weak_ptr
std::weak_ptr<int> wp = sp1;
if (auto locked = wp.lock()) {   // returns shared_ptr or empty
    std::cout << *locked;
}
```

---

## Pitfalls

```cpp
// 1. Dangling reference — returning reference to local
int& bad() {
    int local = 42;
    return local;  // UB: local destroyed after return
}

// 2. Dangling pointer — same as above
int* worse() {
    int local = 42;
    return &local;  // UB
}

// 3. Null dereference
int* p = nullptr;
*p = 5;   // crash (SIGSEGV (Segmentation Fault signal))
// Always check before deref or guarantee non-null

// 4. Object slicing (pointer/reference avoids it)
struct Base { int x; virtual void f(); };
struct Derived : Base { int y; };
Derived d;
Base b = d;     // SLICED: y is lost
Base& r = d;    // OK: polymorphism works through reference/pointer
Base* p2 = &d;  // OK

// 5. Pointer invalidation after realloc
std::vector<int> v = {1,2,3};
int* raw = v.data();
v.push_back(4);   // may reallocate; raw is now dangling!

// 6. Mixing delete and delete[]
int* arr = new int[10];
delete arr;     // UB: should be delete[] arr
```

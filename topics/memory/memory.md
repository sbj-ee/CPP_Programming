# Memory Management & RAII — Cheat Sheet

## Stack vs Heap

| | Stack | Heap |
|-|-------|------|
| Allocation | Automatic (enter scope) | Manual (`new`) or smart ptr |
| Deallocation | Automatic (leave scope) | Manual (`delete`) or destructor |
| Size limit | Small (default ~1–8 MB) | Limited by RAM/swap |
| Speed | Fast (just move SP) | Slower (allocator bookkeeping) |
| Lifetime | Bounded by scope | Until freed |
| Fragmentation | None | Possible |

```cpp
// Stack (automatic storage duration)
void f() {
    int x = 42;          // on stack
    int arr[1000];       // 4000 bytes on stack
}   // x and arr destroyed here

// Heap (dynamic storage duration)
void g() {
    int* p = new int(42);      // heap-allocated
    int* arr = new int[1000];  // heap array
    delete p;
    delete[] arr;
}   // p and arr (pointers) destroyed; objects already freed
```

---

## `new` / `delete`

```cpp
// Single object
int* p = new int;          // default-initialised (indeterminate value)
int* q = new int(99);      // value-initialised to 99
int* r = new int{};        // zero-initialised

// Array
int* arr = new int[10];         // 10 ints, indeterminate
int* arr2 = new int[10]{};      // 10 ints, zero-initialised
int* arr3 = new int[10]{1,2,3}; // first 3 set, rest zero

// Matching delete
delete p;       // single object
delete[] arr;   // array — MUST match new[]

// Placement new — construct in pre-allocated buffer
alignas(Foo) char buf[sizeof(Foo)];
Foo* fp = new(buf) Foo(args);   // no allocation; constructs in buf
fp->~Foo();                      // must call destructor manually!
```

---

## RAII Pattern

**Resource Acquisition Is Initialisation**: acquire in constructor, release in destructor.

```cpp
class FileHandle {
public:
    explicit FileHandle(const char* path)
        : f_(std::fopen(path, "r")) {
        if (!f_) throw std::runtime_error("cannot open file");
    }
    ~FileHandle() { std::fclose(f_); }   // always runs, even on exception

    // Delete copy (resource is unique)
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // Allow move
    FileHandle(FileHandle&& o) noexcept : f_(o.f_) { o.f_ = nullptr; }
    FileHandle& operator=(FileHandle&& o) noexcept {
        if (this != &o) { std::fclose(f_); f_ = o.f_; o.f_ = nullptr; }
        return *this;
    }

    FILE* get() { return f_; }
private:
    FILE* f_;
};
```

---

## `std::unique_ptr<T>`

```cpp
#include <memory>

// Creation — always use make_unique
auto up = std::make_unique<int>(42);
auto up2 = std::make_unique<MyClass>(arg1, arg2);
auto arr = std::make_unique<int[]>(100);   // array form

// Access
*up;            // dereference
up->member;     // member access
up.get();       // raw pointer (non-owning)

// Ownership transfer (move-only)
auto up3 = std::move(up);   // up is now nullptr
f(std::move(up3));          // transfer to function parameter

// Release ownership (you must delete)
int* raw = up.release();    // up = nullptr; you own raw
delete raw;

// Replace resource
up.reset(new int(7));   // deletes old, owns new
up.reset();             // deletes; up = nullptr

// Custom deleter
auto f_del = [](FILE* f){ std::fclose(f); };
std::unique_ptr<FILE, decltype(f_del)> fp(std::fopen("x.txt","r"), f_del);
```

---

## `std::shared_ptr<T>`

```cpp
auto sp1 = std::make_shared<int>(10);   // allocation + control block in one
auto sp2 = sp1;                          // shared ownership; ref count = 2
auto sp3(sp1);                           // same

sp1.use_count();    // 3
sp2.reset();        // ref count = 2; not deleted
sp1.reset();        // ref count = 1
sp3.reset();        // ref count = 0; object deleted

// From raw pointer (avoid double-delete)
int* raw = new int(5);
std::shared_ptr<int> sp(raw);   // OK, but prefer make_shared
// NEVER: shared_ptr<int> bad1(raw); shared_ptr<int> bad2(raw); // two control blocks!

// Custom deleter
auto sp_c = std::shared_ptr<FILE>(std::fopen("x","r"),
                                  [](FILE* f){ std::fclose(f); });

// enable_shared_from_this — get shared_ptr to this inside member
struct Node : std::enable_shared_from_this<Node> {
    std::shared_ptr<Node> self() { return shared_from_this(); }
};
```

---

## `std::weak_ptr<T>`

```cpp
auto sp = std::make_shared<int>(42);
std::weak_ptr<int> wp = sp;   // doesn't increase ref count

wp.expired();         // true if object is gone
wp.use_count();       // number of shared_ptrs (no weak contribution)

// Lock — atomic: if still alive, get shared_ptr; else empty shared_ptr
if (auto locked = wp.lock()) {
    std::cout << *locked;   // safe to use
} else {
    // object was deleted
}
```

### Breaking `shared_ptr` Cycles

```cpp
struct Parent;
struct Child {
    std::shared_ptr<Parent> parent;   // strong ref — creates cycle!
};
struct Parent {
    std::weak_ptr<Child> child;       // weak ref — breaks cycle
};
```

---

## Rule of Five

If you define any of these, define all five:

| Special Member | Signature |
|----------------|-----------|
| Destructor | `~T()` |
| Copy constructor | `T(const T&)` |
| Copy assignment | `T& operator=(const T&)` |
| Move constructor | `T(T&&) noexcept` |
| Move assignment | `T& operator=(T&&) noexcept` |

```cpp
class Buffer {
public:
    Buffer(size_t n) : data_(new char[n]), size_(n) {}

    // Destructor
    ~Buffer() { delete[] data_; }

    // Copy constructor
    Buffer(const Buffer& o) : data_(new char[o.size_]), size_(o.size_) {
        std::memcpy(data_, o.data_, size_);
    }

    // Copy assignment
    Buffer& operator=(const Buffer& o) {
        if (this != &o) {
            delete[] data_;
            data_ = new char[o.size_];
            size_ = o.size_;
            std::memcpy(data_, o.data_, size_);
        }
        return *this;
    }

    // Move constructor
    Buffer(Buffer&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    // Move assignment
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }

private:
    char* data_;
    size_t size_;
};
```

### Rule of Zero

Prefer composition with RAII types so you need none of the five:

```cpp
class Good {
    std::unique_ptr<char[]> data_;  // handles all five automatically
    size_t size_;
};
```

---

## Placement New

```cpp
// Use case: object pool, custom allocator, shared memory
alignas(MyClass) char pool[sizeof(MyClass) * N];

// Construct at specific address
MyClass* obj = new(pool) MyClass(42);

// Must call destructor manually
obj->~MyClass();

// Do NOT delete obj — pool is not heap memory
```

---

## Common Pitfalls

```cpp
// 1. Double delete
int* p = new int(5);
delete p;
delete p;   // UB: heap corruption

// 2. delete[] mismatch
int* arr = new int[10];
delete arr;    // UB: should be delete[]

// 3. Use after free
delete p;
*p = 5;   // UB: p is dangling

// 4. use-after-move
auto up = std::make_unique<int>(1);
auto up2 = std::move(up);
*up;   // UB: up is nullptr

// 5. shared_ptr cycle — memory leak
struct A { std::shared_ptr<B> b; };
struct B { std::shared_ptr<A> a; };  // cycle: neither A nor B freed
// Fix: one side uses weak_ptr

// 6. Raw pointer from make_shared
auto sp = std::make_shared<Foo>();
Foo* raw = sp.get();
// Don't delete raw! Don't create another shared_ptr from raw!

// 7. shared_ptr to stack object
int x = 5;
// shared_ptr<int> sp(&x);  // NEVER: will try to delete stack variable

// 8. Returning unique_ptr by value is fine (NRVO / move)
std::unique_ptr<int> make() { return std::make_unique<int>(7); }  // OK
```

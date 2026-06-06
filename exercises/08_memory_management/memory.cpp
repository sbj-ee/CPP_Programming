// memory.cpp - Exercise 08: Memory Management
// Demonstrates: new/delete, RAII Buffer, unique_ptr, shared_ptr, weak_ptr,
//               stack vs heap decision guide

#include <iostream>
#include <string>
#include <memory>     // unique_ptr, shared_ptr, weak_ptr, make_unique, make_shared
#include <cstring>    // std::memcpy

// ── Section 1: new / delete and new[] / delete[] ──────────────────────────────

void section_new_delete()
{
    std::cout << "=== Section 1: new / delete and new[] / delete[] ===\n";

    // Single object
    int* p = new int(42);
    std::cout << "new int(42): *p = " << *p << "\n";
    delete p;
    p = nullptr;   // prevent dangling pointer
    std::cout << "After delete+nullptr: p is null? " << std::boolalpha << (p == nullptr) << "\n";

    // Array
    int* arr = new int[5]{10, 20, 30, 40, 50};
    std::cout << "new int[5]: ";
    for (int i = 0; i < 5; ++i) std::cout << arr[i] << " ";
    std::cout << "\n";
    delete[] arr;   // MUST use delete[] for arrays
    arr = nullptr;

    // Value-initialised (zeroed) array
    double* zeros = new double[4]();   // () zero-initialises
    std::cout << "new double[4](): ";
    for (int i = 0; i < 4; ++i) std::cout << zeros[i] << " ";
    std::cout << "\n";
    delete[] zeros;

    // What would go wrong (explained, not executed):
    std::cout << "\nPitfalls (described, not executed):\n";
    std::cout << "  double-delete: delete p; delete p; -> undefined behaviour\n";
    std::cout << "  mismatch:      int* a = new int[5]; delete a; -> UB\n";
    std::cout << "  memory leak:   new int(1) without delete -> process holds heap memory\n";
}

// ── Section 2: RAII Buffer class ──────────────────────────────────────────────

// RAII = Resource Acquisition Is Initialisation.
// The resource (heap memory) is acquired in the constructor and released in
// the destructor.  No matter how the scope exits (return, exception, etc.),
// the destructor runs and the memory is freed.

class Buffer {
public:
    // Constructor acquires the resource
    explicit Buffer(std::size_t size)
        : size_(size), data_(new char[size]())
    {
        std::cout << "  [Buffer] ctor: allocated " << size_ << " bytes\n";
    }

    // Destructor releases the resource
    ~Buffer()
    {
        std::cout << "  [Buffer] dtor: freeing " << size_ << " bytes\n";
        delete[] data_;
    }

    // Copy constructor (deep copy)
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new char[other.size_])
    {
        std::memcpy(data_, other.data_, size_);
        std::cout << "  [Buffer] copy ctor\n";
    }

    // Move constructor (steal resources from rvalue)
    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "  [Buffer] move ctor\n";
    }

    // Copy assignment = delete for simplicity (rule of 5 awareness)
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&)      = delete;

    std::size_t size() const { return size_; }

    void fill(int val)
    {
        std::memset(data_, val, size_);
    }

    char at(std::size_t idx) const
    {
        if (idx >= size_) throw std::out_of_range("Buffer::at out of range");
        return data_[idx];
    }

    void print_hex(std::size_t n) const
    {
        std::size_t count = (n < size_) ? n : size_;
        std::cout << "  Buffer[0.." << count-1 << "]: ";
        for (std::size_t i = 0; i < count; ++i)
            std::cout << std::hex << (static_cast<unsigned>(data_[i]) & 0xFF) << " ";
        std::cout << std::dec << "\n";
    }

private:
    std::size_t size_;
    char*       data_;
};

void section_raii()
{
    std::cout << "\n=== Section 2: RAII Buffer Class ===\n";
    {
        Buffer buf(16);
        buf.fill(0xAB);
        buf.print_hex(8);
        // buf goes out of scope here -> destructor automatically called
    }
    std::cout << "  (Buffer was automatically freed when scope ended)\n";

    // Exception safety: even if an exception throws, destructor still runs
    try {
        Buffer buf2(8);
        buf2.fill(0xFF);
        throw std::runtime_error("simulated error");
    } catch (const std::exception& e) {
        std::cout << "  Caught: " << e.what() << " (Buffer still cleaned up)\n";
    }
}

// ── Section 3: std::unique_ptr ────────────────────────────────────────────────

void section_unique_ptr()
{
    std::cout << "\n=== Section 3: std::unique_ptr ===\n";

    // make_unique<T>(args...): preferred way to create unique_ptr (C++14)
    std::unique_ptr<int> uptr = std::make_unique<int>(99);
    std::cout << "unique_ptr value: " << *uptr << "\n";

    // get(): raw pointer (still owned by unique_ptr)
    int* raw = uptr.get();
    std::cout << "get() raw ptr: " << *raw << "\n";

    // reset(): release current resource, optionally take a new one
    uptr.reset(new int(200));
    std::cout << "After reset(200): " << *uptr << "\n";

    uptr.reset();   // release without replacement; uptr is now null
    std::cout << "After reset(): uptr is null? " << (uptr == nullptr) << "\n";

    // Unique ownership: cannot be copied, only moved
    auto u1 = std::make_unique<std::string>("owned by u1");
    std::cout << "u1: " << *u1 << "\n";
    auto u2 = std::move(u1);   // u1 is now null; u2 owns the string
    std::cout << "u2 after move: " << *u2 << "\n";
    std::cout << "u1 after move: " << (u1 == nullptr ? "null" : *u1) << "\n";

    // unique_ptr for arrays
    auto arr = std::make_unique<int[]>(4);
    for (int i = 0; i < 4; ++i) arr[i] = (i+1) * 10;
    std::cout << "unique_ptr<int[]>: ";
    for (int i = 0; i < 4; ++i) std::cout << arr[i] << " ";
    std::cout << "\n";
    // arr automatically delete[]'d when it goes out of scope
}

// ── Section 4: std::shared_ptr ────────────────────────────────────────────────

void section_shared_ptr()
{
    std::cout << "\n=== Section 4: std::shared_ptr (reference counting) ===\n";

    // make_shared<T>: single allocation for object + control block
    std::shared_ptr<std::string> sp1 = std::make_shared<std::string>("shared data");
    std::cout << "sp1: \"" << *sp1 << "\"  use_count=" << sp1.use_count() << "\n";

    {
        std::shared_ptr<std::string> sp2 = sp1;   // share ownership
        std::cout << "sp2 shares: use_count=" << sp1.use_count() << "\n";

        std::shared_ptr<std::string> sp3 = sp2;
        std::cout << "sp3 shares: use_count=" << sp1.use_count() << "\n";

        // sp3 destroyed at end of block -> count drops to 2
        // sp2 destroyed at end of block -> count drops to 1
    }
    std::cout << "After sp2, sp3 out of scope: use_count=" << sp1.use_count() << "\n";
    // sp1 destroyed at end of section_shared_ptr -> count hits 0 -> memory freed
}

// ── Section 5: std::weak_ptr ──────────────────────────────────────────────────

struct Node {
    std::string name;
    std::shared_ptr<Node> next;   // strong reference to next
    std::weak_ptr<Node>   prev;   // weak reference back (breaks cycle)

    explicit Node(const std::string& n) : name(n)
    {
        std::cout << "  [Node] ctor: " << name << "\n";
    }
    ~Node()
    {
        std::cout << "  [Node] dtor: " << name << "\n";
    }
};

void section_weak_ptr()
{
    std::cout << "\n=== Section 5: std::weak_ptr (break reference cycles) ===\n";

    {
        auto node_a = std::make_shared<Node>("A");
        auto node_b = std::make_shared<Node>("B");

        node_a->next = node_b;       // A -> B  (strong)
        node_b->prev = node_a;       // B -> A  (weak, avoids cycle)

        std::cout << "node_a use_count=" << node_a.use_count() << "\n";
        std::cout << "node_b use_count=" << node_b.use_count() << "\n";

        // Using weak_ptr: must lock() to get a shared_ptr temporarily
        std::shared_ptr<Node> locked = node_b->prev.lock();
        if (locked) {
            std::cout << "Locked weak_ptr -> Node: " << locked->name << "\n";
        }
    } // both nodes destroyed here because no cycle exists
    std::cout << "  Both nodes freed (no cycle).\n";

    std::cout << "\nIf both were shared_ptr, they would reference each other\n";
    std::cout << "and NEVER reach use_count 0 -> memory leak!\n";
}

// ── Section 6: Stack vs. Heap decision guide ─────────────────────────────────

void section_stack_vs_heap()
{
    std::cout << "\n=== Section 6: Stack vs. Heap Decision Guide ===\n";

    // Stack: fast, automatic, limited size (~1–8 MB typical)
    int    stack_int  = 42;
    double stack_arr[10] = {};
    std::cout << "Stack: fast, automatic, limited. stack_int=" << stack_int << "\n";
    std::cout << "Stack array size: " << sizeof(stack_arr) << " bytes\n";

    // Heap: slower allocation, manual (or smart pointer) lifetime, large capacity
    auto heap_buf = std::make_unique<int[]>(1'000'000);   // 4 MB
    heap_buf[0] = 1;
    heap_buf[999'999] = 999;
    std::cout << "Heap: large buffer [0]=" << heap_buf[0]
              << " [999999]=" << heap_buf[999'999] << "\n";

    std::cout << "\nDecision guide:\n";
    std::cout << "  Use stack when:\n";
    std::cout << "    - Size is small and known at compile time\n";
    std::cout << "    - Lifetime is tied to current scope\n";
    std::cout << "    - Performance is critical (no allocation overhead)\n";
    std::cout << "  Use heap (smart ptr) when:\n";
    std::cout << "    - Size is large or determined at runtime\n";
    std::cout << "    - Lifetime must extend beyond the creating scope\n";
    std::cout << "    - Sharing ownership across multiple objects\n";
    std::cout << "  unique_ptr: single owner, zero overhead vs raw ptr\n";
    std::cout << "  shared_ptr: shared ownership, reference-counted (small overhead)\n";
    std::cout << "  weak_ptr  : non-owning observer; breaks ownership cycles\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Always pair new[] with delete[], not delete.\n";
    std::cout << "  - RAII ties resource lifetime to object lifetime.\n";
    std::cout << "  - Prefer make_unique/make_shared over raw new.\n";
    std::cout << "  - unique_ptr has zero overhead; prefer it when ownership is clear.\n";
    std::cout << "  - Cycles of shared_ptr leak; use weak_ptr for back-references.\n";
}

int main()
{
    section_new_delete();
    section_raii();
    section_unique_ptr();
    section_shared_ptr();
    section_weak_ptr();
    section_stack_vs_heap();
    return 0;
}

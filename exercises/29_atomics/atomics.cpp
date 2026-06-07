// =============================================================================
// Exercise 29: C++ Atomics (<atomic>)
// =============================================================================
// Topics: std::atomic<T> types, load/store/fetch_add, race vs atomic counter,
//         compare_exchange_strong/weak + CAS loop (atomic max),
//         std::memory_order table, std::atomic_flag spinlock,
//         lock-free Treiber stack
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g -pthread atomics.cpp -o atomics
// =============================================================================

#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <cassert>
#include <string>
#include <sstream>

// =============================================================================
// SECTION 1: std::atomic<T> types — load, store, fetch_add, fetch_sub
// =============================================================================
//
// std::atomic<T> wraps a value with operations that the hardware performs
// atomically — no partial reads or writes visible to other threads.
//
// Specialisations for all integral types, pointers, and bool are guaranteed
// to be lock-free on most platforms (check with is_lock_free()).
//
// Key operations:
//   load(order)           — atomic read
//   store(val, order)     — atomic write
//   fetch_add(n, order)   — atomically add n, return OLD value
//   fetch_sub(n, order)   — atomically subtract n, return OLD value
//   exchange(val, order)  — swap, return old
//   compare_exchange_strong/weak — conditional swap

static void section1_types() {
    std::cout << "\n=== Section 1: std::atomic<T> basics ===\n";

    std::atomic<int>      ai{0};
    std::atomic<long>     al{0};
    std::atomic<bool>     ab{false};
    std::atomic<uint64_t> au{0};
    std::atomic<double>   ad{0.0};   // C++20 mandatory lock-free not guaranteed

    std::cout << "  atomic<int>  lock_free: "
              << (ai.is_lock_free() ? "yes" : "no") << "\n";
    std::cout << "  atomic<long> lock_free: "
              << (al.is_lock_free() ? "yes" : "no") << "\n";
    std::cout << "  atomic<bool> lock_free: "
              << (ab.is_lock_free() ? "yes" : "no") << "\n";
    std::cout << "  atomic<double> lock_free: "
              << (ad.is_lock_free() ? "yes" : "no") << "\n";

    // load / store
    ai.store(42, std::memory_order_relaxed);
    std::cout << "\n  ai.store(42); ai.load() = " << ai.load() << "\n";

    // fetch_add returns old value
    int old = ai.fetch_add(10, std::memory_order_seq_cst);
    std::cout << "  ai.fetch_add(10): old=" << old << "  new=" << ai.load() << "\n";

    // fetch_sub
    old = ai.fetch_sub(5, std::memory_order_seq_cst);
    std::cout << "  ai.fetch_sub(5):  old=" << old << "  new=" << ai.load() << "\n";

    // exchange
    old = ai.exchange(100);
    std::cout << "  ai.exchange(100): old=" << old << "  new=" << ai.load() << "\n";
}

// =============================================================================
// SECTION 2: Race condition vs std::atomic<long> counter (4 threads × 500K)
// =============================================================================

static long           g_racy   = 0;
static std::atomic<long> g_safe(0);

static void racy_add(long n) {
    for (long i = 0; i < n; ++i) ++g_racy;
}
static void atomic_add(long n) {
    for (long i = 0; i < n; ++i) g_safe.fetch_add(1, std::memory_order_relaxed);
}

static void section2_counter_comparison() {
    std::cout << "\n=== Section 2: Race condition vs std::atomic counter ===\n";

    const int  NT  = 4;
    const long PER = 500'000L;

    // Racy version
    g_racy = 0;
    {
        std::vector<std::thread> tv;
        tv.reserve(NT);
        for (int i = 0; i < NT; ++i) tv.emplace_back(racy_add, PER);
        for (auto& t : tv) t.join();
    }
    std::cout << "  plain long  expected=" << NT * PER
              << "  got=" << g_racy << "\n";

    // Atomic version
    g_safe.store(0);
    {
        std::vector<std::thread> tv;
        tv.reserve(NT);
        for (int i = 0; i < NT; ++i) tv.emplace_back(atomic_add, PER);
        for (auto& t : tv) t.join();
    }
    std::cout << "  atomic<long> expected=" << NT * PER
              << "  got=" << g_safe.load() << "\n";
    assert(g_safe.load() == NT * PER);
    std::cout << "  Assertion passed: atomic guarantees correct count.\n";
}

// =============================================================================
// SECTION 3: compare_exchange_strong / weak — CAS loop (atomic max)
// =============================================================================
//
// compare_exchange_strong(expected, desired):
//   If *this == expected: atomically set *this = desired, return true.
//   Else:                 set expected = *this, return false.
//
// compare_exchange_weak may spuriously fail (useful in tight CAS loops for
// performance; use _strong when the code must not loop without progress).
//
// Classic CAS pattern: "atomic max" — set the atomic to max(current, newval).

static std::atomic<int> g_max{0};

static void update_max(int val) {
    int cur = g_max.load(std::memory_order_relaxed);
    // Retry until we either set it (cur < val) or observe a larger value
    while (cur < val &&
           !g_max.compare_exchange_weak(cur, val,
                                        std::memory_order_release,
                                        std::memory_order_relaxed))
    {
        // cur was updated by compare_exchange_weak to the current value
    }
}

static void section3_cas() {
    std::cout << "\n=== Section 3: compare_exchange — atomic max ===\n";

    g_max.store(0);
    const std::vector<int> values = {3, 7, 2, 15, 9, 1, 15, 4};

    std::vector<std::thread> tv;
    for (int v : values)
        tv.emplace_back(update_max, v);
    for (auto& t : tv) t.join();

    std::cout << "  Values: ";
    for (int v : values) std::cout << v << " ";
    std::cout << "\n";
    std::cout << "  Atomic max result: " << g_max.load() << "\n";
    assert(g_max.load() == 15);

    // compare_exchange_strong demo (no spurious failure)
    std::atomic<int> x{10};
    int expected = 10;
    bool ok = x.compare_exchange_strong(expected, 99);
    std::cout << "\n  x=10; CAS(10->99): success=" << ok
              << "  x=" << x.load() << "\n";

    expected = 10;  // x is now 99 — will fail
    ok = x.compare_exchange_strong(expected, 200);
    std::cout << "  x=99; CAS(10->200): success=" << ok
              << "  expected updated to " << expected
              << "  x=" << x.load() << "\n";
}

// =============================================================================
// SECTION 4: std::memory_order table
// =============================================================================

static void section4_memory_order() {
    std::cout << "\n=== Section 4: std::memory_order ===\n";

    std::cout << "  relaxed    — no ordering guarantee; only atomicity.\n";
    std::cout << "               Use for counters where exact order doesn't matter.\n";
    std::cout << "  acquire    — no load/store after this can be reordered before it.\n";
    std::cout << "               Pairs with a release store on the same atomic.\n";
    std::cout << "  release    — no load/store before this can be reordered after it.\n";
    std::cout << "               Pairs with an acquire load.\n";
    std::cout << "  acq_rel    — acquire + release in one operation (RMW ops).\n";
    std::cout << "  seq_cst    — strongest: total sequential consistency across all\n";
    std::cout << "               threads.  Default for most atomic operations.\n";
    std::cout << "               Slowest; adds full memory barrier.\n";
    std::cout << "  consume    — data-dependency ordering (rarely used; avoid).\n";

    std::cout << "\n  Typical patterns:\n";
    std::cout << "    Counter (no ordering needed):  memory_order_relaxed\n";
    std::cout << "    Publish data then flag:        store flag with memory_order_release\n";
    std::cout << "    Read flag then use data:       load flag with memory_order_acquire\n";
    std::cout << "    CAS inner loop:                compare_exchange_weak + relaxed fail\n";

    // Release-acquire demo: flag used to publish data
    std::atomic<bool>        ready{false};
    std::atomic<int>         payload{0};

    std::thread writer([&]() {
        payload.store(42, std::memory_order_relaxed);        // write data first
        ready.store(true, std::memory_order_release);        // publish
    });

    std::thread reader([&]() {
        while (!ready.load(std::memory_order_acquire)) {}    // spin until published
        // Acquire guarantees payload store is visible here
        std::cout << "  release-acquire: reader sees payload="
                  << payload.load(std::memory_order_relaxed) << "\n";
    });

    writer.join();
    reader.join();
}

// =============================================================================
// SECTION 5: std::atomic_flag spinlock (test_and_set / clear)
// =============================================================================
//
// std::atomic_flag is the only guaranteed lock-free atomic type.
// It supports two operations:
//   test_and_set(order) — atomically set to true, return old value
//   clear(order)        — atomically set to false
//
// Use it to build a simple spinlock (busy-wait mutex).

class Spinlock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock()   { while (flag_.test_and_set(std::memory_order_acquire)) {} }
    void unlock() { flag_.clear(std::memory_order_release); }
};

static Spinlock g_spin;
static long     g_spin_counter = 0;

static void spin_increment(long n) {
    for (long i = 0; i < n; ++i) {
        g_spin.lock();
        ++g_spin_counter;
        g_spin.unlock();
    }
}

static void section5_spinlock() {
    std::cout << "\n=== Section 5: std::atomic_flag spinlock ===\n";

    const int  NT  = 4;
    const long PER = 100'000L;
    g_spin_counter = 0;

    std::vector<std::thread> tv;
    tv.reserve(NT);
    for (int i = 0; i < NT; ++i) tv.emplace_back(spin_increment, PER);
    for (auto& t : tv) t.join();

    std::cout << "  Spinlock counter: expected=" << NT * PER
              << "  got=" << g_spin_counter << "\n";
    assert(g_spin_counter == NT * PER);

    std::cout << "  Note: spinlocks waste CPU; prefer std::mutex for contended\n";
    std::cout << "  critical sections.  Spinlocks shine only when the lock is held\n";
    std::cout << "  for a very short time and contention is rare.\n";
}

// =============================================================================
// SECTION 6: Lock-free Treiber stack
// =============================================================================
//
// A Treiber stack is a lock-free LIFO data structure using a single atomic
// pointer (top of stack).  push/pop both use a CAS loop.
//
// NOTE: This implementation has the ABA problem — not safe for general use
// without hazard pointers or tagged pointers.  Shown here as a teaching example.

template<typename T>
class TreiberStack {
    struct Node {
        T     value;
        Node* next;
        explicit Node(const T& v) : value(v), next(nullptr) {}
    };

    std::atomic<Node*> top_{nullptr};

public:
    void push(const T& val) {
        Node* node = new Node(val);
        Node* old_top = top_.load(std::memory_order_relaxed);
        do {
            node->next = old_top;
        } while (!top_.compare_exchange_weak(old_top, node,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    }

    bool pop(T& out) {
        Node* old_top = top_.load(std::memory_order_acquire);
        while (old_top) {
            if (top_.compare_exchange_weak(old_top, old_top->next,
                                           std::memory_order_release,
                                           std::memory_order_acquire)) {
                out = old_top->value;
                delete old_top;
                return true;
            }
            // old_top updated to current top — retry
        }
        return false;  // empty
    }

    ~TreiberStack() {
        T dummy;
        while (pop(dummy)) {}
    }
};

static void section6_treiber_stack() {
    std::cout << "\n=== Section 6: Lock-free Treiber stack ===\n";

    TreiberStack<int> stack;

    // Push from multiple threads
    const int NT = 4, PER = 25;
    std::vector<std::thread> tv;
    for (int t = 0; t < NT; ++t) {
        tv.emplace_back([&stack, t]() {
            for (int i = 0; i < PER; ++i)
                stack.push(t * 100 + i);
        });
    }
    for (auto& t : tv) t.join();

    // Pop all items
    int count = 0;
    int val = 0;
    while (stack.pop(val)) ++count;
    std::cout << "  Pushed " << NT * PER << " items from " << NT << " threads.\n";
    std::cout << "  Popped " << count << " items.\n";
    assert(count == NT * PER);

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. memory_order_relaxed is safe ONLY when ordering doesn't matter\n";
    std::cout << "     (e.g., an independent counter).  Wrong order = data races.\n";
    std::cout << "  2. compare_exchange_weak may fail spuriously — always in a loop.\n";
    std::cout << "     compare_exchange_strong never fails spuriously but may be\n";
    std::cout << "     slower on platforms with LL/SC (ARM, RISC-V).\n";
    std::cout << "  3. Treiber stack has ABA problem: popping the same node twice\n";
    std::cout << "     if it is recycled.  Use epoch-based reclamation in production.\n";
    std::cout << "  4. atomic<double> is standard but often not hardware lock-free.\n";
    std::cout << "     Check is_lock_free() before using in hot paths.\n";
    std::cout << "  5. Spinlocks consume CPU and cause priority inversion under\n";
    std::cout << "     heavy contention.  Prefer std::mutex in most cases.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 29: C++ Atomics ===\n";

    section1_types();
    section2_counter_comparison();
    section3_cas();
    section4_memory_order();
    section5_spinlock();
    section6_treiber_stack();

    std::cout << "\nDone.\n";
    return 0;
}

# std::atomic — Cheat Sheet

## Declaration & `is_lock_free`

```cpp
#include <atomic>

std::atomic<int>      ai{0};         // atomic int
std::atomic<bool>     ab{false};     // atomic bool
std::atomic<uint64_t> a64{0};        // atomic uint64_t
std::atomic<void*>    ap{nullptr};   // atomic pointer

ai.is_lock_free();    // true if hardware-native (no internal mutex)
// Alternatively:
std::atomic<int>::is_always_lock_free   // constexpr bool (C++17)

// Types guaranteed lock-free on all conforming platforms:
std::atomic<bool>
std::atomic<char>
std::atomic_flag
// Others (int, long, pointer) are lock-free on x86/ARM64 in practice
```

---

## Core Operations

```cpp
std::atomic<int> x{10};

// Load / Store
int val = x.load();                          // read
int val = x.load(std::memory_order_acquire); // with ordering
x.store(42);                                 // write
x.store(42, std::memory_order_release);      // with ordering

// Read-Modify-Write (return OLD value)
int old = x.fetch_add(1);    // x += 1;  returns old x
int old = x.fetch_sub(3);    // x -= 3
int old = x.fetch_or(mask);  // x |= mask
int old = x.fetch_and(mask); // x &= mask
int old = x.fetch_xor(mask); // x ^= mask

// Shorthand operators (return new value for prefix, old for postfix)
++x; x++; --x; x--;    // fetch_add(1) / fetch_sub(1)
x += 5;   x -= 2;      // same as fetch_add/sub but return new value
x |= mask; x &= mask; x ^= mask;

// Atomic exchange — swap and return old value
int old = x.exchange(99);

// Wait (C++20) — block until value != expected
x.wait(0);                    // block while x == 0
x.notify_one();               // wake one waiter
x.notify_all();               // wake all waiters
```

---

## Compare-and-Swap (CAS)

```cpp
// compare_exchange_strong:
// If x == expected: x = desired; return true
// Else: expected = x; return false
int expected = 0;
bool ok = x.compare_exchange_strong(expected, 1);

// compare_exchange_weak — may spuriously fail even if equal
// Faster on some platforms (LL/SC architectures); use in loops
int expected = 0;
while (!x.compare_exchange_weak(expected, expected * 2)) {
    // expected was updated to current x — retry
}

// CAS with explicit memory orders
x.compare_exchange_strong(expected, desired,
    std::memory_order_acq_rel,   // success order
    std::memory_order_acquire);  // failure order
// Failure order must not be stronger than success order
// Failure order cannot be release or acq_rel
```

### CAS Loop Pattern

```cpp
// Atomic multiply (no fetch_mul exists)
std::atomic<int> val{5};
int expected = val.load();
while (!val.compare_exchange_weak(expected, expected * 3)) { }
// After loop: val == 15

// Lock-free push to singly-linked list (Treiber stack — see below)
```

---

## `std::memory_order` Table

| Order | Tag | Use |
|-------|-----|-----|
| Relaxed | `memory_order_relaxed` | No sync; only atomicity. Counters, stats. |
| Consume | `memory_order_consume` | Data-dependency ordering (avoid; use acquire) |
| Acquire | `memory_order_acquire` | Load: no reads/writes reordered before this |
| Release | `memory_order_release` | Store: no reads/writes reordered after this |
| Acq_Rel | `memory_order_acq_rel` | Read-modify-write: both acquire + release |
| Seq_Cst | `memory_order_seq_cst` | Total order (default; strongest; slowest) |

```
Release-Acquire pairing:
  Thread 1 (producer):      Thread 2 (consumer):
  data = compute();         while (!ready.load(acquire)) {}
  ready.store(true,release);use(data);  // safe — sees Thread 1's writes
```

### Typical Orderings by Use Case

| Use Case | Load | Store | RMW |
|----------|------|-------|-----|
| Counter (no inter-thread data) | relaxed | relaxed | relaxed |
| Producer/consumer flag | acquire | release | acq_rel |
| Spinlock acquire | acquire | — | acq_rel |
| Spinlock release | — | release | — |
| Sequentially consistent | seq_cst | seq_cst | seq_cst |

---

## `std::atomic_flag`

Guaranteed lock-free. Only two states: set / clear. No load/store.

```cpp
std::atomic_flag flag = ATOMIC_FLAG_INIT;   // must initialise this way (pre-C++20)
// C++20: std::atomic_flag flag{};           // also valid

flag.test_and_set();    // returns old value; sets flag atomically
flag.clear();           // clears flag

// C++20 additions:
flag.test();            // read current value without modifying
flag.wait(false);       // block while value == false
flag.notify_one();
flag.notify_all();
```

### Spinlock with `atomic_flag`

```cpp
class Spinlock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // CPU hint to reduce power and improve speculation
#if defined(__x86_64__) || defined(__i386__)
            __builtin_ia32_pause();     // x86 PAUSE instruction
#else
            std::this_thread::yield();  // portable fallback
#endif
        }
    }
    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }
};

Spinlock sl;
{
    std::lock_guard<Spinlock> lk(sl);   // works because Spinlock has lock()/unlock()
    critical_section();
}
```

---

## `std::atomic_ref<T>` (C++20)

Atomic operations on a non-atomic object.

```cpp
#include <atomic>

int arr[1024] = {};
std::atomic_ref<int> ref(arr[42]);  // atomic access to arr[42]
ref.fetch_add(1, std::memory_order_relaxed);
int val = ref.load();

// Constraint: the referenced object must be aligned (alignof(T))
// Constraint: T must be trivially copyable
// Useful for: adding atomicity to legacy plain-data structures,
//             or elements of arrays shared between threads
```

---

## Lock-Free Treiber Stack

```cpp
template<typename T>
class TreiberStack {
    struct Node { T val; Node* next; };
    std::atomic<Node*> top_{nullptr};

public:
    void push(T val) {
        Node* node = new Node{std::move(val), nullptr};
        node->next = top_.load(std::memory_order_relaxed);
        while (!top_.compare_exchange_weak(
                    node->next, node,
                    std::memory_order_release,
                    std::memory_order_relaxed)) { }
    }

    bool pop(T& out) {
        Node* old = top_.load(std::memory_order_acquire);
        while (old) {
            if (top_.compare_exchange_weak(
                    old, old->next,
                    std::memory_order_acquire,
                    std::memory_order_relaxed)) {
                out = std::move(old->val);
                delete old;
                return true;
            }
        }
        return false;  // empty
    }

    // WARNING: suffers from ABA problem — see pitfalls
};
```

---

## Atomics vs `std::mutex`

| Factor | `std::atomic` | `std::mutex` |
|--------|--------------|-------------|
| Overhead | Very low (hardware instruction) | Higher (syscall possible) |
| Composability | Hard (CAS loops, ABA) | Easy (RAII, lock multiple) |
| Use case | Simple types: int, bool, pointer | Complex invariants |
| Blocking | No | Yes |
| Lock-free? | Yes (for simple ops) | No |

**Use atomics for:** counters, flags, pointers, lock-free data structures.  
**Use mutex for:** multiple variables with invariants, complex operations.

---

## Pitfalls

```cpp
// 1. ABA problem in CAS-based algorithms
// Thread 1: read top = A; Thread 2: pop A, push B, push A; Thread 1: CAS succeeds
// Top looks like A but the stack structure changed underneath.
// Fix: versioned pointer (tagged pointer) or hazard pointers.

// 2. volatile is NOT sufficient for thread synchronisation
volatile int flag = 0;   // WRONG: no memory barrier
// volatile prevents compiler optimisation but doesn't prevent CPU reordering
std::atomic<int> flag2{0};  // CORRECT

// 3. std::atomic<double> has no fetch_add on many platforms
std::atomic<double> sum{0.0};
// sum.fetch_add(x);  — may not compile; no hardware instruction
// Use CAS loop:
double expected = sum.load();
while (!sum.compare_exchange_weak(expected, expected + x)) { }

// 4. Relaxed ordering doesn't prevent reordering
std::atomic<int> x{0}, y{0};
// Thread 1: x.store(1, relaxed); y.store(1, relaxed);
// Thread 2: if (y.load(relaxed) == 1) assert(x.load(relaxed) == 1);
// The assert may FAIL — no ordering guarantee between the two stores.

// 5. Non-atomic compound operations
std::atomic<int> a{0};
if (a == 0) a = 1;   // NOT atomic — check and set are separate ops
// Use CAS: int expected = 0; a.compare_exchange_strong(expected, 1);

// 6. Memory order failure constraint
x.compare_exchange_strong(exp, desired,
    std::memory_order_acquire,   // success
    std::memory_order_seq_cst);  // INVALID: failure stronger than success

// 7. Infinite spinlock under high contention
// Spinlocks starve threads; prefer mutex for long critical sections
// For short critical sections: prefer OS futex-based mutex (usually what std::mutex is)
```

# std::thread & Synchronisation — Cheat Sheet

## `std::thread`

```cpp
#include <thread>

// Create and run
std::thread t(function, arg1, arg2);   // args copied into thread
std::thread t2([&]{ do_work(); });      // lambda with capture

// Thread ID
t.get_id();
std::this_thread::get_id();

// Join — wait for thread to finish (must join or detach before destruction)
t.join();

// Detach — let thread run independently (handle becomes invalid)
t.detach();

// Check if joinable
if (t.joinable()) t.join();

// Hardware concurrency hint
unsigned cores = std::thread::hardware_concurrency();  // may return 0
```

### Passing Arguments

```cpp
void f(int n, std::string s) { }

// Values copied into thread by default
std::string msg = "hello";
std::thread t(f, 42, msg);    // msg is COPIED

// Pass reference: wrap with std::ref
void modify(int& n) { ++n; }
int x = 0;
std::thread t2(modify, std::ref(x));  // x modified by thread
t2.join();

// Pass by move
std::thread t3(f, 42, std::move(msg));  // msg moved into thread

// Lambda capture — most flexible
std::thread t4([&x]{ ++x; });   // capture by ref: x must outlive t4
std::thread t5([x]{ use(x); }); // capture by value: safe copy
```

---

## RAII (Resource Acquisition Is Initialisation) Thread Wrapper

```cpp
class ScopedThread {
    std::thread t_;
public:
    explicit ScopedThread(std::thread t) : t_(std::move(t)) {
        if (!t_.joinable()) throw std::logic_error("No thread");
    }
    ~ScopedThread() { t_.join(); }
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};
// Or use std::jthread (C++20) which joins automatically
```

---

## `std::mutex`

```cpp
#include <mutex>

std::mutex mtx;

// Manual lock/unlock — avoid, prefer RAII wrappers
mtx.lock();
critical_section();
mtx.unlock();

mtx.try_lock();   // returns false immediately if locked
```

### `std::lock_guard` — Simple RAII Lock

```cpp
{
    std::lock_guard<std::mutex> lock(mtx);  // locks on construction
    // ...
}   // unlocks automatically (even on exception)

// C++17: CTAD (Class Template Argument Deduction)
std::lock_guard lock(mtx);
```

### `std::unique_lock` — Flexible RAII Lock

```cpp
std::unique_lock<std::mutex> ul(mtx);        // lock immediately
std::unique_lock<std::mutex> ul2(mtx, std::defer_lock);   // don't lock yet
ul2.lock();
ul2.unlock();         // can unlock early
ul2.lock();           // re-lock
ul2.owns_lock();      // check if locked
ul2.release();        // release ownership (must unlock manually!)

// Movable — can transfer lock to another function
std::unique_lock<std::mutex> take_lock() {
    return std::unique_lock<std::mutex>(mtx);  // moved out
}
```

### Locking Multiple Mutexes Atomically

```cpp
std::mutex m1, m2;
// Deadlock-safe: acquires both or none
std::lock(m1, m2);
std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);

// C++17 scoped_lock — simpler
std::scoped_lock lock(m1, m2);   // locks both atomically; RAII
```

---

## `std::condition_variable`

```cpp
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

// Producer
{
    std::lock_guard lock(mtx);
    ready = true;
}
cv.notify_one();    // or notify_all()

// Consumer
{
    std::unique_lock lock(mtx);
    cv.wait(lock, []{ return ready; });  // re-checks predicate on spurious wakeup
    // lock re-acquired after wakeup
    consume();
}

// Wait with timeout
cv.wait_for(lock, std::chrono::seconds(5), []{ return ready; });
cv.wait_until(lock, deadline, pred);
```

### Bounded Queue (Producer/Consumer Pattern)

```cpp
template<typename T, size_t Capacity>
class BoundedQueue {
    std::queue<T> q_;
    std::mutex mtx_;
    std::condition_variable not_empty_, not_full_;

public:
    void push(T val) {
        std::unique_lock lock(mtx_);
        not_full_.wait(lock, [&]{ return q_.size() < Capacity; });
        q_.push(std::move(val));
        not_empty_.notify_one();
    }

    T pop() {
        std::unique_lock lock(mtx_);
        not_empty_.wait(lock, [&]{ return !q_.empty(); });
        T val = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return val;
    }
};
```

---

## `std::atomic<T>`

```cpp
#include <atomic>

std::atomic<int> counter{0};

counter.load();                     // read
counter.store(5);                   // write
counter.fetch_add(1);               // returns old value; atomic ++
counter.fetch_sub(1);               // atomic --
counter.fetch_or(mask);             // atomic |=
counter.fetch_and(mask);            // atomic &=
counter.fetch_xor(mask);            // atomic ^=
++counter; counter++;               // shorthand fetch_add(1)
counter.exchange(newval);           // swap; returns old

// Compare-and-swap
int expected = 0;
bool ok = counter.compare_exchange_strong(expected, 1);
// If counter == expected: sets counter=1, returns true
// Else: sets expected=counter's actual value, returns false

// Weak version: may spuriously fail (but faster on some platforms)
while (!counter.compare_exchange_weak(expected, expected + 1)) { }

counter.is_lock_free();  // true if hardware-native (no mutex inside)
```

---

## `std::memory_order`

| Order | Meaning |
|-------|---------|
| `memory_order_relaxed` | No synchronisation; only atomicity |
| `memory_order_consume` | Dependency-ordering load (rarely used) |
| `memory_order_acquire` | Load; no reads/writes moved before this load |
| `memory_order_release` | Store; no reads/writes moved after this store |
| `memory_order_acq_rel` | Both acquire and release (for RMW (Read-Modify-Write) ops) |
| `memory_order_seq_cst` | Total sequential ordering (default) |

```cpp
// Release-acquire pattern (producer/consumer flag)
std::atomic<bool> ready{false};
int data = 0;

// Thread 1 (producer)
data = 42;
ready.store(true, std::memory_order_release);  // writes before this are visible

// Thread 2 (consumer)
while (!ready.load(std::memory_order_acquire)) { }  // sees writes from before release
use(data);   // safe: data == 42

// Relaxed for simple counter (no ordering needed)
std::atomic<size_t> event_count{0};
event_count.fetch_add(1, std::memory_order_relaxed);
```

---

## `std::atomic_flag` — Spinlock

```cpp
std::atomic_flag flag = ATOMIC_FLAG_INIT;

// Spinlock using atomic_flag
class Spinlock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // spin — on x86 add _mm_pause(); on POSIX yield:
            std::this_thread::yield();
        }
    }
    void unlock() { flag_.clear(std::memory_order_release); }
};

// C++20 additions to atomic_flag
flag.test();   // read without setting
flag.wait(true);    // block until value != true
flag.notify_one();
```

---

## `std::jthread` (C++20)

```cpp
#include <thread>

// Joins automatically on destruction — no need to join/detach
std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        do_work();
    }
});

// Request stop from outside
t.request_stop();   // sets stop token; thread checks and exits
// t's destructor calls request_stop() + join()
```

---

## `thread_local` Storage

```cpp
thread_local int counter = 0;    // each thread has its own counter
thread_local std::string name;   // initialised per thread, destroyed on thread exit

// Use case: thread-local random engine
thread_local std::mt19937 rng(std::random_device{}());
```

---

## Pitfalls

```cpp
// 1. Deadlock — acquiring locks in different orders
// Thread A: lock(m1) then lock(m2)
// Thread B: lock(m2) then lock(m1)  → deadlock
// Fix: always acquire in same order, or use std::scoped_lock(m1, m2)

// 2. Spurious wakeup — always use predicate with wait()
cv.wait(lock);        // BAD: may wake without notify
cv.wait(lock, pred);  // GOOD: re-checks predicate

// 3. Dangling reference captured in lambda
int local = 5;
std::thread t([&local]{ use(local); });
// If main exits before t, local is gone
// Fix: capture by value or ensure lifetime
t.detach(); // DANGER: local now dangling

// 4. Data race — accessing shared data without synchronisation
int shared = 0;
std::thread t1([&]{ ++shared; });
std::thread t2([&]{ ++shared; });  // RACE: UB
// Fix: std::atomic<int> shared or protect with mutex

// 5. Forgetting join() or detach() — terminates program
{
    std::thread t(f);
}   // t destroyed without join/detach → std::terminate()

// 6. pre-C++20: std::atomic<double> has no fetch_add; C++20 adds it
// Use compare_exchange loop instead:
std::atomic<double> sum{0.0};
double expected = sum.load();
while (!sum.compare_exchange_weak(expected, expected + val)) { }

// 7. volatile is NOT sufficient for thread synchronisation
volatile int flag = 0;  // WRONG: no memory barrier, no atomicity guarantee
std::atomic<int> flag2{0};  // CORRECT
```

# Semaphores — Cheat Sheet

## POSIX `sem_t` API

```cpp
#include <semaphore.h>

sem_t sem;

// Unnamed semaphore
sem_init(&sem,
    0,      // pshared: 0 = between threads; nonzero = between processes
    1);     // initial value
sem_destroy(&sem);  // call when done

// Named semaphore (persists across processes)
sem_t* sp = sem_open("/mysem",
    O_CREAT | O_EXCL,  // flags
    0644,              // permissions
    1);                // initial value
sem_close(sp);         // close handle (semaphore persists)
sem_unlink("/mysem");  // destroy it

// Operations
sem_wait(&sem);         // P() — decrement; blocks if 0
sem_trywait(&sem);      // non-blocking; returns -1 (EAGAIN) if 0
sem_timedwait(&sem, &abs_timespec);  // timed wait (absolute time)
sem_post(&sem);         // V() — increment; wakes one waiter
sem_getvalue(&sem, &val); // read current value
```

---

## C++20 `<semaphore>`

```cpp
#include <semaphore>

// counting_semaphore<MaxVal> — template arg is compile-time max
std::counting_semaphore<10> pool_sem{10};   // 10 slots
std::counting_semaphore<>   csem{5};        // default max = implementation max

// binary_semaphore = counting_semaphore<1>
std::binary_semaphore bsem{0};   // initial value 0 (locked)

// Operations
sem.acquire();                     // block until count > 0; decrement
sem.release();                     // increment; wake waiter
sem.release(n);                    // increment by n (C++20)
sem.try_acquire();                 // non-blocking; returns bool
sem.try_acquire_for(duration);     // timed; returns bool
sem.try_acquire_until(time_point); // timed; returns bool
```

---

## C++20 vs POSIX Comparison

| Feature | POSIX `sem_t` | C++20 `counting_semaphore` |
|---------|--------------|---------------------------|
| Named semaphore | Yes | No (POSIX only) |
| Cross-process | Yes (pshared) | No (same process only) |
| Timed wait | `sem_timedwait` (absolute) | `try_acquire_for` (relative) |
| Max value | System-defined | Template param |
| Compiler-agnostic | Yes (POSIX) | C++20 compilers |
| Header | `<semaphore.h>` | `<semaphore>` |
| Compile-time max | No | Yes (`LeastMaxValue`) |
| Check max | `sem_getvalue` | (no equivalent) |

---

## Binary Semaphore: Signal Pattern

```cpp
// Thread synchronisation without a shared variable
std::binary_semaphore ready{0};   // starts locked

// Thread A (waiter)
void waiter() {
    ready.acquire();   // block until B posts
    use_data();
}

// Thread B (signaller)
void signaller() {
    prepare_data();
    ready.release();   // unblock A
}
```

POSIX equivalent:

```cpp
sem_t ready;
sem_init(&ready, 0, 0);

void waiter()    { sem_wait(&ready); use_data(); }
void signaller() { prepare_data(); sem_post(&ready); }
```

---

## Counting Semaphore: Resource Pool

```cpp
// Limit concurrent access to N resources
constexpr int N = 3;
std::counting_semaphore<N> pool{N};

void use_resource() {
    pool.acquire();          // claim one slot; blocks if all N in use
    // ... use resource ...
    pool.release();          // return slot
}

// With std::lock_guard-like RAII
class SemaphoreGuard {
    std::counting_semaphore<>& sem_;
public:
    explicit SemaphoreGuard(std::counting_semaphore<>& s) : sem_(s) { sem_.acquire(); }
    ~SemaphoreGuard() { sem_.release(); }
    SemaphoreGuard(const SemaphoreGuard&) = delete;
};
```

POSIX equivalent:

```cpp
sem_t pool_sem;
sem_init(&pool_sem, 0, N);

void use_resource() {
    sem_wait(&pool_sem);
    // ... use resource ...
    sem_post(&pool_sem);
}
```

---

## Producer/Consumer with Semaphores

```cpp
constexpr int BUF_SIZE = 8;
std::counting_semaphore<BUF_SIZE> empty_slots{BUF_SIZE};
std::counting_semaphore<BUF_SIZE> full_slots{0};
std::mutex buf_mtx;
std::queue<int> buffer;

void producer(int item) {
    empty_slots.acquire();         // wait for space
    { std::lock_guard lk(buf_mtx); buffer.push(item); }
    full_slots.release();          // signal item available
}

int consumer() {
    full_slots.acquire();          // wait for item
    int item;
    { std::lock_guard lk(buf_mtx); item = buffer.front(); buffer.pop(); }
    empty_slots.release();         // signal space available
    return item;
}
```

---

## Named Semaphores (POSIX Only)

```cpp
#include <semaphore.h>
#include <fcntl.h>

// Process A — create
sem_t* sp = sem_open("/app_sem", O_CREAT | O_EXCL, 0644, 1);
if (sp == SEM_FAILED) { /* errno */ }
sem_wait(sp);
// critical section
sem_post(sp);
sem_close(sp);     // close this process's handle
sem_unlink("/app_sem");  // remove from system

// Process B — open existing
sem_t* sp = sem_open("/app_sem", 0);  // 0 = open existing
if (sp == SEM_FAILED) { /* errno */ }
sem_wait(sp);
sem_post(sp);
sem_close(sp);     // sem still exists until unlinked

// List (Linux) — all named semaphores in /dev/shm/sem.<name>
// ls /dev/shm/
```

---

## Timed Wait Examples

```cpp
// C++20 — relative duration
bool ok = sem.try_acquire_for(std::chrono::milliseconds(500));
if (!ok) { /* timed out */ }

bool ok2 = sem.try_acquire_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));

// POSIX — absolute time (monotonic or realtime)
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec += 2;   // 2 seconds from now
int rc = sem_timedwait(&sem, &ts);
if (rc == -1 && errno == ETIMEDOUT) { /* timed out */ }
```

---

## Pitfalls

```cpp
// 1. Forgetting sem_destroy / sem_unlink — resource leak
sem_t s; sem_init(&s, 0, 1);
// ... use s ...
sem_destroy(&s);   // MUST call when done

sem_t* sp = sem_open("/foo", O_CREAT, 0644, 1);
sem_close(sp);     // close handle
sem_unlink("/foo"); // remove semaphore (else persists until reboot/explicit unlink)

// 2. sem_timedwait uses absolute CLOCK_REALTIME, not relative duration
// Calling with relative time = almost certainly wrong
struct timespec ts{2, 0};   // WRONG: sec=2, not "2 seconds from now"
clock_gettime(CLOCK_REALTIME, &ts); ts.tv_sec += 2;  // CORRECT

// 3. Counting_semaphore max value is NOT the current value
// The template parameter LeastMaxValue is just the maximum the impl must support
std::counting_semaphore<10> s{0};
s.release(15);  // UB if 15 > LeastMaxValue (implementation may allow or may not)

// 4. Binary semaphore is NOT a mutex
std::binary_semaphore bsem{1};
bsem.acquire();
// ... critical section ...
bsem.release();
// DIFFERENT from mutex: any thread can release it, even one that didn't acquire it
// Useful for signalling; mutex for mutual exclusion

// 5. Semaphore starvation — no fairness guarantee
// sem_post wakes one waiter; which one is implementation-defined (usually FIFO on Linux)

// 6. Spurious wakeup from sem_timedwait on signal
int rc;
do { rc = sem_timedwait(&sem, &ts); } while (rc == -1 && errno == EINTR);

// 7. C++20 semaphore: acquire() is NOT interruptible / cancellable
// If thread holds acquire() and program exits, destructor never called — use try_acquire_for
```

// =============================================================================
// Exercise 24: C++ Threads and Mutexes
// =============================================================================
// Topics: std::thread basics, race conditions, std::mutex/lock_guard,
//         std::condition_variable (bounded queue), thread attributes,
//         std::jthread (C++20 mention), deadlock / spurious wakeup pitfalls
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g -pthread threads.cpp -o threads
// =============================================================================

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <queue>
#include <string>
#include <chrono>
#include <functional>
#include <cassert>

// =============================================================================
// SECTION 1: std::thread basics — create, join, detach, lambda capture
// =============================================================================
//
// std::thread represents an OS thread.  The thread starts immediately on
// construction.  You MUST call join() or detach() before the std::thread
// object is destroyed, otherwise the destructor calls std::terminate().
//
//   join()   — caller blocks until thread finishes.
//   detach() — thread runs independently; caller never waits.
//               After detach(), the std::thread object no longer owns the thread.
//
// Passing arguments: prefer lambda capture over raw pointers (safer, readable).

static void thread_func(int id, const std::string& msg) {
    std::cout << "  [thread " << id << "] " << msg << "\n";
}

static void section1_basics() {
    std::cout << "\n=== Section 1: std::thread basics ===\n";

    // Spawn with a free function
    std::thread t1(thread_func, 1, "Hello from a free-function thread");
    t1.join();

    // Spawn with a lambda — captures by value
    int value = 42;
    std::thread t2([value]() {
        std::cout << "  [thread 2] lambda captured value=" << value << "\n";
    });
    t2.join();

    // Heap-allocated argument via lambda capture (avoids dangling-ref risk)
    auto msg_ptr = std::make_shared<std::string>("heap-allocated message");
    std::thread t3([msg_ptr]() {
        std::cout << "  [thread 3] via shared_ptr: " << *msg_ptr << "\n";
    });
    t3.join();

    // Detached thread: runs to completion independently
    // We sleep briefly so the demo output appears before section 2.
    std::thread t4([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // NOTE: writing to cout from a detached thread can race with main.
        // Acceptable here because we sleep enough; in production use a mutex.
        std::cout << "  [thread 4] detached thread finishing\n";
    });
    t4.detach();
    // After detach() the handle is no longer joinable.
    std::cout << "  Main: t4 detached, continuing...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::cout << "  std::thread::hardware_concurrency() = "
              << std::thread::hardware_concurrency() << "\n";
}

// =============================================================================
// SECTION 2: Race condition — 4 threads × 500K increments of plain long
// =============================================================================
//
// A plain long increment (++counter) compiles to:
//   load  counter → register
//   add   1 → register
//   store register → counter
// This is a read-modify-write: not atomic.  When two threads interleave these
// steps the result is a "lost update" — final count is less than expected.

static long g_racy_counter = 0;

static void racy_increment(long n) {
    for (long i = 0; i < n; ++i)
        ++g_racy_counter;   // data race — undefined behaviour
}

static void section2_race_condition() {
    std::cout << "\n=== Section 2: Race condition (plain long, no mutex) ===\n";

    const int  NUM_THREADS = 4;
    const long PER_THREAD  = 500'000L;
    const long EXPECTED    = NUM_THREADS * PER_THREAD;

    g_racy_counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; ++i)
        threads.emplace_back(racy_increment, PER_THREAD);
    for (auto& t : threads) t.join();

    std::cout << "  Expected: " << EXPECTED << "\n";
    std::cout << "  Got:      " << g_racy_counter << "\n";
    if (g_racy_counter != EXPECTED)
        std::cout << "  Lost updates: " << (EXPECTED - g_racy_counter) << "  (race!)\n";
    else
        std::cout << "  (Got correct result by luck — races are non-deterministic)\n";
}

// =============================================================================
// SECTION 3: std::mutex + std::lock_guard — always exactly 2 000 000
// =============================================================================
//
// std::mutex protects a critical section: only one thread holds the lock at
// a time.  std::lock_guard is an RAII (Resource Acquisition Is Initialisation) wrapper: acquires on construction,
// releases when it goes out of scope — even if an exception is thrown.
//
// Rule: always protect every access to shared mutable data with the same mutex.

static long      g_safe_counter = 0;
static std::mutex g_counter_mutex;

static void safe_increment(long n) {
    for (long i = 0; i < n; ++i) {
        std::lock_guard<std::mutex> lk(g_counter_mutex);  // acquire
        ++g_safe_counter;
    }   // lk destroyed here — releases mutex
}

static void section3_mutex() {
    std::cout << "\n=== Section 3: std::mutex + std::lock_guard ===\n";

    const int  NUM_THREADS = 4;
    const long PER_THREAD  = 500'000L;
    const long EXPECTED    = NUM_THREADS * PER_THREAD;

    g_safe_counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; ++i)
        threads.emplace_back(safe_increment, PER_THREAD);
    for (auto& t : threads) t.join();

    std::cout << "  Expected: " << EXPECTED << "\n";
    std::cout << "  Got:      " << g_safe_counter << "\n";
    assert(g_safe_counter == EXPECTED);
    std::cout << "  Assertion passed — mutex guarantees correct count.\n";
}

// =============================================================================
// SECTION 4: std::condition_variable — bounded queue (1 producer, 2 consumers)
// =============================================================================
//
// std::condition_variable works with std::unique_lock<std::mutex>:
//   wait(lock, predicate) — atomically releases the lock and suspends.
//                           Wakes when notified AND predicate is true.
//                           (The predicate loop protects against spurious wakeups.)
//   notify_one()          — wake one waiting thread.
//   notify_all()          — wake all waiting threads.
//
// Bounded queue pattern:
//   Producer waits if queue is full.
//   Consumer waits if queue is empty.
//   A "done" flag tells consumers when no more items will arrive.

static const size_t QUEUE_CAPACITY = 4;

struct BoundedQueue {
    std::queue<int>         q;
    std::mutex              mtx;
    std::condition_variable cv_not_full;   // producer waits here
    std::condition_variable cv_not_empty;  // consumers wait here
    bool                    done = false;
};

static void producer(BoundedQueue& bq, int n) {
    for (int i = 1; i <= n; ++i) {
        std::unique_lock<std::mutex> lk(bq.mtx);
        // Wait while full — spurious wakeup safe because of the while-predicate
        bq.cv_not_full.wait(lk, [&bq]{ return bq.q.size() < QUEUE_CAPACITY; });
        bq.q.push(i);
        std::cout << "  [producer] pushed " << i
                  << "  (queue size=" << bq.q.size() << ")\n";
        lk.unlock();
        bq.cv_not_empty.notify_all();
    }
    // Signal consumers that no more items will arrive
    {
        std::lock_guard<std::mutex> lk(bq.mtx);
        bq.done = true;
    }
    bq.cv_not_empty.notify_all();
}

static void consumer(BoundedQueue& bq, int id) {
    while (true) {
        std::unique_lock<std::mutex> lk(bq.mtx);
        // Wait while empty AND not done
        bq.cv_not_empty.wait(lk, [&bq]{
            return !bq.q.empty() || bq.done;
        });
        if (bq.q.empty()) break;  // done and no more items
        int item = bq.q.front();
        bq.q.pop();
        std::cout << "  [consumer " << id << "] consumed " << item
                  << "  (queue size=" << bq.q.size() << ")\n";
        lk.unlock();
        bq.cv_not_full.notify_one();
    }
    std::cout << "  [consumer " << id << "] exiting\n";
}

static void section4_condition_variable() {
    std::cout << "\n=== Section 4: std::condition_variable (bounded queue) ===\n";
    std::cout << "  Queue capacity=" << QUEUE_CAPACITY
              << ", 1 producer, 2 consumers, 6 items\n";

    BoundedQueue bq;
    std::thread prod(producer, std::ref(bq), 6);
    std::thread cons1(consumer, std::ref(bq), 1);
    std::thread cons2(consumer, std::ref(bq), 2);

    prod.join();
    cons1.join();
    cons2.join();

    std::cout << "  All threads finished.\n";
}

// =============================================================================
// SECTION 5: Thread attributes — detached thread, std::jthread (C++20 mention)
// =============================================================================
//
// std::thread has no built-in stack-size control (use pthreads directly for that).
//
// std::jthread (C++20):
//   - Automatically joins in its destructor (no more "forgot to join" bugs).
//   - Supports cooperative cancellation via std::stop_token.
//   - Use with -std=c++20.

static void section5_attributes() {
    std::cout << "\n=== Section 5: Thread attributes and std::jthread ===\n";

    // Detached thread: runs independently, must not touch caller's stack vars
    {
        // Capture a shared_ptr so the data outlives this scope safely
        auto data = std::make_shared<int>(999);
        std::thread t([data]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::cout << "  [detached] data=" << *data << "\n";
        });
        t.detach();
        std::cout << "  Detached thread launched with shared_ptr capture.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    // std::jthread mention (C++20)
    std::cout << "\n  std::jthread (C++20 note):\n";
    std::cout << "    - Joins automatically in destructor — no manual join() needed.\n";
    std::cout << "    - Supports std::stop_token for cooperative cancellation.\n";
    std::cout << "    Example (C++20):\n";
    std::cout << "      std::jthread t([](std::stop_token st) {\n";
    std::cout << "          while (!st.stop_requested()) { /* work */ }\n";
    std::cout << "      });\n";
    std::cout << "      t.request_stop();  // sets the stop token\n";
    std::cout << "      // destructor joins automatically\n";
}

// =============================================================================
// SECTION 6: Pitfalls
// =============================================================================

static void section6_pitfalls() {
    std::cout << "\n=== Section 6: Pitfalls ===\n";

    std::cout << "  1. DEADLOCK — lock ordering:\n";
    std::cout << "     Thread A: lock(m1), lock(m2)\n";
    std::cout << "     Thread B: lock(m2), lock(m1)   <- deadlock!\n";
    std::cout << "     Fix: always acquire mutexes in the same global order,\n";
    std::cout << "          or use std::lock(m1, m2) / std::scoped_lock{m1,m2}.\n";

    std::cout << "\n  2. SPURIOUS WAKEUP — condition_variable:\n";
    std::cout << "     cv.wait() can return without being notified on some OSes.\n";
    std::cout << "     Always use the predicate overload:\n";
    std::cout << "       cv.wait(lock, []{ return !queue.empty(); });  // safe\n";
    std::cout << "     Never:  cv.wait(lock);  then  if (queue.empty()) ...\n";

    std::cout << "\n  3. STACK REFERENCE CAPTURE in detached threads:\n";
    std::cout << "     std::thread t([&local_var]() { use(local_var); });\n";
    std::cout << "     t.detach();  // local_var may be destroyed before thread runs!\n";
    std::cout << "     Fix: capture by value or use shared_ptr.\n";

    std::cout << "\n  4. FORGOT JOIN/DETACH:\n";
    std::cout << "     If std::thread is destroyed while still joinable,\n";
    std::cout << "     std::terminate() is called — the program crashes.\n";
    std::cout << "     Fix: use std::jthread (C++20) or RAII wrapper.\n";

    std::cout << "\n  5. DATA RACE on non-atomic shared variable:\n";
    std::cout << "     Undefined behaviour — result may be wrong, vary per run,\n";
    std::cout << "     or crash.  Use mutex, std::atomic<>, or std::jthread.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 24: C++ Threads and Mutexes ===\n";

    section1_basics();
    section2_race_condition();
    section3_mutex();
    section4_condition_variable();
    section5_attributes();
    section6_pitfalls();

    std::cout << "\nDone.\n";
    return 0;
}

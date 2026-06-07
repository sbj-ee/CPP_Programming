// =============================================================================
// Exercise 30: Semaphores — POSIX sem_t and C++20 std::counting_semaphore
// =============================================================================
// Topics: concepts, unnamed sem_t basics, binary semaphore signal pattern,
//         counting semaphore resource pool, sem_timedwait, named semaphores
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++20 -g -pthread semaphores.cpp -o semaphores
// Falls back to POSIX sem_t when std::counting_semaphore is unavailable.
// =============================================================================

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <thread>
#include <vector>
#include <chrono>

#include <semaphore.h>      // POSIX sem_t
#include <fcntl.h>          // O_CREAT, O_EXCL for named semaphores
#include <sys/stat.h>       // mode constants
#include <ctime>            // clock_gettime, timespec
#include <unistd.h>
#include <sys/wait.h>       // waitpid

// C++20 std::counting_semaphore / std::binary_semaphore
#if __cpp_lib_semaphore >= 201907L
#  include <semaphore>
#  define HAVE_CPP_SEMAPHORE 1
#else
#  define HAVE_CPP_SEMAPHORE 0
#endif

// =============================================================================
// SECTION 1: Concepts
// =============================================================================
//
// A semaphore is a non-negative integer counter with two atomic operations:
//   wait (P, down, acquire): decrement; block if 0 until another thread posts.
//   post (V, up, release):   increment; wake one blocked waiter if any.
//
// POSIX provides:
//   Unnamed: sem_init / sem_wait / sem_post / sem_destroy  (within a process)
//   Named:   sem_open / sem_wait / sem_post / sem_close / sem_unlink (cross-process)
//
// C++20 adds:
//   std::binary_semaphore    — max count 1; equivalent to POSIX unnamed binary sem
//   std::counting_semaphore<N> — max count N; equivalent to counting sem

static void section1_concepts() {
    std::cout << "\n=== Section 1: Semaphore concepts ===\n";

    std::cout << "  Semaphore = non-negative integer + wait + post (atomic).\n";
    std::cout << "  wait(): if counter > 0, decrement and continue;\n";
    std::cout << "          if counter == 0, block until another thread posts.\n";
    std::cout << "  post(): increment counter; wake one blocked waiter.\n\n";

    std::cout << "  Binary semaphore (max=1): used as a mutex or signal.\n";
    std::cout << "  Counting semaphore (max=N): models N-slot resource pool.\n\n";

    std::cout << "  POSIX unnamed:  sem_init / sem_wait / sem_post / sem_destroy\n";
    std::cout << "  POSIX named:    sem_open / sem_wait / sem_post / sem_close / sem_unlink\n";
    std::cout << "  C++20:          std::counting_semaphore<N>, std::binary_semaphore\n\n";

#if HAVE_CPP_SEMAPHORE
    std::cout << "  C++20 <semaphore> detected on this compiler.\n";
#else
    std::cout << "  C++20 <semaphore> NOT available — using POSIX sem_t fallback.\n";
#endif
}

// =============================================================================
// SECTION 2: Unnamed sem_t basics (sem_init / sem_wait / sem_post / sem_destroy)
// =============================================================================

static void section2_unnamed_basics() {
    std::cout << "\n=== Section 2: Unnamed sem_t basics ===\n";

    sem_t sem;
    // sem_init(sem, pshared, initial_value)
    // pshared=0: shared between threads of the process only.
    // pshared=1: shared between processes (must live in shared memory).
    if (sem_init(&sem, 0, 3) == -1) {
        std::cerr << "sem_init: " << std::strerror(errno) << "\n";
        return;
    }

    int val = 0;
    sem_getvalue(&sem, &val);
    std::cout << "  Initial value: " << val << "\n";

    sem_wait(&sem);  // decrement: 3 -> 2
    sem_getvalue(&sem, &val);
    std::cout << "  After sem_wait: " << val << "\n";

    sem_wait(&sem);  // 2 -> 1
    sem_getvalue(&sem, &val);
    std::cout << "  After sem_wait: " << val << "\n";

    sem_post(&sem);  // 1 -> 2
    sem_getvalue(&sem, &val);
    std::cout << "  After sem_post: " << val << "\n";

    // sem_trywait: non-blocking; returns EAGAIN if would block
    for (int i = 0; i < 3; ++i) {
        int rc = sem_trywait(&sem);
        sem_getvalue(&sem, &val);
        if (rc == 0)
            std::cout << "  sem_trywait succeeded; value now " << val << "\n";
        else
            std::cout << "  sem_trywait would block (EAGAIN); value=" << val << "\n";
    }

    sem_destroy(&sem);
    std::cout << "  sem_destroy called.\n";
}

// =============================================================================
// SECTION 3: Binary semaphore — producer / consumer signal pattern
// =============================================================================
//
// A binary semaphore initialized to 0 acts as a "ready signal":
//   Consumer blocks on wait() until producer post()s.

static void section3_binary_signal() {
    std::cout << "\n=== Section 3: Binary semaphore — producer/consumer ===\n";

#if HAVE_CPP_SEMAPHORE
    std::cout << "  Using std::binary_semaphore (C++20)\n";

    std::binary_semaphore ready{0};  // start at 0 — consumer blocks
    int shared_data = 0;

    std::thread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        shared_data = 42;
        std::cout << "  [producer] produced data=" << shared_data << "\n";
        ready.release();  // signal consumer
    });

    std::thread consumer([&]() {
        ready.acquire();  // block until producer posts
        std::cout << "  [consumer] consumed data=" << shared_data << "\n";
    });

    producer.join();
    consumer.join();
#else
    std::cout << "  Using POSIX sem_t binary semaphore\n";

    sem_t ready;
    sem_init(&ready, 0, 0);  // initial value 0 — consumer blocks
    int shared_data = 0;

    std::thread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        shared_data = 42;
        std::cout << "  [producer] produced data=" << shared_data << "\n";
        sem_post(&ready);
    });

    std::thread consumer([&]() {
        sem_wait(&ready);
        std::cout << "  [consumer] consumed data=" << shared_data << "\n";
    });

    producer.join();
    consumer.join();
    sem_destroy(&ready);
#endif
}

// =============================================================================
// SECTION 4: Counting semaphore — resource pool (N = 3 slots)
// =============================================================================
//
// Initialized to N: represents N identical resources.
// Each consumer acquires one slot (waits), uses it, then releases (posts).

static void section4_resource_pool() {
    std::cout << "\n=== Section 4: Counting semaphore — resource pool (3 slots) ===\n";

    const int SLOTS  = 3;
    const int WORKERS = 6;

#if HAVE_CPP_SEMAPHORE
    std::cout << "  Using std::counting_semaphore<3> (C++20)\n";
    std::counting_semaphore<SLOTS> pool{SLOTS};

    std::mutex print_mtx;
    std::vector<std::thread> tv;
    for (int i = 0; i < WORKERS; ++i) {
        tv.emplace_back([&, i]() {
            pool.acquire();
            {
                std::lock_guard<std::mutex> lk(print_mtx);
                std::cout << "  worker " << i << " acquired slot\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            {
                std::lock_guard<std::mutex> lk(print_mtx);
                std::cout << "  worker " << i << " releasing slot\n";
            }
            pool.release();
        });
    }
    for (auto& t : tv) t.join();
#else
    std::cout << "  Using POSIX sem_t counting semaphore\n";
    sem_t pool;
    sem_init(&pool, 0, static_cast<unsigned>(SLOTS));

    std::mutex print_mtx;
    std::vector<std::thread> tv;
    for (int i = 0; i < WORKERS; ++i) {
        tv.emplace_back([&, i]() {
            sem_wait(&pool);
            {
                std::lock_guard<std::mutex> lk(print_mtx);
                std::cout << "  worker " << i << " acquired slot\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            {
                std::lock_guard<std::mutex> lk(print_mtx);
                std::cout << "  worker " << i << " releasing slot\n";
            }
            sem_post(&pool);
        });
    }
    for (auto& t : tv) t.join();
    sem_destroy(&pool);
#endif

    std::cout << "  All workers done.  At most " << SLOTS << " ran concurrently.\n";
}

// =============================================================================
// SECTION 5: sem_timedwait — bounded wait
// =============================================================================
//
// sem_timedwait(sem, abs_timeout) blocks until:
//   - The semaphore becomes available, OR
//   - The absolute clock reaches abs_timeout
// On timeout: returns -1, errno = ETIMEDOUT.
// Note: abs_timeout is ABSOLUTE (CLOCK_REALTIME), not a relative offset.

static void section5_timedwait() {
    std::cout << "\n=== Section 5: sem_timedwait — bounded wait ===\n";

    sem_t sem;
    sem_init(&sem, 0, 0);  // starts at 0; wait will block

    // Compute absolute deadline: now + 100 ms
    timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 100'000'000L;  // 100 ms
    if (deadline.tv_nsec >= 1'000'000'000L) {
        deadline.tv_nsec -= 1'000'000'000L;
        deadline.tv_sec  += 1;
    }

    std::cout << "  Calling sem_timedwait (100 ms timeout, semaphore at 0)...\n";
    int rc = sem_timedwait(&sem, &deadline);
    if (rc == -1 && errno == ETIMEDOUT) {
        std::cout << "  Timed out as expected (ETIMEDOUT).\n";
    } else if (rc == 0) {
        std::cout << "  Unexpectedly acquired semaphore.\n";
    } else {
        std::cout << "  Error: " << std::strerror(errno) << "\n";
    }

    // Now post and verify immediate acquisition
    sem_post(&sem);
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 5;  // generous 5-second deadline
    rc = sem_timedwait(&sem, &deadline);
    std::cout << "  After post, sem_timedwait returned "
              << (rc == 0 ? "success" : "failure") << "\n";

    sem_destroy(&sem);
}

// =============================================================================
// SECTION 6: Named semaphores — cross-process
// =============================================================================
//
// Named semaphores persist in the kernel (visible in /dev/shm/ on Linux) until
// sem_unlink() is called.  Any process that knows the name can open one.

static const char* SEM_NAME = "/ex30_cpp_demo";

static void section6_named() {
    std::cout << "\n=== Section 6: Named semaphores (cross-process) ===\n";

    // Remove any leftover from a previous run
    sem_unlink(SEM_NAME);

    // Create with initial value 0
    sem_t* sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 0);
    if (sem == SEM_FAILED) {
        std::cerr << "sem_open(create): " << std::strerror(errno) << "\n";
        return;
    }
    std::cout << "  Created named semaphore: " << SEM_NAME << "\n";

    pid_t pid = ::fork();
    if (pid == 0) {
        // CHILD: open by name, post, close
        sem_t* csem = sem_open(SEM_NAME, 0);
        if (csem == SEM_FAILED) {
            std::cerr << "[child] sem_open: " << std::strerror(errno) << "\n";
            ::_exit(1);
        }
        std::cout << "  [child]  posting named semaphore\n";
        std::cout.flush();
        sem_post(csem);
        sem_close(csem);
        ::_exit(0);
    }

    // PARENT: wait on the semaphore
    std::cout << "  [parent] waiting on named semaphore (child will post)...\n";
    sem_wait(sem);
    std::cout << "  [parent] received signal from child!\n";

    int status;
    ::waitpid(pid, &status, 0);

    sem_close(sem);
    sem_unlink(SEM_NAME);  // remove the name from the kernel
    std::cout << "  Named semaphore unlinked.\n";

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. sem_destroy() on an unnamed semaphore that still has waiting\n";
    std::cout << "     threads is undefined behaviour — always drain waiters first.\n";
    std::cout << "  2. Named semaphore names must start with '/'; only one '/' allowed.\n";
    std::cout << "  3. sem_timedwait uses CLOCK_REALTIME absolute time — compute with\n";
    std::cout << "     clock_gettime(CLOCK_REALTIME, &ts) + offset.\n";
    std::cout << "  4. C++20 std::counting_semaphore::try_acquire_for() uses relative\n";
    std::cout << "     durations (std::chrono) — much easier than POSIX timedwait.\n";
    std::cout << "  5. Semaphores and mutexes solve different problems:\n";
    std::cout << "     mutex — ownership (the locker must unlock).\n";
    std::cout << "     semaphore — signaling (any thread can post).\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 30: Semaphores ===\n";

    section1_concepts();
    section2_unnamed_basics();
    section3_binary_signal();
    section4_resource_pool();
    section5_timedwait();
    section6_named();

    std::cout << "\nDone.\n";
    return 0;
}

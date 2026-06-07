// =============================================================================
// Exercise 23: POSIX (Portable Operating System Interface) Signals in C++
// =============================================================================
// Topics: signal table, signal()/raise()/SIG_IGN, sig_atomic_t flag pattern,
//         sigaction() with SA_RESTART/SA_SIGINFO, sigprocmask blocking,
//         SIGALRM/alarm() timeout, async-signal safety
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g signals.cpp -o signals
// POSIX feature macros must be defined before any system headers.
// =============================================================================

#define _POSIX_C_SOURCE 200809L

#include <iostream>
#include <csignal>      // std::signal, std::raise, sig_atomic_t, SIG_IGN, SIG_DFL
#include <cstring>      // strsignal
#include <unistd.h>     // alarm(), write(), pause(), STDOUT_FILENO
#include <sys/types.h>

// =============================================================================
// SECTION 1: Signal Table
// =============================================================================
//
// Signals are asynchronous notifications delivered to a process.  They can
// originate from the kernel (hardware faults), the terminal (Ctrl-C), another
// process (kill()), or the process itself (raise()).
//
// The 13 POSIX-guaranteed signals every C++ programmer should know:

struct SigEntry {
    int         number;
    const char* name;
    const char* description;
};

static const SigEntry SIGNAL_TABLE[] = {
    { SIGHUP,  "SIGHUP",  "Hangup — terminal closed or daemon reload"         },
    { SIGINT,  "SIGINT",  "Interrupt — Ctrl-C from terminal"                  },
    { SIGQUIT, "SIGQUIT", "Quit — Ctrl-\\ ; produces core dump"               },
    { SIGILL,  "SIGILL",  "Illegal instruction — invalid CPU opcode"           },
    { SIGABRT, "SIGABRT", "Abort — raised by abort(), assert failure"          },
    { SIGFPE,  "SIGFPE",  "FP/integer exception — divide by zero, overflow"   },
    { SIGKILL, "SIGKILL", "Kill — cannot be caught, blocked, or ignored"       },
    { SIGSEGV, "SIGSEGV", "Segfault — invalid memory reference"               },
    { SIGPIPE, "SIGPIPE", "Broken pipe — write to pipe with no readers"        },
    { SIGALRM, "SIGALRM", "Alarm — raised by alarm() timer expiry"             },
    { SIGTERM, "SIGTERM", "Terminate — polite shutdown request (kill default)" },
    { SIGUSR1, "SIGUSR1", "User-defined signal 1"                              },
    { SIGUSR2, "SIGUSR2", "User-defined signal 2"                              },
};

static void print_signal_table() {
    std::cout << "\n=== Section 1: Signal Table ===\n";
    std::cout << "  " << "Num" << "  " << "Name      " << "  " << "Description\n";
    std::cout << "  " << "---" << "  " << "----------" << "  "
              << "-------------------------------------------\n";
    for (const auto& e : SIGNAL_TABLE) {
        // Print number (field width 3), name (width 10), description
        std::cout << "  ";
        if (e.number < 10)  std::cout << " ";
        if (e.number < 100) std::cout << " ";
        std::cout << e.number << "  " << e.name;
        // Pad name to 10 chars
        for (int i = static_cast<int>(std::strlen(e.name)); i < 10; ++i)
            std::cout << ' ';
        std::cout << "  " << e.description << "\n";
    }
}

// =============================================================================
// SECTION 2: signal() / raise() / SIG_IGN
// =============================================================================
//
// signal() installs a simple signal handler.  It is the oldest API; prefer
// sigaction() for production code (signal() has unspecified semantics for
// concurrent signals on some platforms).
//
// SIG_DFL restores the default action.
// SIG_IGN causes the signal to be silently discarded.
// raise()  sends a signal to the calling process.

static void simple_handler(int signum) {
    // Only async-signal-safe functions here!
    // write() is safe; std::cout is NOT (it may hold internal locks).
    const char msg[] = "  [handler] simple_handler: received signal\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    (void)signum;  // suppress unused-parameter warning
}

static void section2_signal_raise() {
    std::cout << "\n=== Section 2: signal() / raise() / SIG_IGN ===\n";

    // Install a handler for SIGUSR1
    if (std::signal(SIGUSR1, simple_handler) == SIG_ERR) {
        std::cerr << "signal() failed\n";
        return;
    }
    std::cout << "  Raising SIGUSR1 (handler installed):\n";
    std::raise(SIGUSR1);  // delivers SIGUSR1 to this process

    // Ignore SIGUSR2: the signal is silently discarded
    std::signal(SIGUSR2, SIG_IGN);
    std::cout << "  Raising SIGUSR2 (ignored — no output expected):\n";
    std::raise(SIGUSR2);
    std::cout << "  (returned normally after ignored SIGUSR2)\n";

    // Restore defaults
    std::signal(SIGUSR1, SIG_DFL);
    std::signal(SIGUSR2, SIG_DFL);
}

// =============================================================================
// SECTION 3: sig_atomic_t flag pattern
// =============================================================================
//
// A signal handler and the main program share memory.  Ordinary variables are
// NOT safe to write in a handler because the compiler can reorder or cache
// accesses.  Use volatile std::sig_atomic_t (guaranteed to be read/written
// atomically relative to signal delivery on the same platform).

static volatile std::sig_atomic_t g_flag = 0;

static void flag_handler(int signum) {
    (void)signum;
    g_flag = 1;   // safe: sig_atomic_t, volatile
}

static void section3_sig_atomic() {
    std::cout << "\n=== Section 3: sig_atomic_t flag pattern ===\n";

    std::signal(SIGUSR1, flag_handler);

    std::cout << "  g_flag before raise: " << static_cast<int>(g_flag) << "\n";
    std::raise(SIGUSR1);
    std::cout << "  g_flag after raise:  " << static_cast<int>(g_flag) << "\n";

    // Pattern used in real programs:
    //   while (!g_flag) { /* do work */ }
    //   // clean up when flag is set

    std::signal(SIGUSR1, SIG_DFL);
    g_flag = 0;
}

// =============================================================================
// SECTION 4: sigaction() — SA_RESTART, SA_SIGINFO, sigprocmask
// =============================================================================
//
// sigaction() provides portable, well-defined signal handling:
//   SA_RESTART  — automatically restart interrupted system calls (read, write…)
//   SA_SIGINFO  — handler receives extra info via siginfo_t (sender PID, etc.)
//   sigprocmask — block/unblock a set of signals (critical sections)

static void siginfo_handler(int signum, siginfo_t* info, void* /*context*/) {
    // Only async-signal-safe calls!
    char buf[128];
    int  n = snprintf(buf, sizeof(buf),
        "  [SA_SIGINFO handler] sig=%d from pid=%d\n",
        signum, static_cast<int>(info->si_pid));
    write(STDOUT_FILENO, buf, static_cast<size_t>(n));
}

static void section4_sigaction() {
    std::cout << "\n=== Section 4: sigaction() / sigprocmask ===\n";

    // --- SA_RESTART + SA_SIGINFO ---
    struct sigaction sa = {};
    sa.sa_sigaction = siginfo_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
        std::cerr << "sigaction() failed\n";
        return;
    }

    std::cout << "  Raising SIGUSR1 with SA_SIGINFO handler:\n";
    std::raise(SIGUSR1);

    // --- sigprocmask: block SIGUSR1 during a critical section ---
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGUSR1);

    // Block SIGUSR1; save old mask in old_set
    sigprocmask(SIG_BLOCK, &block_set, &old_set);
    std::cout << "  SIGUSR1 blocked. Raising it now (will be pending)...\n";
    std::raise(SIGUSR1);   // signal is now pending, not delivered yet
    std::cout << "  Still running in critical section (signal pending).\n";

    // Unblock: pending signal is delivered immediately
    std::cout << "  Unblocking SIGUSR1 — pending signal will fire now:\n";
    sigprocmask(SIG_SETMASK, &old_set, nullptr);
    std::cout << "  Returned from unblock.\n";

    // Restore default
    struct sigaction sa_dfl = {};
    sa_dfl.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl.sa_mask);
    sigaction(SIGUSR1, &sa_dfl, nullptr);
}

// =============================================================================
// SECTION 5: SIGALRM / alarm() — busy-work timeout
// =============================================================================
//
// alarm(n) asks the kernel to deliver SIGALRM to this process after n seconds.
// Only one alarm is pending at a time; alarm(0) cancels it.
// The handler uses only async-signal-safe write().

static volatile std::sig_atomic_t g_alarm_fired = 0;

static void alarm_handler(int /*signum*/) {
    g_alarm_fired = 1;
    const char msg[] = "  [alarm_handler] SIGALRM fired — stopping busy work\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

static void section5_sigalrm() {
    std::cout << "\n=== Section 5: SIGALRM / alarm() ===\n";

    std::signal(SIGALRM, alarm_handler);
    g_alarm_fired = 0;

    std::cout << "  Setting alarm for 1 second, then doing busy work...\n";
    alarm(1);  // SIGALRM in 1 second

    // Busy work loop — runs until alarm fires
    volatile long counter = 0;
    while (!g_alarm_fired) {
        ++counter;
    }

    std::cout << "  Busy loop counted to " << counter
              << " before alarm fired.\n";

    alarm(0);  // cancel any remaining alarm
    std::signal(SIGALRM, SIG_DFL);
}

// =============================================================================
// SECTION 6: Async-signal safety
// =============================================================================
//
// Only async-signal-safe functions may be called from a signal handler.
// The POSIX standard enumerates a specific list (~180 functions); everything
// else — including std::cout, malloc, printf, new/delete — is UNSAFE because:
//   • They may be interrupted mid-execution and re-entered, corrupting state.
//   • They may hold internal locks (e.g., cout's streambuf mutex) that are
//     already held by the interrupted thread, causing deadlock.
//
// Safe minimal idiom in a handler:
//   write(STDOUT_FILENO, buf, n);   // system call — always safe
//   _exit(1);                       // _exit is safe; exit() is NOT
//
// Everything else should happen outside the handler:
//   set a volatile sig_atomic_t flag → inspect it in the main loop.

static void section6_async_safety() {
    std::cout << "\n=== Section 6: Async-signal safety ===\n";

    std::cout << "  Safe in a signal handler (POSIX async-signal-safe):\n";
    std::cout << "    write()        — raw syscall, no buffering\n";
    std::cout << "    _exit()        — immediate exit without stdio flush\n";
    std::cout << "    read()         — raw syscall\n";
    std::cout << "    sigprocmask()  — mask manipulation\n";
    std::cout << "    sem_post()     — POSIX semaphore post\n";
    std::cout << "    kill()         — send a signal\n";
    std::cout << "    snprintf()     — safe IF no heap allocation path is taken\n";
    std::cout << "      (use only with fixed buffers; do not use %s with %m)\n";

    std::cout << "\n  UNSAFE in a signal handler:\n";
    std::cout << "    std::cout / std::cerr / printf — use internal locks\n";
    std::cout << "    malloc / new / delete          — heap lock\n";
    std::cout << "    exit() / std::exit()           — flushes atexit handlers\n";
    std::cout << "    anything that holds a mutex    — deadlock risk\n";
    std::cout << "    longjmp() into non-handler     — undefined on most platforms\n";

    std::cout << "\n  Best practice:\n";
    std::cout << "    Handler: set volatile sig_atomic_t flag only.\n";
    std::cout << "    Main loop: check flag, do real work outside handler.\n";
    std::cout << "    Alternative: use self-pipe trick or signalfd() on Linux.\n";

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. Prefer sigaction() over signal(): signal() has implementation-\n"
              << "     defined semantics for concurrent signals and does not restart\n"
              << "     interrupted system calls on all platforms.\n";
    std::cout << "  2. SA_RESTART restarts most slow syscalls automatically; without\n"
              << "     it, read()/write() return EINTR when a signal arrives.\n";
    std::cout << "  3. sig_atomic_t must be volatile — the compiler must not cache it\n"
              << "     in a register across a potential signal delivery point.\n";
    std::cout << "  4. SIGKILL and SIGSTOP can never be caught, blocked, or ignored.\n";
    std::cout << "  5. Multiple pending identical signals (non-real-time) may coalesce\n"
              << "     to a single delivery — never rely on count.\n";
    std::cout << "  6. The self-pipe trick: write a byte to a pipe in the handler;\n"
              << "     the main loop monitors the pipe with select/poll/epoll.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 23: POSIX Signals in C++ ===\n";

    print_signal_table();
    section2_signal_raise();
    section3_sig_atomic();
    section4_sigaction();
    section5_sigalrm();
    section6_async_safety();

    std::cout << "\nDone.\n";
    return 0;
}

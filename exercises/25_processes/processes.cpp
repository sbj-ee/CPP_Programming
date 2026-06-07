// =============================================================================
// Exercise 25: POSIX (Portable Operating System Interface) Process Control in C++
// =============================================================================
// Topics: fork/waitpid, execvp, pipes (unidirectional + bidirectional),
//         popen/pclose, RAII pipe wrapper, zombie reaping, FD_CLOEXEC
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g processes.cpp -o processes
// =============================================================================

#define _POSIX_C_SOURCE 200809L

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>      // strerror
#include <cerrno>

#include <unistd.h>     // fork, execvp, pipe, read, write, close, dup2
#include <sys/types.h>
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED
#include <fcntl.h>      // fcntl, F_SETFD, FD_CLOEXEC
#include <cstdio>       // FILE*, popen, pclose

// =============================================================================
// RAII Pipe Wrapper
// =============================================================================
//
// Encapsulates a pipe pair {read_fd, write_fd} and closes both ends in the
// destructor.  Individual ends can be released (moved out) with release_read()
// / release_write() when they are transferred to another process context.

class PipePair {
public:
    PipePair() {
        int fds[2];
        if (::pipe(fds) == -1) {
            std::cerr << "pipe() failed: " << std::strerror(errno) << "\n";
            read_fd_  = -1;
            write_fd_ = -1;
        } else {
            read_fd_  = fds[0];
            write_fd_ = fds[1];
        }
    }

    // Non-copyable; movable
    PipePair(const PipePair&)            = delete;
    PipePair& operator=(const PipePair&) = delete;
    PipePair(PipePair&& o) noexcept
        : read_fd_(o.read_fd_), write_fd_(o.write_fd_) {
        o.read_fd_ = o.write_fd_ = -1;
    }

    ~PipePair() {
        close_read();
        close_write();
    }

    int  read_fd()  const { return read_fd_;  }
    int  write_fd() const { return write_fd_; }
    bool ok()       const { return read_fd_ >= 0 && write_fd_ >= 0; }

    void close_read()  { if (read_fd_  >= 0) { ::close(read_fd_);  read_fd_  = -1; } }
    void close_write() { if (write_fd_ >= 0) { ::close(write_fd_); write_fd_ = -1; } }

    // Transfer ownership of one end; caller is now responsible for closing
    int release_read()  { int fd = read_fd_;  read_fd_  = -1; return fd; }
    int release_write() { int fd = write_fd_; write_fd_ = -1; return fd; }

private:
    int read_fd_;
    int write_fd_;
};

// =============================================================================
// SECTION 1: fork() — process identity
// =============================================================================
//
// fork() duplicates the calling process.  The child gets a copy-on-write
// image of the parent's memory.  Both continue executing from the line after
// fork().  Return value distinguishes the two:
//   == 0  in child
//   > 0   in parent (return value is child PID)
//   == -1 fork failed

static void section1_fork() {
    std::cout << "\n=== Section 1: fork() and process identity ===\n";

    pid_t pid = ::fork();
    if (pid == -1) {
        std::cerr << "fork failed: " << std::strerror(errno) << "\n";
        return;
    }

    if (pid == 0) {
        // CHILD
        std::cout << "  [child]  PID=" << ::getpid()
                  << "  PPID=" << ::getppid() << "\n";
        std::cout << "  [child]  fork() returned 0\n";
        std::cout.flush();
        ::_exit(0);   // use _exit in child to avoid double-flush of stdio buffers
    } else {
        // PARENT
        std::cout << "  [parent] PID=" << ::getpid()
                  << "  child PID=" << pid << "\n";
        std::cout << "  [parent] fork() returned child PID=" << pid << "\n";
        int status;
        ::waitpid(pid, &status, 0);
        std::cout << "  [parent] child exited cleanly\n";
    }
}

// =============================================================================
// SECTION 2: waitpid() — exit status decoding
// =============================================================================
//
// WIFEXITED(status)   — child called exit/return
// WEXITSTATUS(status) — exit code (valid only if WIFEXITED)
// WIFSIGNALED(status) — child killed by signal
// WTERMSIG(status)    — which signal (valid only if WIFSIGNALED)

static void decode_status(int status) {
    if (WIFEXITED(status)) {
        std::cout << "  exited normally, code=" << WEXITSTATUS(status) << "\n";
    } else if (WIFSIGNALED(status)) {
        std::cout << "  killed by signal " << WTERMSIG(status) << "\n";
    } else {
        std::cout << "  unknown status\n";
    }
}

static void section2_waitpid() {
    std::cout << "\n=== Section 2: waitpid() and exit status decoding ===\n";

    // Child exits with code 42
    pid_t p1 = ::fork();
    if (p1 == 0) { ::_exit(42); }
    int st1;
    ::waitpid(p1, &st1, 0);
    std::cout << "  Child exited with code 42: ";
    decode_status(st1);

    // Child exits with code 0
    pid_t p2 = ::fork();
    if (p2 == 0) { ::_exit(0); }
    int st2;
    ::waitpid(p2, &st2, 0);
    std::cout << "  Child exited with code 0:  ";
    decode_status(st2);

    // Child killed by SIGKILL
    pid_t p3 = ::fork();
    if (p3 == 0) { ::kill(::getpid(), SIGKILL); ::_exit(0); }
    int st3;
    ::waitpid(p3, &st3, 0);
    std::cout << "  Child killed by SIGKILL:   ";
    decode_status(st3);
}

// =============================================================================
// SECTION 3: execvp() — run echo and a bad program
// =============================================================================
//
// exec replaces the current process image with a new program.
// execvp() searches PATH for the executable (like the shell).
// On success it never returns; on failure it returns -1 and sets errno.
// Status 127 is the conventional "command not found" code used by shells.

static void section3_execvp() {
    std::cout << "\n=== Section 3: execvp() — echo and bad program ===\n";

    // -- echo --
    pid_t p1 = ::fork();
    if (p1 == 0) {
        // Child
        const char* args[] = { "echo", "  [execvp] Hello from echo!", nullptr };
        ::execvp("echo", const_cast<char* const*>(args));
        // execvp returns only on error
        std::cerr << "execvp(echo) failed: " << std::strerror(errno) << "\n";
        ::_exit(127);
    }
    int st1;
    ::waitpid(p1, &st1, 0);
    std::cout << "  echo exit code: " << WEXITSTATUS(st1) << "\n";

    // -- bad program → exit 127 --
    pid_t p2 = ::fork();
    if (p2 == 0) {
        const char* args[] = { "this_program_does_not_exist", nullptr };
        ::execvp("this_program_does_not_exist", const_cast<char* const*>(args));
        ::_exit(127);  // conventional "not found" code
    }
    int st2;
    ::waitpid(p2, &st2, 0);
    std::cout << "  bad-program exit code: " << WEXITSTATUS(st2)
              << "  (127 = command not found)\n";
}

// =============================================================================
// SECTION 4: Pipes — unidirectional and bidirectional
// =============================================================================
//
// A pipe is a one-directional, in-kernel byte stream.
// Unidirectional: parent writes, child reads (or vice versa).
// Bidirectional: two pipes — one each direction.
//
// Close unused ends!  If the parent holds the write end open while waiting
// for the child to read, and the child blocks waiting for EOF, you deadlock.

static void section4_pipes() {
    std::cout << "\n=== Section 4: Pipes (unidirectional + bidirectional) ===\n";

    // --- Unidirectional: parent → child ---
    {
        PipePair pipe;
        if (!pipe.ok()) return;

        pid_t pid = ::fork();
        if (pid == 0) {
            pipe.close_write();  // child doesn't write
            char buf[128] = {};
            ssize_t n = ::read(pipe.read_fd(), buf, sizeof(buf) - 1);
            if (n > 0) buf[n] = '\0';
            std::cout << "  [child unidirectional] received: " << buf << "\n";
            std::cout.flush();
            ::_exit(0);
        }
        pipe.close_read();  // parent doesn't read
        const std::string msg = "Hello from parent via pipe";
        ::write(pipe.write_fd(), msg.c_str(), msg.size());
        pipe.close_write();  // signals EOF to child
        int st; ::waitpid(pid, &st, 0);
        std::cout << "  [parent unidirectional] child exit=" << WEXITSTATUS(st) << "\n";
    }

    // --- Bidirectional: two pipes ---
    {
        PipePair p2c;  // parent-to-child
        PipePair c2p;  // child-to-parent
        if (!p2c.ok() || !c2p.ok()) return;

        pid_t pid = ::fork();
        if (pid == 0) {
            p2c.close_write();
            c2p.close_read();

            char buf[128] = {};
            ssize_t n = ::read(p2c.read_fd(), buf, sizeof(buf) - 1);
            if (n > 0) buf[n] = '\0';

            std::string reply = std::string("[child reply] got: ") + buf;
            ::write(c2p.write_fd(), reply.c_str(), reply.size());
            std::cout.flush();
            ::_exit(0);
        }
        p2c.close_read();
        c2p.close_write();

        const std::string msg = "ping";
        ::write(p2c.write_fd(), msg.c_str(), msg.size());
        p2c.close_write();

        char buf[256] = {};
        ssize_t n = ::read(c2p.read_fd(), buf, sizeof(buf) - 1);
        if (n > 0) buf[n] = '\0';
        std::cout << "  [parent bidirectional] received: " << buf << "\n";

        int st; ::waitpid(pid, &st, 0);
    }
}

// =============================================================================
// SECTION 5: popen() / pclose()
// =============================================================================
//
// popen() creates a child process (via /bin/sh -c) and returns a FILE* stream
// connected to its stdout (mode "r") or stdin (mode "w").
// pclose() waits for the child and returns its exit status.
// Fine to use FILE* directly in C++ — stdio is part of the C++ standard library.

static void section5_popen() {
    std::cout << "\n=== Section 5: popen() / pclose() ===\n";

    // Read command output
    FILE* fp = ::popen("uname -r", "r");
    if (!fp) {
        std::cerr << "popen failed: " << std::strerror(errno) << "\n";
        return;
    }
    char line[256];
    while (std::fgets(line, sizeof(line), fp)) {
        std::cout << "  uname -r: " << line;
    }
    int rc = ::pclose(fp);
    std::cout << "  pclose returned: " << rc << "\n";

    // Count lines in /etc/hostname (safe on any Linux)
    FILE* fp2 = ::popen("wc -l < /etc/hostname 2>/dev/null || echo 0", "r");
    if (fp2) {
        if (std::fgets(line, sizeof(line), fp2))
            std::cout << "  /etc/hostname line count: " << line;
        ::pclose(fp2);
    }
}

// =============================================================================
// SECTION 6: Pitfalls — zombie reaping, FD_CLOEXEC
// =============================================================================

static void section6_pitfalls() {
    std::cout << "\n=== Section 6: Pitfalls — zombie reaping, FD_CLOEXEC ===\n";

    std::cout << "  ZOMBIE PROCESSES:\n";
    std::cout << "    A child that has exited but not been wait()ed for is a zombie.\n";
    std::cout << "    It holds a slot in the process table until the parent calls\n";
    std::cout << "    waitpid().  Accumulating zombies can exhaust PID space.\n";
    std::cout << "    Fix: always waitpid() every forked child.\n";
    std::cout << "    Alternatively: signal(SIGCHLD, SIG_IGN) auto-reaps on Linux.\n";

    std::cout << "\n  FD_CLOEXEC:\n";
    std::cout << "    By default, all open file descriptors are inherited across\n";
    std::cout << "    fork()+exec().  This can leak private sockets, files, or pipe\n";
    std::cout << "    ends into the child program.\n";
    std::cout << "    Fix: set FD_CLOEXEC on every fd you don't want to inherit:\n";
    std::cout << "      fcntl(fd, F_SETFD, FD_CLOEXEC);\n";
    std::cout << "    Better: open files with O_CLOEXEC flag from the start:\n";
    std::cout << "      open(path, O_RDONLY | O_CLOEXEC);\n";
    std::cout << "    Or use pipe2(fds, O_CLOEXEC) instead of pipe().\n";

    // Demonstrate FD_CLOEXEC on a real fd
    int fd = ::open("/dev/null", O_RDONLY);
    if (fd >= 0) {
        int flags = ::fcntl(fd, F_GETFD);
        std::cout << "\n  /dev/null fd=" << fd
                  << "  FD_CLOEXEC before: "
                  << ((flags & FD_CLOEXEC) ? "set" : "not set") << "\n";
        ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        flags = ::fcntl(fd, F_GETFD);
        std::cout << "  /dev/null fd=" << fd
                  << "  FD_CLOEXEC after:  "
                  << ((flags & FD_CLOEXEC) ? "set" : "not set") << "\n";
        ::close(fd);
    }

    std::cout << "\n  DOUBLE-FLUSH with _exit vs exit:\n";
    std::cout << "    In the child after fork(), always use _exit() NOT exit().\n";
    std::cout << "    exit() flushes stdio buffers and runs atexit handlers —\n";
    std::cout << "    both of which may execute code the child should not run.\n";
    std::cout << "    _exit() terminates immediately without flushing.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 25: POSIX Process Control in C++ ===\n";

    section1_fork();
    section2_waitpid();
    section3_execvp();
    section4_pipes();
    section5_popen();
    section6_pitfalls();

    std::cout << "\nDone.\n";
    return 0;
}

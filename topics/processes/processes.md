# Process Control (POSIX) — Cheat Sheet

> POSIX process APIs are identical in C++. C++ adds RAII wrappers, `std::string` for args, and safer resource management.

## Headers

```cpp
#include <unistd.h>     // fork, exec*, pipe, read, write, close
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS
#include <sys/types.h>  // pid_t
#include <fcntl.h>      // O_CLOEXEC, fcntl
#include <errno.h>      // errno
#include <cstring>      // strerror
#include <stdexcept>    // std::runtime_error
#include <string>
#include <vector>
```

---

## `fork()`

```cpp
pid_t pid = fork();
if (pid < 0) {
    throw std::runtime_error(std::string("fork: ") + strerror(errno));
}
if (pid == 0) {
    // Child process — pid == 0
    // Has its own copy of parent's address space (copy-on-write)
    child_work();
    _exit(0);   // use _exit, not exit(), in child to avoid double-flush of stdio
}
// Parent process — pid > 0 (child's PID)
parent_work();
```

---

## `waitpid()`

```cpp
int status;
pid_t result = waitpid(pid, &status, 0);    // block until child exits
// Options:
//   0           — block
//   WNOHANG     — return 0 immediately if no child exited
//   WUNTRACED   — also report stopped children

if (result == -1) {
    throw std::runtime_error(std::string("waitpid: ") + strerror(errno));
}

// Inspect status
if (WIFEXITED(status)) {
    int code = WEXITSTATUS(status);  // 0–255
}
if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);      // signal that killed child
}
if (WIFSTOPPED(status)) {
    int sig = WSTOPSIG(status);
}

// Wait for any child
pid_t any = waitpid(-1, &status, 0);
// Wait for any child in process group pgid
pid_t grp = waitpid(-pgid, &status, 0);
```

---

## `exec()` Family

```cpp
// Replace current process image with new program
execl ("/bin/ls", "ls", "-l", nullptr);      // NULL-terminated arg list
execlp("ls",      "ls", "-l", nullptr);      // search PATH
execv ("/bin/ls", argv);                     // argv[] array, nullptr-terminated
execvp("ls",      argv);                     // search PATH
execve("/bin/ls", argv, envp);               // explicit environment

// execv with std::vector<std::string>
std::vector<std::string> args = {"ls", "-l", "/tmp"};
std::vector<char*> cargs;
for (auto& s : args) cargs.push_back(s.data());
cargs.push_back(nullptr);
execvp(cargs[0], cargs.data());

// exec only returns on error
throw std::runtime_error(std::string("exec: ") + strerror(errno));
```

---

## Pipes

```cpp
int fd[2];
if (pipe(fd) == -1) {
    throw std::runtime_error(std::string("pipe: ") + strerror(errno));
}
// fd[0] = read end, fd[1] = write end

// In parent:
close(fd[0]);           // close read end if parent only writes
write(fd[1], "hello", 5);
close(fd[1]);           // EOF to child when done

// In child (after fork):
close(fd[1]);           // close write end if child only reads
char buf[128];
ssize_t n = read(fd[0], buf, sizeof(buf));
```

### Pipe from Parent to Child (stdin redirect)

```cpp
int pipefd[2];
pipe(pipefd);

pid_t pid = fork();
if (pid == 0) {
    // Child: redirect stdin from pipe read end
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    execlp("cat", "cat", nullptr);
    _exit(1);
}
// Parent: write to child's stdin
close(pipefd[0]);
const char* msg = "hello from parent\n";
write(pipefd[1], msg, strlen(msg));
close(pipefd[1]);   // signal EOF
waitpid(pid, nullptr, 0);
```

---

## RAII Wrappers (C++ Style)

### RAII Pipe

```cpp
class PipePair {
public:
    PipePair() {
        if (pipe(fds_) == -1)
            throw std::runtime_error(std::string("pipe: ") + strerror(errno));
    }
    ~PipePair() {
        if (fds_[0] >= 0) close(fds_[0]);
        if (fds_[1] >= 0) close(fds_[1]);
    }
    PipePair(const PipePair&) = delete;
    PipePair& operator=(const PipePair&) = delete;

    int read_fd()  const { return fds_[0]; }
    int write_fd() const { return fds_[1]; }

    void close_read()  { close(fds_[0]); fds_[0] = -1; }
    void close_write() { close(fds_[1]); fds_[1] = -1; }
    int  release_read()  { int fd = fds_[0]; fds_[0] = -1; return fd; }
    int  release_write() { int fd = fds_[1]; fds_[1] = -1; return fd; }
private:
    int fds_[2];
};
```

### RAII File Descriptor

```cpp
class FileDescriptor {
    int fd_;
public:
    explicit FileDescriptor(int fd) : fd_(fd) {
        if (fd_ < 0) throw std::runtime_error("invalid fd");
    }
    ~FileDescriptor() { if (fd_ >= 0) close(fd_); }
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }

    int get() const { return fd_; }
    int release() { int fd = fd_; fd_ = -1; return fd; }
};
```

### Helper: `fork_exec` with Pipe

```cpp
// Run command, feed stdin from string, capture stdout
std::string run_command(const std::vector<std::string>& cmd,
                        const std::string& input = "") {
    PipePair in_pipe, out_pipe;

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        dup2(in_pipe.read_fd(), STDIN_FILENO);
        dup2(out_pipe.write_fd(), STDOUT_FILENO);
        in_pipe.close_read(); in_pipe.close_write();
        out_pipe.close_read(); out_pipe.close_write();

        std::vector<char*> argv;
        std::vector<std::string> args = cmd;
        for (auto& s : args) argv.push_back(s.data());
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    // Parent
    in_pipe.close_read();
    out_pipe.close_write();

    // Write input
    if (!input.empty())
        write(in_pipe.write_fd(), input.data(), input.size());
    in_pipe.close_write();

    // Read output
    std::string output;
    char buf[4096];
    ssize_t n;
    while ((n = read(out_pipe.read_fd(), buf, sizeof(buf))) > 0)
        output.append(buf, n);

    waitpid(pid, nullptr, 0);
    return output;
}
```

---

## Signal Handling

```cpp
#include <signal.h>

// Simple handler
void sig_handler(int sig) {
    // only async-signal-safe functions allowed here
    write(STDERR_FILENO, "caught\n", 7);
}

struct sigaction sa{};
sa.sa_handler = sig_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;   // restart interrupted syscalls
sigaction(SIGTERM, &sa, nullptr);

// Ignore a signal
signal(SIGPIPE, SIG_IGN);   // ignore broken pipe (important for piped writes)

// Block signals
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGTERM);
sigprocmask(SIG_BLOCK, &set, nullptr);

// Self-pipe trick for signal-safe notification to event loop
// Write 1 byte to a pipe in handler; select/poll/epoll on read end
```

---

## Process IDs and Groups

```cpp
getpid()     // current process PID
getppid()    // parent PID
getpgid(0)   // current process group ID
setpgid(0,0) // new process group (child becomes group leader)
setsid()     // create new session (daemon step 1)
```

---

## Pitfalls

```cpp
// 1. Not waiting for child → zombie
pid_t pid = fork();
if (pid == 0) { do_work(); _exit(0); }
// Parent never calls waitpid → child becomes zombie until parent exits

// 2. exit() in child flushes stdio buffers → double output
pid_t p = fork();
if (p == 0) { exit(0); }   // WRONG: flushes parent's unflushed stdio
// Use _exit(0) in child

// 3. Forgetting to close unused pipe ends
// If parent holds write end open, child's read() never gets EOF → hangs

// 4. exec replaces address space; C++ destructors are NOT called
// Ensure any necessary cleanup happens before exec, or use fork+exec

// 5. Inheriting file descriptors across exec — use O_CLOEXEC
int fd = open("file", O_RDONLY | O_CLOEXEC);  // auto-closed on exec

// 6. Race condition: child calls exec, parent hasn't set up signal handlers yet
// Use signalfd or self-pipe trick; or block signals around fork/exec

// 7. fork in multi-threaded program — only async-signal-safe code in child
// Other threads are not cloned; mutexes may be locked → deadlock in child
// Rule: fork() then exec() immediately in child; don't use malloc/new
```

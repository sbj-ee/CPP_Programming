# I/O Multiplexing (POSIX (Portable Operating System Interface)) — Cheat Sheet

> POSIX I/O multiplexing APIs (`select`, `poll`, `epoll`) are identical in C++. C++ adds RAII `EpollFd` and `PipePair` classes for safe resource management.

## Headers

```cpp
#include <sys/epoll.h>   // epoll_create1, epoll_ctl, epoll_wait
#include <sys/select.h>  // select, fd_set, FD_SET, FD_ZERO, FD_ISSET
#include <poll.h>        // poll, struct pollfd
#include <unistd.h>      // pipe, read, write, close
#include <fcntl.h>       // fcntl, F_SETFL, O_NONBLOCK
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <string>
```

---

## Comparison: select vs poll vs epoll

| Feature | `select` | `poll` | `epoll` |
|---------|---------|--------|---------|
| Max FDs | FD_SETSIZE (1024) | Unlimited | Unlimited |
| Performance | O(max_fd) | O(n_fds) | O(ready) |
| API | Bitmask | Array of pollfd | Event-driven |
| Edge/level trigger | Level only | Level only | Both |
| Linux only | No | No | Yes |
| Best for | Portability | < few hundred fds | High-scale servers |

---

## `select`

```cpp
int server_fd;   // assume already listening

fd_set read_fds, write_fds;
FD_ZERO(&read_fds);
FD_ZERO(&write_fds);
FD_SET(server_fd, &read_fds);
FD_SET(other_fd,  &read_fds);

int max_fd = std::max(server_fd, other_fd);

struct timeval timeout{5, 0};   // 5 seconds (or nullptr to block forever)

int n = select(max_fd + 1, &read_fds, &write_fds, nullptr, &timeout);
if (n == -1) {
    if (errno == EINTR) { /* interrupted by signal, retry */ }
    else throw std::runtime_error(std::string("select: ") + strerror(errno));
}
if (n == 0) { /* timeout */ }

if (FD_ISSET(server_fd, &read_fds)) {
    int client = accept(server_fd, nullptr, nullptr);
}
if (FD_ISSET(other_fd, &read_fds)) {
    char buf[256];
    ssize_t bytes = read(other_fd, buf, sizeof(buf));
}

// Note: read_fds is modified by select — rebuild before each call
```

---

## `poll`

```cpp
std::vector<pollfd> fds = {
    {server_fd,  POLLIN,  0},
    {stdin_fd,   POLLIN,  0},
    {client_fd,  POLLOUT, 0},
};

int timeout_ms = 5000;   // milliseconds; -1 = block forever
int n = poll(fds.data(), fds.size(), timeout_ms);
if (n == -1) {
    if (errno == EINTR) { /* retry */ }
    else throw std::runtime_error(std::string("poll: ") + strerror(errno));
}
if (n == 0) { /* timeout */ }

for (auto& pfd : fds) {
    if (pfd.revents & POLLIN)    { /* ready to read */ }
    if (pfd.revents & POLLOUT)   { /* ready to write */ }
    if (pfd.revents & POLLERR)   { /* error */ }
    if (pfd.revents & POLLHUP)   { /* hang up (peer closed) */ }
    if (pfd.revents & POLLNVAL)  { /* invalid fd */ }
    pfd.revents = 0;             // clear for next iteration
}

// pollfd events bits (same constants for both events and revents)
// POLLIN   — data to read
// POLLOUT  — space to write
// POLLERR  — error condition (revents only)
// POLLHUP  — hang up (revents only)
// POLLNVAL — invalid request: fd not open (revents only)
// POLLPRI  — urgent data
```

---

## `epoll` (Linux)

### Setup

```cpp
// Create epoll instance (EPOLL_CLOEXEC: close-on-exec)
int epfd = epoll_create1(EPOLL_CLOEXEC);
if (epfd == -1) throw std::runtime_error("epoll_create1");

// Register interest in fd
epoll_event ev{};
ev.events  = EPOLLIN;     // or EPOLLIN | EPOLLOUT | EPOLLET
ev.data.fd = server_fd;   // or ev.data.ptr = &my_struct
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

// Modify interest
ev.events = EPOLLIN | EPOLLOUT;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

// Deregister
epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
```

### Event Loop

```cpp
constexpr int MAX_EVENTS = 64;
epoll_event events[MAX_EVENTS];

while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, /*timeout_ms=*/-1);
    if (n == -1) {
        if (errno == EINTR) continue;
        throw std::runtime_error(std::string("epoll_wait: ") + strerror(errno));
    }

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        uint32_t ev = events[i].events;

        if (fd == server_fd && (ev & EPOLLIN)) {
            int client = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (client >= 0) {
                epoll_event cev{}; cev.events = EPOLLIN | EPOLLET; cev.data.fd = client;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client, &cev);
            }
        } else if (ev & EPOLLIN) {
            char buf[4096];
            ssize_t bytes = read(fd, buf, sizeof(buf));
            if (bytes <= 0) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
            } else {
                // process buf[0..bytes-1]
            }
        }
        if (ev & (EPOLLERR | EPOLLHUP)) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
        }
    }
}
close(epfd);
```

### Epoll Trigger Modes

| Mode | Meaning |
|------|---------|
| `EPOLLET` (edge) | Notified only when state changes (ready → ready is NOT another event); must drain fd completely |
| Default (level) | Notified as long as fd is ready; simpler but more events |

```cpp
// Edge-triggered: must drain until EAGAIN
ev.events = EPOLLIN | EPOLLET;
// ...
while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;  // drained
    if (n <= 0) { close(fd); break; }
    // process n bytes
}
```

---

## `epoll_event.data` Union

```cpp
union epoll_data {
    void*    ptr;   // pointer to connection struct
    int      fd;
    uint32_t u32;
    uint64_t u64;
};

// Use ptr for connection objects:
struct Connection { int fd; char buf[4096]; };
Connection* conn = new Connection{client_fd, {}};
ev.data.ptr = conn;
// On event: Connection* c = static_cast<Connection*>(events[i].data.ptr);
```

---

## RAII (Resource Acquisition Is Initialisation) Wrappers (C++ Style)

### RAII `EpollFd`

```cpp
class EpollFd {
    int epfd_;
public:
    EpollFd() : epfd_(epoll_create1(EPOLL_CLOEXEC)) {
        if (epfd_ == -1) throw std::runtime_error(std::string("epoll_create1: ") + strerror(errno));
    }
    ~EpollFd() { if (epfd_ >= 0) close(epfd_); }
    EpollFd(const EpollFd&) = delete;
    EpollFd& operator=(const EpollFd&) = delete;
    EpollFd(EpollFd&& o) noexcept : epfd_(o.epfd_) { o.epfd_ = -1; }

    int get() const { return epfd_; }

    void add(int fd, uint32_t events, void* data = nullptr) {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = data ? data : reinterpret_cast<void*>(static_cast<intptr_t>(fd));
        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
            throw std::runtime_error(std::string("epoll_ctl ADD: ") + strerror(errno));
    }
    void add_fd(int fd, uint32_t events) {
        epoll_event ev{}; ev.events = events; ev.data.fd = fd;
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    }
    void mod(int fd, uint32_t events, void* data = nullptr) {
        epoll_event ev{}; ev.events = events; ev.data.ptr = data;
        epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
    }
    void del(int fd) { epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr); }

    int wait(epoll_event* events, int max, int timeout_ms = -1) {
        int n;
        do { n = epoll_wait(epfd_, events, max, timeout_ms); }
        while (n == -1 && errno == EINTR);
        if (n == -1) throw std::runtime_error(std::string("epoll_wait: ") + strerror(errno));
        return n;
    }
};
```

### RAII `PipePair`

```cpp
class PipePair {
    int fds_[2] = {-1, -1};
public:
    PipePair() {
        if (pipe2(fds_, O_CLOEXEC) == -1)  // pipe2 is Linux; portable: pipe(fds_) + fcntl
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
    int release_read()  { int fd=fds_[0]; fds_[0]=-1; return fd; }
    int release_write() { int fd=fds_[1]; fds_[1]=-1; return fd; }
};
```

### Self-Pipe Trick (Signal-Safe Wakeup)

```cpp
// Use a pipe to wake epoll from a signal handler
PipePair wake_pipe;
efd.add_fd(wake_pipe.read_fd(), EPOLLIN);

// Signal handler (async-signal-safe):
void sig_handler(int) {
    char c = 1;
    write(wake_pipe_write_fd, &c, 1);  // wake epoll
}

// In event loop, drain the pipe when read_fd fires:
if (fd == wake_pipe.read_fd()) {
    char buf[64]; read(fd, buf, sizeof(buf));  // drain
    // handle shutdown etc.
}
```

---

## Non-Blocking I/O Setup

```cpp
// Set O_NONBLOCK on any fd before using with epoll/poll
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl F_SETFL");
}

// accept4 — create non-blocking client socket atomically (Linux)
int client = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
```

---

## Pitfalls

```cpp
// 1. Not draining fd on edge-triggered epoll
// ET: only notified when fd becomes readable; if you read 100 bytes
// and 200 are available, you won't get another event for the remaining 100.
// Always read in a loop until EAGAIN with EPOLLET.

// 2. select fd_set rebuilt each call — read_fds is modified by select
fd_set master_set, read_set;
FD_ZERO(&master_set); FD_SET(fd, &master_set);
while (true) {
    read_set = master_set;   // copy before each select call
    select(max_fd+1, &read_set, nullptr, nullptr, nullptr);
}

// 3. select FD_SETSIZE limit (1024 on most systems)
// Use poll or epoll for more than ~1000 connections

// 4. epoll_ctl ADD on already-registered fd → EEXIST error
// epoll_ctl DEL on unregistered fd → ENOENT error
// Track registered fds in a set/map to avoid double-add

// 5. Closing fd without removing from epoll
// (Linux auto-removes from epoll when last reference is closed,
//  but if fd was dup'd, it stays registered — always epoll_ctl DEL first)
efd.del(fd);
close(fd);

// 6. EPOLLHUP vs EPOLLRDHUP
// EPOLLHUP: error or both halves of connection closed
// EPOLLRDHUP (Linux 2.6.17+): peer closed write end (half-close)
ev.events = EPOLLIN | EPOLLRDHUP;
if (ev & EPOLLRDHUP) { /* peer done sending; may still send to them */ }

// 7. Spurious EPOLLOUT — adding EPOLLOUT when already writable fires immediately
// Only register EPOLLOUT when you have data to write; remove when write buffer empty

// 8. poll/select timeout struct is modified on Linux (shows remaining time)
// Reconstruct timeval/timespec before each call for reliable timeout
```

// =============================================================================
// Exercise 28: I/O Multiplexing in C++ — select, poll, epoll
// =============================================================================
// Topics: EpollFd / PipePair RAII wrappers, select() two-pipe demo,
//         poll() same demo, epoll() same demo, O_NONBLOCK + EAGAIN,
//         multi-client echo server (epoll + fork)
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g io_mux.cpp -o io_mux
// Linux-only: epoll is not POSIX.
// =============================================================================

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <vector>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// =============================================================================
// RAII Wrappers
// =============================================================================

class EpollFd {
public:
    EpollFd() : fd_(::epoll_create1(0)) {
        if (fd_ < 0)
            std::cerr << "epoll_create1: " << std::strerror(errno) << "\n";
    }
    ~EpollFd() { if (fd_ >= 0) ::close(fd_); }
    EpollFd(const EpollFd&)            = delete;
    EpollFd& operator=(const EpollFd&) = delete;

    bool   valid() const { return fd_ >= 0; }
    int    fd()    const { return fd_;      }

    bool add(int targetfd, uint32_t events) {
        epoll_event ev = {};
        ev.events  = events;
        ev.data.fd = targetfd;
        return ::epoll_ctl(fd_, EPOLL_CTL_ADD, targetfd, &ev) == 0;
    }
    bool del(int targetfd) {
        return ::epoll_ctl(fd_, EPOLL_CTL_DEL, targetfd, nullptr) == 0;
    }
    int wait(epoll_event* buf, int maxev, int timeout_ms = -1) {
        return ::epoll_wait(fd_, buf, maxev, timeout_ms);
    }

private:
    int fd_;
};

class PipePair {
public:
    PipePair() {
        int fds[2];
        if (::pipe2(fds, 0) == -1) {
            std::cerr << "pipe2: " << std::strerror(errno) << "\n";
            rfd_ = wfd_ = -1;
        } else {
            rfd_ = fds[0]; wfd_ = fds[1];
        }
    }
    ~PipePair() { close_read(); close_write(); }
    PipePair(const PipePair&)            = delete;
    PipePair& operator=(const PipePair&) = delete;

    bool ok()      const { return rfd_ >= 0 && wfd_ >= 0; }
    int  rfd()     const { return rfd_; }
    int  wfd()     const { return wfd_; }

    void close_read()  { if (rfd_ >= 0) { ::close(rfd_); rfd_ = -1; } }
    void close_write() { if (wfd_ >= 0) { ::close(wfd_); wfd_ = -1; } }

    void set_nonblock_read() {
        int fl = ::fcntl(rfd_, F_GETFL);
        ::fcntl(rfd_, F_SETFL, fl | O_NONBLOCK);
    }

private:
    int rfd_, wfd_;
};

// Helper: write a fixed string to a pipe fd
static void pipe_write(int fd, const char* s) {
    size_t len = std::strlen(s);
    while (len > 0) {
        ssize_t n = ::write(fd, s, len);
        if (n <= 0) break;
        s   += n;
        len -= static_cast<size_t>(n);
    }
}

// =============================================================================
// SECTION 1: Concepts — select / poll / epoll comparison table
// =============================================================================

static void section1_concepts() {
    std::cout << "\n=== Section 1: I/O multiplexing concepts ===\n";

    std::cout << "  Problem: a single thread needs to wait on multiple fds\n";
    std::cout << "  (pipes, sockets) without blocking on any one of them.\n";
    std::cout << "  Solution: ask the kernel which fds are ready, then act.\n";

    std::cout << "\n  Comparison:\n";
    std::cout << "  select()  — POSIX; fd_set bitmask; limited to FD_SETSIZE (~1024 fds).\n";
    std::cout << "              O(n) scan on every call; not scalable.\n";
    std::cout << "  poll()    — POSIX; pollfd array; no hard fd limit.\n";
    std::cout << "              Still O(n) scan; better than select for fd count.\n";
    std::cout << "  epoll()   — Linux only; event-driven; O(1) per ready fd.\n";
    std::cout << "              Scales to hundreds of thousands of fds.\n";
    std::cout << "              Modes: EPOLLET (edge-triggered), EPOLLIN, EPOLLOUT.\n";

    std::cout << "\n  Level-triggered (default): fd stays reported until fully drained.\n";
    std::cout << "  Edge-triggered (EPOLLET):  reported only once per state change.\n";
    std::cout << "  ET requires O_NONBLOCK + read loop until EAGAIN.\n";
}

// =============================================================================
// SECTION 2: select() — two-pipe two-writer demo
// =============================================================================

static void section2_select() {
    std::cout << "\n=== Section 2: select() — two-pipe demo ===\n";

    PipePair p1, p2;
    if (!p1.ok() || !p2.ok()) return;

    // Write to both pipes
    pipe_write(p1.wfd(), "msg-from-pipe1");
    pipe_write(p2.wfd(), "msg-from-pipe2");

    int maxfd = std::max(p1.rfd(), p2.rfd()) + 1;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(p1.rfd(), &rfds);
    FD_SET(p2.rfd(), &rfds);

    // Non-blocking timeout
    timeval tv = {0, 500'000};  // 0.5 s

    int n = ::select(maxfd, &rfds, nullptr, nullptr, &tv);
    std::cout << "  select() returned: " << n << " fd(s) ready\n";

    if (n > 0) {
        char buf[64];
        if (FD_ISSET(p1.rfd(), &rfds)) {
            ssize_t r = ::read(p1.rfd(), buf, sizeof(buf) - 1);
            if (r > 0) { buf[r] = '\0'; std::cout << "  pipe1 ready: " << buf << "\n"; }
        }
        if (FD_ISSET(p2.rfd(), &rfds)) {
            ssize_t r = ::read(p2.rfd(), buf, sizeof(buf) - 1);
            if (r > 0) { buf[r] = '\0'; std::cout << "  pipe2 ready: " << buf << "\n"; }
        }
    }
}

// =============================================================================
// SECTION 3: poll() — same demo
// =============================================================================

static void section3_poll() {
    std::cout << "\n=== Section 3: poll() — two-pipe demo ===\n";

    PipePair p1, p2;
    if (!p1.ok() || !p2.ok()) return;

    pipe_write(p1.wfd(), "poll-msg-1");
    pipe_write(p2.wfd(), "poll-msg-2");

    pollfd pfds[2] = {};
    pfds[0].fd     = p1.rfd();
    pfds[0].events = POLLIN;
    pfds[1].fd     = p2.rfd();
    pfds[1].events = POLLIN;

    int n = ::poll(pfds, 2, 500);  // 500 ms timeout
    std::cout << "  poll() returned: " << n << " fd(s) ready\n";

    char buf[64];
    for (int i = 0; i < 2; ++i) {
        if (pfds[i].revents & POLLIN) {
            ssize_t r = ::read(pfds[i].fd, buf, sizeof(buf) - 1);
            if (r > 0) {
                buf[r] = '\0';
                std::cout << "  pipe" << (i + 1) << " ready: " << buf << "\n";
            }
        }
    }
}

// =============================================================================
// SECTION 4: epoll — same demo
// =============================================================================

static void section4_epoll() {
    std::cout << "\n=== Section 4: epoll() — two-pipe demo ===\n";

    PipePair p1, p2;
    if (!p1.ok() || !p2.ok()) return;

    pipe_write(p1.wfd(), "epoll-msg-1");
    pipe_write(p2.wfd(), "epoll-msg-2");

    EpollFd ep;
    if (!ep.valid()) return;

    ep.add(p1.rfd(), EPOLLIN);
    ep.add(p2.rfd(), EPOLLIN);

    epoll_event evs[4];
    int n = ep.wait(evs, 4, 500);
    std::cout << "  epoll_wait() returned: " << n << " fd(s) ready\n";

    char buf[64];
    for (int i = 0; i < n; ++i) {
        if (evs[i].events & EPOLLIN) {
            int fd = evs[i].data.fd;
            ssize_t r = ::read(fd, buf, sizeof(buf) - 1);
            if (r > 0) {
                buf[r] = '\0';
                std::cout << "  fd=" << fd << " ready: " << buf << "\n";
            }
        }
    }
}

// =============================================================================
// SECTION 5: Non-blocking I/O — O_NONBLOCK + EAGAIN
// =============================================================================
//
// A non-blocking fd returns EAGAIN (or EWOULDBLOCK) immediately if no data
// is available, instead of blocking.  Required for edge-triggered epoll.

static void section5_nonblocking() {
    std::cout << "\n=== Section 5: O_NONBLOCK + EAGAIN ===\n";

    PipePair p;
    if (!p.ok()) return;

    // Set read end non-blocking
    int flags = ::fcntl(p.rfd(), F_GETFL);
    ::fcntl(p.rfd(), F_SETFL, flags | O_NONBLOCK);

    // Read from empty pipe — must get EAGAIN
    char buf[16];
    ssize_t r = ::read(p.rfd(), buf, sizeof(buf));
    if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        std::cout << "  Empty pipe read returned EAGAIN (correct for O_NONBLOCK)\n";
    } else {
        std::cout << "  Unexpected result: r=" << r << " errno=" << errno << "\n";
    }

    // Write then read
    pipe_write(p.wfd(), "data");
    r = ::read(p.rfd(), buf, sizeof(buf) - 1);
    if (r > 0) {
        buf[r] = '\0';
        std::cout << "  After write, read got: " << buf << "\n";
    }

    // After draining, EAGAIN again
    r = ::read(p.rfd(), buf, sizeof(buf));
    if (r == -1 && errno == EAGAIN)
        std::cout << "  After drain, EAGAIN again (correct)\n";

    std::cout << "\n  Edge-triggered epoll pattern:\n";
    std::cout << "    while (true) {\n";
    std::cout << "        ssize_t n = read(fd, buf, sizeof(buf));\n";
    std::cout << "        if (n < 0 && errno == EAGAIN) break;  // fully drained\n";
    std::cout << "        if (n <= 0) { /* error or EOF */ break; }\n";
    std::cout << "        process(buf, n);\n";
    std::cout << "    }\n";
}

// =============================================================================
// SECTION 6: Multi-client echo server (epoll, 3 clients, fork)
// =============================================================================
//
// Parent: epoll-based echo server accepting and serving multiple clients.
// Child:  sends 3 messages from separate forked clients.

static void section6_echo_server() {
    std::cout << "\n=== Section 6: Multi-client echo server (epoll) ===\n";

    // Create listening TCP socket
    int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { std::cerr << "socket: " << std::strerror(errno) << "\n"; return; }

    int yes = 1;
    ::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons(0);
    ::bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(listenfd, 8);

    socklen_t alen = sizeof(addr);
    ::getsockname(listenfd, reinterpret_cast<sockaddr*>(&addr), &alen);
    int port = ntohs(addr.sin_port);
    std::cout << "  Echo server on loopback port " << port << "\n";

    const int NUM_CLIENTS = 3;

    // Spawn NUM_CLIENTS child processes (clients)
    std::vector<pid_t> pids;
    for (int ci = 0; ci < NUM_CLIENTS; ++ci) {
        pid_t p = ::fork();
        if (p == 0) {
            ::close(listenfd);
            int cfd = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in srv = {};
            srv.sin_family      = AF_INET;
            srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            srv.sin_port        = htons(static_cast<uint16_t>(port));
            ::connect(cfd, reinterpret_cast<sockaddr*>(&srv), sizeof(srv));
            char msg[32];
            std::snprintf(msg, sizeof(msg), "hello-from-client-%d", ci);
            ::send(cfd, msg, std::strlen(msg), 0);
            char buf[64] = {};
            ::recv(cfd, buf, sizeof(buf) - 1, 0);
            std::cout.flush();
            ::close(cfd);
            ::_exit(0);
        }
        pids.push_back(p);
    }

    // Server: epoll loop serving NUM_CLIENTS connections
    EpollFd ep;
    if (!ep.valid()) { ::close(listenfd); return; }
    ep.add(listenfd, EPOLLIN);

    int served = 0;
    while (served < NUM_CLIENTS) {
        epoll_event evs[8];
        int n = ep.wait(evs, 8, 2000);
        if (n <= 0) break;
        for (int i = 0; i < n; ++i) {
            int fd = evs[i].data.fd;
            if (fd == listenfd) {
                sockaddr_in ca = {};
                socklen_t   cl = sizeof(ca);
                int conn = ::accept(listenfd, reinterpret_cast<sockaddr*>(&ca), &cl);
                if (conn >= 0) ep.add(conn, EPOLLIN);
            } else {
                char buf[64] = {};
                ssize_t r = ::recv(fd, buf, sizeof(buf) - 1, 0);
                if (r > 0) {
                    buf[r] = '\0';
                    std::cout << "  [server] echoing: " << buf << "\n";
                    ::send(fd, buf, static_cast<size_t>(r), MSG_NOSIGNAL);
                    ++served;
                }
                ep.del(fd);
                ::close(fd);
            }
        }
    }
    ::close(listenfd);

    for (pid_t p : pids) ::waitpid(p, nullptr, 0);
    std::cout << "  Echo server served " << served << " clients.\n";

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "  1. select() modifies the fd_set on return — reset before each call.\n";
    std::cout << "  2. poll() is portable; epoll() is Linux-only but scales to millions.\n";
    std::cout << "  3. Edge-triggered epoll: must read until EAGAIN or events are lost.\n";
    std::cout << "  4. epoll_wait() can return EINTR on signal — retry loop needed.\n";
    std::cout << "  5. Always set sockets O_NONBLOCK with edge-triggered epoll.\n";
    std::cout << "  6. Prefer epoll for servers with many connections (N>100);\n";
    std::cout << "     poll() is fine for small counts.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 28: I/O Multiplexing — select / poll / epoll ===\n";

    section1_concepts();
    section2_select();
    section3_poll();
    section4_epoll();
    section5_nonblocking();
    section6_echo_server();

    std::cout << "\nDone.\n";
    return 0;
}

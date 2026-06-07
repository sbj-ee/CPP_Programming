# Sockets (POSIX (Portable Operating System Interface)) — Cheat Sheet

> POSIX socket API is identical in C++. C++ adds RAII `Socket` classes, `std::string` for messages, and `send_all`/`recv_all` helpers.

## Headers

```cpp
#include <sys/socket.h>   // socket, bind, listen, accept, connect, send, recv
#include <netinet/in.h>   // sockaddr_in, sockaddr_in6
#include <arpa/inet.h>    // inet_pton, inet_ntop, htonl, htons
#include <netdb.h>        // getaddrinfo, freeaddrinfo
#include <unistd.h>       // close
#include <fcntl.h>        // fcntl, O_NONBLOCK
#include <cerrno>
#include <cstring>        // strerror
#include <string>
#include <stdexcept>
```

---

## Socket Creation

```cpp
// socket(domain, type, protocol)
int fd = socket(AF_INET,   SOCK_STREAM, 0);   // IPv4 TCP (Transmission Control Protocol)
int fd = socket(AF_INET6,  SOCK_STREAM, 0);   // IPv6 TCP
int fd = socket(AF_INET,   SOCK_DGRAM,  0);   // IPv4 UDP (User Datagram Protocol)
int fd = socket(AF_UNIX,   SOCK_STREAM, 0);   // Unix domain

if (fd == -1) throw std::runtime_error(std::string("socket: ") + strerror(errno));
```

---

## Server: TCP

```cpp
// 1. socket
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

// 2. setsockopt — reuse address (avoid "address already in use")
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 3. bind
sockaddr_in addr{};
addr.sin_family      = AF_INET;
addr.sin_port        = htons(8080);         // host-to-network byte order
addr.sin_addr.s_addr = INADDR_ANY;          // 0.0.0.0
bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

// 4. listen
listen(server_fd, /*backlog=*/10);

// 5. accept loop
while (true) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           reinterpret_cast<sockaddr*>(&client_addr),
                           &client_len);
    if (client_fd == -1) continue;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
    // handle client_fd — usually in a thread or via epoll
    close(client_fd);
}
close(server_fd);
```

---

## Client: TCP

```cpp
// 1. socket
int fd = socket(AF_INET, SOCK_STREAM, 0);

// 2. connect
sockaddr_in server{};
server.sin_family = AF_INET;
server.sin_port   = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

if (connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == -1) {
    close(fd);
    throw std::runtime_error(std::string("connect: ") + strerror(errno));
}

// 3. send / recv
send(fd, "hello", 5, 0);
char buf[1024];
ssize_t n = recv(fd, buf, sizeof(buf), 0);  // returns 0 on peer close
close(fd);
```

---

## `getaddrinfo` (Portable Host Resolution)

```cpp
addrinfo hints{}, *res;
hints.ai_family   = AF_UNSPEC;    // IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM;  // TCP

int rc = getaddrinfo("example.com", "80", &hints, &res);
if (rc != 0) throw std::runtime_error(gai_strerror(rc));

int fd = -1;
for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
    fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd == -1) continue;
    if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
    close(fd); fd = -1;
}
freeaddrinfo(res);
if (fd == -1) throw std::runtime_error("could not connect");
```

---

## UDP

```cpp
// Server
int fd = socket(AF_INET, SOCK_DGRAM, 0);
sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(9000); addr.sin_addr.s_addr=INADDR_ANY;
bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

char buf[1500];
sockaddr_in from{};
socklen_t fromlen = sizeof(from);
ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                     reinterpret_cast<sockaddr*>(&from), &fromlen);
sendto(fd, "reply", 5, 0,
       reinterpret_cast<sockaddr*>(&from), fromlen);
close(fd);

// Client
int fd = socket(AF_INET, SOCK_DGRAM, 0);
sockaddr_in srv{}; srv.sin_family=AF_INET; srv.sin_port=htons(9000);
inet_pton(AF_INET,"127.0.0.1",&srv.sin_addr);
sendto(fd,"hello",5,0,reinterpret_cast<sockaddr*>(&srv),sizeof(srv));
```

---

## Non-blocking and Options

```cpp
// Set non-blocking
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// recv/send return -1 with errno == EAGAIN or EWOULDBLOCK when would block

// Common socket options
setsockopt(fd, SOL_SOCKET,  SO_REUSEADDR, &one, sizeof(one));  // reuse addr
setsockopt(fd, SOL_SOCKET,  SO_KEEPALIVE, &one, sizeof(one));  // TCP keepalive
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,  &one, sizeof(one));  // disable Nagle

// Receive / send buffer sizes
int bufsize = 256*1024;
setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

// Timeout (SO_RCVTIMEO / SO_SNDTIMEO)
struct timeval tv{5, 0};   // 5 seconds
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

---

## RAII (Resource Acquisition Is Initialisation) Socket Class (C++ Style)

```cpp
class Socket {
    int fd_;
public:
    explicit Socket(int domain, int type, int proto = 0)
        : fd_(::socket(domain, type, proto)) {
        if (fd_ == -1) throw std::runtime_error(std::string("socket: ") + strerror(errno));
    }
    explicit Socket(int fd) : fd_(fd) {
        if (fd_ < 0) throw std::runtime_error("invalid fd");
    }
    ~Socket() { if (fd_ >= 0) ::close(fd_); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    Socket& operator=(Socket&& o) noexcept {
        if (this != &o) {
            if (fd_ >= 0) ::close(fd_);  // guard: don't close -1
            fd_ = o.fd_; o.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    int release() { int fd = fd_; fd_ = -1; return fd; }

    void set_reuseaddr() {
        int one = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
    void set_nonblocking() {
        int fl = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, fl | O_NONBLOCK);
    }

    Socket accept() {
        int client = ::accept(fd_, nullptr, nullptr);
        if (client == -1) throw std::runtime_error(std::string("accept: ") + strerror(errno));
        return Socket(client);
    }
};

// Usage
Socket srv(AF_INET, SOCK_STREAM);
srv.set_reuseaddr();
// bind, listen...
Socket client = srv.accept();
// client closed automatically when it goes out of scope
```

---

## `send_all` / `recv_all` Helpers

```cpp
// TCP may send/recv fewer bytes than requested
void send_all(int fd, const void* buf, size_t len) {
    const char* ptr = static_cast<const char*>(buf);
    while (len > 0) {
        ssize_t sent = send(fd, ptr, len, MSG_NOSIGNAL);  // MSG_NOSIGNAL: no SIGPIPE
        if (sent <= 0) throw std::runtime_error("send_all: connection closed or error");
        ptr += sent; len -= sent;
    }
}

void send_all(int fd, const std::string& s) {
    send_all(fd, s.data(), s.size());
}

std::string recv_exactly(int fd, size_t len) {
    std::string buf(len, '\0');
    char* ptr = buf.data();
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = recv(fd, ptr, remaining, 0);
        if (n <= 0) throw std::runtime_error("recv_exactly: connection closed or error");
        ptr += n; remaining -= n;
    }
    return buf;
}

// Read until newline (for line-based protocols)
std::string recv_line(int fd) {
    std::string line;
    char c;
    while (recv(fd, &c, 1, 0) == 1) {
        if (c == '\n') break;
        if (c != '\r') line += c;
    }
    return line;
}
```

---

## Unix Domain Sockets

```cpp
#include <sys/un.h>

constexpr const char* PATH = "/tmp/myapp.sock";

// Server
int fd = socket(AF_UNIX, SOCK_STREAM, 0);
sockaddr_un addr{};
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, PATH, sizeof(addr.sun_path)-1);
unlink(PATH);   // remove stale socket file
bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
listen(fd, 5);
// ...
unlink(PATH);   // cleanup on exit

// Client
int fd = socket(AF_UNIX, SOCK_STREAM, 0);
sockaddr_un addr{}; addr.sun_family=AF_UNIX;
strncpy(addr.sun_path, PATH, sizeof(addr.sun_path)-1);
connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
```

---

## Pitfalls

```cpp
// 1. Partial send/recv — TCP is a stream, not message-based
// send(fd, buf, 1000) may only send 300 bytes — always use send_all

// 2. SIGPIPE kills process when writing to closed peer
// Fix: setsockopt(SO_NOSIGPIPE) on macOS, MSG_NOSIGNAL on Linux, or signal(SIGPIPE,SIG_IGN)

// 3. Forgetting htons/htonl for network byte order
addr.sin_port = 8080;          // WRONG: byte order depends on host
addr.sin_port = htons(8080);   // RIGHT: network byte order (big-endian)

// 4. Not handling EINTR on blocking calls
ssize_t n;
do { n = recv(fd, buf, len, 0); } while (n == -1 && errno == EINTR);

// 5. bind before SO_REUSEADDR → "Address already in use" for 2 minutes (TIME_WAIT)

// 6. accept() returning -1 with EAGAIN in non-blocking mode is not an error
int client = accept(fd, nullptr, nullptr);
if (client == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;

// 7. recv returning 0 means peer closed connection
ssize_t n = recv(fd, buf, sizeof(buf), 0);
if (n == 0) { /* peer closed */ close(fd); }

// 8. IPv4-mapped IPv6: listening on "::" may or may not accept IPv4
// Explicitly set IPV6_V6ONLY to control behaviour
int v6only = 1;
setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
```

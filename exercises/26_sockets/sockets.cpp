// =============================================================================
// Exercise 26: POSIX Sockets in C++ with RAII
// =============================================================================
// Topics: Socket RAII class, TCP (Transmission Control Protocol) loopback with length-prefix framing,
//         UDP (User Datagram Protocol) sendto/recvfrom, AF_UNIX domain sockets, getaddrinfo loop,
//         SIGPIPE/MSG_NOSIGNAL, SO_REUSEADDR, partial send/recv
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g sockets.cpp -o sockets
// =============================================================================

#define _POSIX_C_SOURCE 200809L

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>         // sockaddr_un
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // htons, inet_ntop
#include <netdb.h>          // getaddrinfo, addrinfo
#include <csignal>          // signal, SIGPIPE, SIG_IGN
#include <sys/wait.h>       // waitpid
#include <fcntl.h>          // fcntl, O_NONBLOCK

// =============================================================================
// RAII Socket Wrapper
// =============================================================================
//
// Automatically closes the file descriptor when the Socket object goes out of
// scope.  This prevents resource leaks when exceptions are thrown.

class Socket {
public:
    // Construct from existing fd (takes ownership)
    explicit Socket(int fd = -1) noexcept : fd_(fd) {}

    // Create a new socket (wraps ::socket())
    static Socket make(int domain, int type, int protocol = 0) {
        int fd = ::socket(domain, type, protocol);
        if (fd == -1)
            std::cerr << "socket(): " << std::strerror(errno) << "\n";
        return Socket(fd);
    }

    ~Socket() { close(); }

    // Non-copyable, movable
    Socket(const Socket&)            = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    Socket& operator=(Socket&& o) noexcept {
        if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
        return *this;
    }

    int  fd()   const { return fd_; }
    bool valid() const { return fd_ >= 0; }

    void close() {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }

    // Release ownership without closing (e.g., hand to accept())
    int release() { int f = fd_; fd_ = -1; return f; }

private:
    int fd_;
};

// =============================================================================
// Framing helpers: send_all / recv_all + 4-byte length prefix
// =============================================================================
//
// TCP is a byte stream — a single send() may be split across multiple recv()
// calls.  We prefix every message with its 4-byte big-endian length.

static bool send_all(int fd, const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    while (len > 0) {
        // MSG_NOSIGNAL: don't send SIGPIPE if peer closed connection
        ssize_t n = ::send(fd, p, len, MSG_NOSIGNAL);
        if (n <= 0) return false;
        p   += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

static bool recv_all(int fd, void* buf, size_t len) {
    char* p = static_cast<char*>(buf);
    while (len > 0) {
        ssize_t n = ::recv(fd, p, len, 0);
        if (n <= 0) return false;
        p   += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

static bool send_msg(int fd, const std::string& s) {
    uint32_t hdr = htonl(static_cast<uint32_t>(s.size()));
    return send_all(fd, &hdr, 4) && send_all(fd, s.data(), s.size());
}

static bool recv_msg(int fd, std::string& out) {
    uint32_t hdr = 0;
    if (!recv_all(fd, &hdr, 4)) return false;
    size_t len = ntohl(hdr);
    out.resize(len);
    return recv_all(fd, out.data(), len);
}

// =============================================================================
// SECTION 1: Concepts — byte order, INADDR_LOOPBACK, socket API overview
// =============================================================================

static void section1_concepts() {
    std::cout << "\n=== Section 1: Socket concepts ===\n";

    std::cout << "  Byte order:\n";
    std::cout << "    Network byte order = big-endian (MSB (Most Significant Byte) first).\n";
    std::cout << "    htons(x) converts host->network short (16-bit).\n";
    std::cout << "    htonl(x) converts host->network long  (32-bit).\n";
    std::cout << "    ntohs/ntohl convert the reverse direction.\n";
    uint16_t port = 8080;
    uint16_t net  = htons(port);
    std::cout << "    htons(" << port << ") = 0x"
              << std::hex << net << std::dec
              << "  (on this machine)\n";

    std::cout << "\n  INADDR_LOOPBACK = 127.0.0.1 (loopback interface).\n";
    std::cout << "  INADDR_ANY      = 0.0.0.0   (bind to all interfaces).\n";

    std::cout << "\n  Key socket syscalls:\n";
    std::cout << "    socket()       — create endpoint\n";
    std::cout << "    bind()         — attach local address/port\n";
    std::cout << "    listen()       — mark TCP socket as passive\n";
    std::cout << "    accept()       — accept incoming TCP connection\n";
    std::cout << "    connect()      — initiate TCP connection\n";
    std::cout << "    send/recv()    — send/receive over connected socket\n";
    std::cout << "    sendto/recvfrom() — send/receive with explicit address (UDP)\n";
    std::cout << "    close()        — deallocate socket\n";
}

// =============================================================================
// SECTION 2: TCP loopback — port 0, getsockname, length-prefix framing
// =============================================================================

static void section2_tcp() {
    std::cout << "\n=== Section 2: TCP loopback with length-prefix framing ===\n";

    // Ignore SIGPIPE globally so broken-pipe doesn't kill us
    std::signal(SIGPIPE, SIG_IGN);

    // Create listening socket
    Socket listener = Socket::make(AF_INET, SOCK_STREAM);
    if (!listener.valid()) return;

    // SO_REUSEADDR: allow bind to TIME_WAIT port
    int yes = 1;
    setsockopt(listener.fd(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons(0);  // let OS pick a port

    if (bind(listener.fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::cerr << "bind: " << std::strerror(errno) << "\n";
        return;
    }
    listen(listener.fd(), 4);

    // Discover the assigned port via getsockname
    socklen_t addrlen = sizeof(addr);
    getsockname(listener.fd(), reinterpret_cast<sockaddr*>(&addr), &addrlen);
    int port = ntohs(addr.sin_port);
    std::cout << "  Server bound to loopback port " << port << "\n";

    // Fork: child is client, parent is server
    pid_t pid = ::fork();
    if (pid == 0) {
        // CLIENT
        listener.close();
        Socket client = Socket::make(AF_INET, SOCK_STREAM);
        sockaddr_in srv = {};
        srv.sin_family      = AF_INET;
        srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        srv.sin_port        = htons(static_cast<uint16_t>(port));
        if (connect(client.fd(), reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) == -1) {
            std::cerr << "[client] connect: " << std::strerror(errno) << "\n";
            ::_exit(1);
        }
        send_msg(client.fd(), "Hello, TCP server!");
        std::string reply;
        recv_msg(client.fd(), reply);
        std::cout << "  [client] received: " << reply << "\n";
        std::cout.flush();
        ::_exit(0);
    }

    // SERVER — accept one connection
    socklen_t clen = sizeof(addr);
    Socket conn(::accept(listener.fd(), reinterpret_cast<sockaddr*>(&addr), &clen));
    if (!conn.valid()) { std::cerr << "accept failed\n"; ::waitpid(pid, nullptr, 0); return; }

    std::string msg;
    recv_msg(conn.fd(), msg);
    std::cout << "  [server] received: " << msg << "\n";
    send_msg(conn.fd(), "Echo: " + msg);

    ::waitpid(pid, nullptr, 0);
    std::cout << "  TCP demo complete.\n";
}

// =============================================================================
// SECTION 3: UDP sendto / recvfrom
// =============================================================================

static void section3_udp() {
    std::cout << "\n=== Section 3: UDP sendto / recvfrom ===\n";

    // Server socket — bind to port 0
    Socket server = Socket::make(AF_INET, SOCK_DGRAM);
    if (!server.valid()) return;

    sockaddr_in saddr = {};
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    saddr.sin_port        = htons(0);
    bind(server.fd(), reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));

    socklen_t slen = sizeof(saddr);
    getsockname(server.fd(), reinterpret_cast<sockaddr*>(&saddr), &slen);
    int port = ntohs(saddr.sin_port);
    std::cout << "  UDP server on port " << port << "\n";

    pid_t pid = ::fork();
    if (pid == 0) {
        // CLIENT
        server.close();
        Socket client = Socket::make(AF_INET, SOCK_DGRAM);
        sockaddr_in dest = {};
        dest.sin_family      = AF_INET;
        dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        dest.sin_port        = htons(static_cast<uint16_t>(port));

        const std::string payload = "UDP datagram";
        ::sendto(client.fd(), payload.c_str(), payload.size(), 0,
                 reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        std::cout.flush();
        ::_exit(0);
    }

    // Server receives
    char buf[256] = {};
    sockaddr_in caddr = {};
    socklen_t   clen  = sizeof(caddr);
    ssize_t n = ::recvfrom(server.fd(), buf, sizeof(buf) - 1, 0,
                           reinterpret_cast<sockaddr*>(&caddr), &clen);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << "  [UDP server] received: " << buf << "\n";
    }
    ::waitpid(pid, nullptr, 0);
}

// =============================================================================
// SECTION 4: AF_UNIX domain socket
// =============================================================================

static const char* UNIX_SOCK_PATH = "/tmp/ex26_cpp.sock";

static void section4_unix() {
    std::cout << "\n=== Section 4: AF_UNIX domain socket ===\n";

    ::unlink(UNIX_SOCK_PATH);  // remove leftover from previous run

    Socket server = Socket::make(AF_UNIX, SOCK_STREAM);
    if (!server.valid()) return;

    sockaddr_un saddr = {};
    saddr.sun_family = AF_UNIX;
    std::strncpy(saddr.sun_path, UNIX_SOCK_PATH, sizeof(saddr.sun_path) - 1);

    if (bind(server.fd(), reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) == -1) {
        std::cerr << "AF_UNIX bind: " << std::strerror(errno) << "\n";
        return;
    }
    listen(server.fd(), 4);
    std::cout << "  AF_UNIX server at " << UNIX_SOCK_PATH << "\n";

    pid_t pid = ::fork();
    if (pid == 0) {
        server.close();
        Socket client = Socket::make(AF_UNIX, SOCK_STREAM);
        sockaddr_un caddr = {};
        caddr.sun_family = AF_UNIX;
        std::strncpy(caddr.sun_path, UNIX_SOCK_PATH, sizeof(caddr.sun_path) - 1);
        if (connect(client.fd(), reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr)) == -1) {
            std::cerr << "[unix client] connect: " << std::strerror(errno) << "\n";
            ::_exit(1);
        }
        const std::string msg = "Hello over UNIX socket";
        send_msg(client.fd(), msg);
        std::cout.flush();
        ::_exit(0);
    }

    socklen_t alen = sizeof(saddr);
    Socket conn(::accept(server.fd(), reinterpret_cast<sockaddr*>(&saddr), &alen));
    std::string msg;
    recv_msg(conn.fd(), msg);
    std::cout << "  [AF_UNIX server] received: " << msg << "\n";

    ::waitpid(pid, nullptr, 0);
    ::unlink(UNIX_SOCK_PATH);
}

// =============================================================================
// SECTION 5: getaddrinfo loop
// =============================================================================

static void section5_getaddrinfo() {
    std::cout << "\n=== Section 5: getaddrinfo loop ===\n";

    addrinfo hints = {};
    hints.ai_family   = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int rc = ::getaddrinfo("localhost", "80", &hints, &result);
    if (rc != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rc) << "\n";
        return;
    }

    std::cout << "  getaddrinfo(\"localhost\", \"80\") returned addresses:\n";
    int count = 0;
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        if (::getnameinfo(rp->ai_addr, rp->ai_addrlen,
                          host, sizeof(host), serv, sizeof(serv),
                          NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            std::cout << "    [" << count++ << "] family="
                      << (rp->ai_family == AF_INET ? "AF_INET" : "AF_INET6")
                      << "  addr=" << host << "  port=" << serv << "\n";
        }
    }
    ::freeaddrinfo(result);

    // Typical usage: iterate the list, try each until connect() succeeds
    std::cout << "  (In real code: try each entry with socket()+connect() until one works)\n";
}

// =============================================================================
// SECTION 6: Pitfalls
// =============================================================================

static void section6_pitfalls() {
    std::cout << "\n=== Section 6: Pitfalls ===\n";

    std::cout << "  SIGPIPE:\n";
    std::cout << "    Writing to a socket whose peer has closed sends SIGPIPE,\n";
    std::cout << "    which terminates the process by default.\n";
    std::cout << "    Fix: signal(SIGPIPE, SIG_IGN) + check send() return value,\n";
    std::cout << "         or use MSG_NOSIGNAL flag on send() / sendmsg().\n";

    std::cout << "\n  SO_REUSEADDR:\n";
    std::cout << "    After a server exits, the port stays in TIME_WAIT for ~60s.\n";
    std::cout << "    Without SO_REUSEADDR, bind() fails with EADDRINUSE.\n";
    std::cout << "    Fix: setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))\n";
    std::cout << "         before bind().\n";

    std::cout << "\n  PARTIAL SEND/RECV:\n";
    std::cout << "    send() and recv() may transfer fewer bytes than requested.\n";
    std::cout << "    Always loop until all bytes are transferred (send_all/recv_all).\n";
    std::cout << "    For UDP: datagrams are atomic — one sendto = one recvfrom.\n";

    std::cout << "\n  TCP is a byte STREAM — no message boundaries:\n";
    std::cout << "    Two consecutive sends may arrive as one recv or be split.\n";
    std::cout << "    Fix: length-prefix framing (4-byte header + payload) or\n";
    std::cout << "         delimiter-based framing (e.g., newline-terminated lines).\n";

    std::cout << "\n  Address reuse for AF_UNIX:\n";
    std::cout << "    The path must be unlink()ed before bind() or bind() returns\n";
    std::cout << "    EADDRINUSE even if no server is running.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 26: POSIX Sockets in C++ ===\n";

    section1_concepts();
    section2_tcp();
    section3_udp();
    section4_unix();
    section5_getaddrinfo();
    section6_pitfalls();

    std::cout << "\nDone.\n";
    return 0;
}

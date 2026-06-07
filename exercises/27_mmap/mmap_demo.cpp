// =============================================================================
// Exercise 27: POSIX mmap in C++ with RAII
// =============================================================================
// Topics: MmapRegion RAII class, file-backed read-only, MAP_SHARED write with
//         msync, anonymous private mapping, shared memory across fork,
//         page alignment, pitfalls table
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g mmap_demo.cpp -o mmap_demo
// =============================================================================

#define _POSIX_C_SOURCE 200809L

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdint>

#include <unistd.h>     // getpagesize, ftruncate, close, unlink, read
#include <sys/mman.h>   // mmap, munmap, msync, MAP_SHARED, MAP_PRIVATE, MAP_ANONYMOUS
#include <sys/stat.h>   // fstat, struct stat
#include <fcntl.h>      // open, O_RDONLY, O_RDWR, O_CREAT, O_TRUNC
#include <sys/types.h>
#include <sys/wait.h>   // waitpid

// =============================================================================
// RAII MmapRegion
// =============================================================================
//
// Wraps a mmap() region; calls munmap() in the destructor.
// Prevents resource leaks when exceptions propagate.

class MmapRegion {
public:
    // Map 'length' bytes from fd at 'offset' with given prot and flags.
    // Pass fd=-1 with MAP_ANONYMOUS for anonymous mappings.
    MmapRegion(size_t length, int prot, int flags, int fd = -1, off_t offset = 0)
        : ptr_(MAP_FAILED), length_(length)
    {
        void* p = ::mmap(nullptr, length, prot, flags, fd, offset);
        if (p == MAP_FAILED) {
            std::cerr << "mmap() failed: " << std::strerror(errno)
                      << "  (len=" << length << ")\n";
        } else {
            ptr_ = p;
        }
    }

    ~MmapRegion() {
        if (ptr_ != MAP_FAILED)
            ::munmap(ptr_, length_);
    }

    // Non-copyable, movable
    MmapRegion(const MmapRegion&)            = delete;
    MmapRegion& operator=(const MmapRegion&) = delete;

    MmapRegion(MmapRegion&& o) noexcept
        : ptr_(o.ptr_), length_(o.length_) {
        o.ptr_ = MAP_FAILED;
    }

    bool  valid()  const { return ptr_ != MAP_FAILED; }
    void* data()   const { return ptr_; }
    size_t size()  const { return length_; }

    template<typename T>
    T* as() const { return static_cast<T*>(ptr_); }

private:
    void*  ptr_;
    size_t length_;
};

// =============================================================================
// SECTION 1: Concepts — page size, MAP_FAILED, flag combinations
// =============================================================================

static void section1_concepts() {
    std::cout << "\n=== Section 1: mmap concepts ===\n";

    long pagesize = ::sysconf(_SC_PAGESIZE);
    std::cout << "  System page size: " << pagesize << " bytes\n";

    std::cout << "\n  mmap() maps a contiguous virtual-address range onto:\n";
    std::cout << "    - A file descriptor (file-backed)\n";
    std::cout << "    - Nothing (anonymous, with MAP_ANONYMOUS)\n";

    std::cout << "\n  Protection flags (prot):\n";
    std::cout << "    PROT_READ   — pages are readable\n";
    std::cout << "    PROT_WRITE  — pages are writable\n";
    std::cout << "    PROT_EXEC   — pages are executable\n";
    std::cout << "    PROT_NONE   — no access (guard pages)\n";

    std::cout << "\n  Visibility flags:\n";
    std::cout << "    MAP_SHARED  — writes visible to other mappers + written to file\n";
    std::cout << "    MAP_PRIVATE — copy-on-write; writes not visible elsewhere\n";
    std::cout << "    MAP_ANONYMOUS — not backed by a file; fd must be -1\n";

    std::cout << "\n  MAP_FAILED: returned by mmap() on error (not NULL!).\n";
    std::cout << "  Always compare with MAP_FAILED, not nullptr.\n";
}

// =============================================================================
// SECTION 2: File-backed read-only — fstat, pointer scan, memchr
// =============================================================================

static void section2_readonly() {
    std::cout << "\n=== Section 2: File-backed read-only mmap ===\n";

    // Create a small temp file to map
    const char* path = "/tmp/ex27_ro.txt";
    {
        int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { std::cerr << "open(write) failed\n"; return; }
        const char content[] = "Hello mmap world!\nLine two here.\n";
        ::write(fd, content, sizeof(content) - 1);
        ::close(fd);
    }

    int fd = ::open(path, O_RDONLY);
    if (fd < 0) { std::cerr << "open(read) failed\n"; return; }

    struct stat st = {};
    ::fstat(fd, &st);
    size_t filesize = static_cast<size_t>(st.st_size);
    std::cout << "  File: " << path << "  size=" << filesize << " bytes\n";

    MmapRegion region(filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);  // fd can be closed after mmap — mapping remains valid

    if (!region.valid()) return;

    const char* p   = region.as<char>();
    const char* end = p + filesize;

    // Count lines with memchr (async-signal-safe; works on raw memory)
    int lines = 0;
    const char* cur = p;
    while (cur < end) {
        const void* nl = std::memchr(cur, '\n', static_cast<size_t>(end - cur));
        if (!nl) break;
        ++lines;
        cur = static_cast<const char*>(nl) + 1;
    }
    std::cout << "  Line count via memchr scan: " << lines << "\n";
    std::cout << "  First 20 chars: [";
    for (size_t i = 0; i < 20 && i < filesize; ++i)
        std::cout << p[i];
    std::cout << "]\n";

    ::unlink(path);
}

// =============================================================================
// SECTION 3: File-backed MAP_SHARED write with struct Record and msync
// =============================================================================
//
// MAP_SHARED writes go back to the file.  msync(MS_SYNC) flushes dirty pages
// to disk synchronously before the mapping is released.

struct Record {
    uint32_t id;
    char     name[28];
    double   value;
};

static_assert(sizeof(Record) == 40, "Record size check");

static void section3_shared_write() {
    std::cout << "\n=== Section 3: MAP_SHARED write with struct Record ===\n";

    const char* path = "/tmp/ex27_rw.bin";
    const size_t sz  = sizeof(Record);

    // Create/truncate file to exact size
    int fd = ::open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { std::cerr << "open failed\n"; return; }
    ::ftruncate(fd, static_cast<off_t>(sz));

    MmapRegion region(sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close(fd);
    if (!region.valid()) { ::unlink(path); return; }

    Record* rec = region.as<Record>();
    rec->id    = 42;
    std::strncpy(rec->name, "mmap-record", sizeof(rec->name) - 1);
    rec->name[sizeof(rec->name) - 1] = '\0';
    rec->value = 3.14159;

    // msync flushes dirty pages to backing file synchronously
    if (::msync(region.data(), sz, MS_SYNC) == -1)
        std::cerr << "msync: " << std::strerror(errno) << "\n";

    std::cout << "  Wrote record: id=" << rec->id
              << "  name=" << rec->name
              << "  value=" << rec->value << "\n";

    // Re-read the file to verify
    {
        int rfd = ::open(path, O_RDONLY);
        MmapRegion verify(sz, PROT_READ, MAP_PRIVATE, rfd, 0);
        ::close(rfd);
        if (verify.valid()) {
            const Record* r2 = verify.as<Record>();
            std::cout << "  Re-read:  id=" << r2->id
                      << "  name=" << r2->name
                      << "  value=" << r2->value << "\n";
        }
    }
    ::unlink(path);
}

// =============================================================================
// SECTION 4: Anonymous private — 1M ints, zero guarantee
// =============================================================================
//
// MAP_ANONYMOUS | MAP_PRIVATE gives a fresh zeroed region not backed by any
// file.  The kernel pages it in on first access.  This is how malloc() obtains
// large chunks from the OS.

static void section4_anonymous() {
    std::cout << "\n=== Section 4: Anonymous private mapping (1M ints) ===\n";

    const size_t COUNT = 1024 * 1024;  // 1M ints = 4 MiB
    MmapRegion region(COUNT * sizeof(int), PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!region.valid()) return;

    int* arr = region.as<int>();

    // Verify zero-initialisation (POSIX guarantees it for anonymous mappings)
    bool all_zero = true;
    for (size_t i = 0; i < COUNT; ++i) {
        if (arr[i] != 0) { all_zero = false; break; }
    }
    std::cout << "  1M ints all zero after mmap: " << (all_zero ? "yes" : "no") << "\n";

    // Write a pattern and verify
    for (size_t i = 0; i < COUNT; ++i) arr[i] = static_cast<int>(i);
    std::cout << "  arr[0]=" << arr[0] << "  arr[500000]=" << arr[500000]
              << "  arr[1048575]=" << arr[1048575] << "\n";

    // MmapRegion destructor calls munmap() here — region released back to OS
    std::cout << "  (MmapRegion destructor will call munmap on scope exit)\n";
}

// =============================================================================
// SECTION 5: Shared memory across fork (MAP_ANONYMOUS | MAP_SHARED)
// =============================================================================
//
// MAP_SHARED + MAP_ANONYMOUS creates a region shared between parent and child
// after fork().  The child's writes are immediately visible to the parent.
// This is the simplest form of IPC for related processes.

static void section5_shared_fork() {
    std::cout << "\n=== Section 5: Shared memory across fork ===\n";

    const size_t SZ = sizeof(long);
    MmapRegion region(SZ, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!region.valid()) return;

    long* counter = region.as<long>();
    *counter = 0;

    pid_t pid = ::fork();
    if (pid == 0) {
        // Child: increment the shared counter
        for (long i = 0; i < 1000; ++i)
            ++(*counter);   // not atomic — demo only
        ::_exit(0);
    }

    ::waitpid(pid, nullptr, 0);
    std::cout << "  Shared counter after child incremented 1000 times: "
              << *counter << "\n";
    std::cout << "  (MAP_ANONYMOUS|MAP_SHARED makes child writes visible to parent)\n";
    std::cout << "  Note: for real shared-counter code, use an atomic or mutex.\n";
}

// =============================================================================
// SECTION 6: Page alignment arithmetic and pitfalls table
// =============================================================================

static void section6_alignment_pitfalls() {
    std::cout << "\n=== Section 6: Page alignment and pitfalls ===\n";

    long pagesize = ::sysconf(_SC_PAGESIZE);
    std::cout << "  Page size = " << pagesize << " bytes\n";

    // Round an offset UP to the next page boundary
    size_t offset = 5000;
    size_t aligned = (offset + static_cast<size_t>(pagesize) - 1)
                   & ~(static_cast<size_t>(pagesize) - 1);
    std::cout << "  Offset " << offset << " rounded UP to page boundary: " << aligned << "\n";

    // Round DOWN
    size_t aligned_down = offset & ~(static_cast<size_t>(pagesize) - 1);
    std::cout << "  Offset " << offset << " rounded DOWN:                 " << aligned_down << "\n";

    std::cout << "\n  Pitfalls:\n";
    std::cout << "    1. mmap offset must be a multiple of the page size.\n";
    std::cout << "       To map a range not starting on a page boundary:\n";
    std::cout << "       round the offset DOWN and add the delta to the pointer.\n";
    std::cout << "    2. MAP_FAILED != NULL — always compare with MAP_FAILED.\n";
    std::cout << "    3. The mapped length need not be page-aligned; the kernel\n";
    std::cout << "       rounds up, but you must not read beyond your intended size.\n";
    std::cout << "    4. msync() is needed before munmap() only if you need\n";
    std::cout << "       crash-safety; munmap() itself does NOT guarantee a sync.\n";
    std::cout << "    5. Closing the fd after mmap() is fine — the mapping holds\n";
    std::cout << "       a reference to the underlying inode.\n";
    std::cout << "    6. Writing beyond the file size via MAP_SHARED causes SIGBUS.\n";
    std::cout << "       Use ftruncate() to extend the file first.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 27: POSIX mmap in C++ ===\n";

    section1_concepts();
    section2_readonly();
    section3_shared_write();
    section4_anonymous();
    section5_shared_fork();
    section6_alignment_pitfalls();

    std::cout << "\nDone.\n";
    return 0;
}

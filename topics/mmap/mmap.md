# Memory-Mapped Files (POSIX) — Cheat Sheet

> POSIX `mmap` API is identical in C++. C++ adds a RAII `MmapRegion` class with automatic `munmap` in the destructor, and typed span access.

## Headers

```cpp
#include <sys/mman.h>    // mmap, munmap, mprotect, msync, MAP_*, PROT_*
#include <sys/stat.h>    // fstat, struct stat
#include <fcntl.h>       // open, O_RDONLY, O_RDWR, O_CREAT
#include <unistd.h>      // close, ftruncate, sysconf
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <span>          // C++20 — for typed view over mapped region
```

---

## `mmap()` Call

```cpp
void* addr = mmap(
    nullptr,        // hint address (NULL = let kernel choose)
    length,         // size in bytes
    prot,           // protection flags
    flags,          // mapping type flags
    fd,             // file descriptor (-1 for anonymous)
    offset          // offset into file (must be page-aligned)
);
if (addr == MAP_FAILED) {
    throw std::runtime_error(std::string("mmap: ") + strerror(errno));
}
```

### Protection Flags (`prot`)

| Flag | Meaning |
|------|---------|
| `PROT_READ` | Region can be read |
| `PROT_WRITE` | Region can be written |
| `PROT_EXEC` | Region can be executed |
| `PROT_NONE` | No access (guard page) |
| Combined | `PROT_READ | PROT_WRITE` |

### Mapping Flags (`flags`)

| Flag | Meaning |
|------|---------|
| `MAP_SHARED` | Writes visible to other processes; synced to file |
| `MAP_PRIVATE` | Copy-on-write; writes not visible to others |
| `MAP_ANONYMOUS` | Not backed by file; fd must be -1 |
| `MAP_FIXED` | Place at exact address (dangerous) |
| `MAP_POPULATE` | Pre-fault all pages (Linux) |
| `MAP_HUGETLB` | Use huge pages (Linux) |
| `MAP_NORESERVE` | Don't reserve swap space |

---

## Map a File for Reading

```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string_view>

std::string_view map_file_ro(const char* path, void*& mapping, size_t& len) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) throw std::runtime_error(std::string("open: ") + strerror(errno));

    struct stat sb;
    if (fstat(fd, &sb) == -1) { close(fd); throw std::runtime_error("fstat"); }
    len = static_cast<size_t>(sb.st_size);

    mapping = mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);   // fd can be closed immediately after mmap

    if (mapping == MAP_FAILED) throw std::runtime_error(std::string("mmap: ") + strerror(errno));
    return {static_cast<char*>(mapping), len};
}

// Usage
void* m; size_t sz;
auto view = map_file_ro("data.bin", m, sz);
// use view...
munmap(m, sz);
```

---

## Map a File for Read/Write

```cpp
int fd = open("data.bin", O_RDWR);
struct stat sb; fstat(fd, &sb);
size_t len = sb.st_size;

char* ptr = static_cast<char*>(
    mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
close(fd);

// Modify in-place — changes go to disk (MAP_SHARED)
ptr[0] = 'X';

// Flush to disk explicitly (optional; OS does it eventually)
msync(ptr, len, MS_SYNC);    // MS_SYNC = wait until complete
                             // MS_ASYNC = schedule (non-blocking)
munmap(ptr, len);
```

---

## Create and Map a New File

```cpp
const char* path = "output.bin";
size_t len = 4096;

int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
if (fd == -1) throw std::runtime_error("open");

// Size the file to desired length
if (ftruncate(fd, len) == -1) {
    close(fd); throw std::runtime_error("ftruncate");
}

char* ptr = static_cast<char*>(
    mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
close(fd);

// Write structured data directly
struct Header { uint32_t magic; uint32_t version; };
auto* h = reinterpret_cast<Header*>(ptr);
h->magic   = 0xDEADBEEF;
h->version = 1;

msync(ptr, len, MS_SYNC);
munmap(ptr, len);
```

---

## Anonymous Mapping (no file)

```cpp
// Allocate a large anonymous buffer (alternative to malloc for large allocs)
size_t len = 1024 * 1024 * 64;   // 64 MB
void* mem = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (mem == MAP_FAILED) throw std::runtime_error("mmap anon");

// Use as heap space
auto* arr = static_cast<int*>(mem);
arr[0] = 42;

munmap(mem, len);
```

---

## Other Operations

```cpp
// Change protection after mapping
mprotect(ptr, len, PROT_READ);              // make read-only
mprotect(ptr, page_size, PROT_NONE);       // guard page

// Advise kernel about access pattern
madvise(ptr, len, MADV_SEQUENTIAL);         // prefetch ahead
madvise(ptr, len, MADV_RANDOM);             // no prefetch
madvise(ptr, len, MADV_DONTNEED);          // discard pages
madvise(ptr, len, MADV_WILLNEED);          // prefetch now

// Get page size
long page_size = sysconf(_SC_PAGESIZE);    // typically 4096
// offset in mmap must be multiple of page_size

// Partial flush
msync(ptr + offset, partial_len, MS_ASYNC);

// Lock pages in memory (prevent swap)
mlock(ptr, len);
munlock(ptr, len);
```

---

## RAII `MmapRegion` Class (C++ Style)

```cpp
class MmapRegion {
    void*  addr_ = MAP_FAILED;
    size_t len_  = 0;
public:
    // Map existing file
    MmapRegion(const char* path, bool writable = false) {
        int flags_open = writable ? O_RDWR : O_RDONLY;
        int fd = open(path, flags_open);
        if (fd == -1) throw std::runtime_error(std::string("open: ") + strerror(errno));

        struct stat sb;
        if (fstat(fd, &sb) == -1) { close(fd); throw std::runtime_error("fstat"); }
        len_ = static_cast<size_t>(sb.st_size);

        int prot  = PROT_READ | (writable ? PROT_WRITE : 0);
        int mflags = writable ? MAP_SHARED : MAP_PRIVATE;
        addr_ = mmap(nullptr, len_, prot, mflags, fd, 0);
        close(fd);

        if (addr_ == MAP_FAILED)
            throw std::runtime_error(std::string("mmap: ") + strerror(errno));
    }

    // Anonymous mapping
    MmapRegion(size_t len, int prot = PROT_READ | PROT_WRITE) : len_(len) {
        addr_ = mmap(nullptr, len_, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr_ == MAP_FAILED) throw std::runtime_error("mmap anon");
    }

    ~MmapRegion() {
        if (addr_ != MAP_FAILED && len_ > 0) munmap(addr_, len_);
    }

    MmapRegion(const MmapRegion&) = delete;
    MmapRegion& operator=(const MmapRegion&) = delete;
    MmapRegion(MmapRegion&& o) noexcept : addr_(o.addr_), len_(o.len_) {
        o.addr_ = MAP_FAILED; o.len_ = 0;
    }

    void* data() const { return addr_; }
    size_t size() const { return len_; }

    template<typename T>
    T* as() const { return static_cast<T*>(addr_); }

    // C++20: typed span view
    template<typename T>
    std::span<T> span_as() const {
        return {static_cast<T*>(addr_), len_ / sizeof(T)};
    }

    void sync(bool wait = true) const {
        msync(addr_, len_, wait ? MS_SYNC : MS_ASYNC);
    }
    void advise_sequential() const { madvise(addr_, len_, MADV_SEQUENTIAL); }
    void advise_random()     const { madvise(addr_, len_, MADV_RANDOM); }
};

// Usage
MmapRegion region("data.bin");
auto ints = region.span_as<uint32_t>();
for (uint32_t v : ints) { use(v); }

MmapRegion rw_region("output.bin", /*writable=*/true);
rw_region.as<Header>()->magic = 0xCAFE;
rw_region.sync();
// munmap called automatically
```

---

## Shared Memory Between Processes

```cpp
// POSIX shared memory
#include <sys/mman.h>
#include <fcntl.h>

// Create/open shared memory object
int shm_fd = shm_open("/myapp_shm", O_CREAT | O_RDWR, 0600);
ftruncate(shm_fd, sizeof(SharedData));

SharedData* shm = static_cast<SharedData*>(
    mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
close(shm_fd);

// Use shm — visible to other processes that open the same name
shm->counter = 42;
munmap(shm, sizeof(SharedData));

// Cleanup
shm_unlink("/myapp_shm");
```

---

## Pitfalls

```cpp
// 1. offset must be page-aligned
size_t page = sysconf(_SC_PAGESIZE);
size_t aligned_off = (offset / page) * page;    // round down to page boundary
size_t delta = offset - aligned_off;             // extra bytes at start
char* p = (char*)mmap(nullptr, len + delta, PROT_READ, MAP_PRIVATE, fd, aligned_off);
char* data = p + delta;   // actual start of desired data

// 2. Accessing beyond mapped length → SIGBUS or SIGSEGV
// Always stay within [addr, addr+len)

// 3. MAP_SHARED write without fsync/msync doesn't guarantee durability
// OS may not flush to disk before crash
msync(ptr, len, MS_SYNC);

// 4. File shrunk after mapping → SIGBUS on access to removed pages
// Avoid ftruncate(shrink) while mapped

// 5. Reinterpret cast for unaligned struct access → UB
// Use memcpy instead of pointer cast for strict-aliasing safety
SomeStruct s;
std::memcpy(&s, ptr + offset, sizeof(s));   // safe
// auto* p = reinterpret_cast<SomeStruct*>(ptr + offset);  // UB if misaligned

// 6. mmap on macOS vs Linux: MAP_ANONYMOUS not available on macOS by name
// Use MAP_ANON (BSD alias) or check with #ifdef
#ifdef __APPLE__
    #define MAP_ANONYMOUS MAP_ANON
#endif

// 7. MAP_FAILED is (void*)-1, not nullptr — always compare to MAP_FAILED
void* p = mmap(...);
if (p == nullptr) { }      // WRONG
if (p == MAP_FAILED) { }   // CORRECT
```

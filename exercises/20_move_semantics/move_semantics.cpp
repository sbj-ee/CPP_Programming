// =============================================================================
// Exercise 20: Move Semantics
// =============================================================================
// Topics: lvalue vs rvalue, T&& rvalue reference, move ctor/assign,
//         Rule of Five, std::move, std::forward, RVO/NRVO,
//         string/vector move benefits, noexcept on move ops
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g move_semantics.cpp -o move_semantics
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

// =============================================================================
// SECTION 1: lvalue vs rvalue — Reference Categories
// =============================================================================
//
// lvalue: an expression that has a persistent identity (name, addressable).
//         Can appear on the left of an assignment.  Examples: variables, *ptr.
//
// rvalue: a temporary with no persistent identity.  Cannot take its address.
//         Examples: literals, arithmetic, function return values (usually).
//
// lvalue reference (T&): binds to lvalues only.
// rvalue reference (T&&): binds to rvalues only.
// const lvalue reference (const T&): binds to BOTH — the original "universal" ref.

void takes_lvalue_ref(int& x)        { std::cout << "  lvalue ref: " << x << "\n"; }
void takes_rvalue_ref(int&& x)       { std::cout << "  rvalue ref: " << x << "\n"; }
void takes_const_ref(const int& x)   { std::cout << "  const ref:  " << x << "\n"; }

void demo_value_categories() {
    std::cout << "\n--- Section 1: lvalue vs rvalue ---\n";

    int a = 42;               // a is an lvalue
    takes_lvalue_ref(a);      // OK: a is lvalue
    takes_const_ref(a);       // OK: const ref binds lvalue
    takes_const_ref(99);      // OK: const ref binds rvalue (temporary)
    takes_rvalue_ref(99);     // OK: 99 is rvalue
    takes_rvalue_ref(std::move(a));  // OK: std::move casts a to rvalue

    // Attempting takes_lvalue_ref(99) or takes_rvalue_ref(a) would be compile errors.

    std::cout << "  a after std::move: " << a << "  (value unchanged — move is a cast)\n";
}

// =============================================================================
// SECTION 2: Move Constructor and Move Assignment Operator
// =============================================================================
//
// A move constructor 'steals' resources from a soon-to-be-destroyed temporary.
// It must leave the moved-from object in a VALID but UNSPECIFIED state
// (typically null/empty so the destructor is safe).
//
// noexcept is crucial: std::vector::push_back uses move only if noexcept;
// otherwise it falls back to copy (for exception safety).

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(new int[size]), size_(size)
    {
        std::cout << "  Buffer(" << size_ << ") constructed\n";
        for (std::size_t i = 0; i < size_; ++i) data_[i] = static_cast<int>(i);
    }

    // Copy constructor — deep copy
    Buffer(const Buffer& other)
        : data_(new int[other.size_]), size_(other.size_)
    {
        std::cout << "  Buffer COPY constructed (size=" << size_ << ")\n";
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // Move constructor — transfer ownership, O(1)
    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_)
    {
        std::cout << "  Buffer MOVE constructed (size=" << size_ << ")\n";
        other.data_ = nullptr;   // leave moved-from in valid state
        other.size_ = 0;
    }

    // Copy assignment
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        std::cout << "  Buffer COPY assigned\n";
        delete[] data_;
        data_ = new int[other.size_];
        size_ = other.size_;
        std::copy(other.data_, other.data_ + size_, data_);
        return *this;
    }

    // Move assignment — transfer, O(1)
    Buffer& operator=(Buffer&& other) noexcept {
        std::cout << "  Buffer MOVE assigned\n";
        if (this == &other) return *this;
        delete[] data_;
        data_       = other.data_;
        size_       = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    // Destructor (Rule of Five: if you define any of the 5, define all)
    ~Buffer() {
        std::cout << "  ~Buffer(size=" << size_ << ")\n";
        delete[] data_;
    }

    std::size_t size() const noexcept { return size_; }
    int& operator[](std::size_t i)       { return data_[i]; }
    int  operator[](std::size_t i) const { return data_[i]; }

private:
    int*        data_;
    std::size_t size_;
};

void demo_move_ctor() {
    std::cout << "\n--- Section 2: Move Constructor / Assignment ---\n";

    Buffer b1(4);
    std::cout << "  b1[0]=" << b1[0] << " b1[3]=" << b1[3] << "\n";

    Buffer b2 = std::move(b1);  // move constructor
    std::cout << "  b1.size() after move: " << b1.size() << "  (empty)\n";
    std::cout << "  b2[0]=" << b2[0] << "\n";

    Buffer b3(2);
    b3 = std::move(b2);          // move assignment
    std::cout << "  b2.size() after move assign: " << b2.size() << "\n";
    std::cout << "  b3[0]=" << b3[0] << "\n";
}

// =============================================================================
// SECTION 3: Rule of Five
// =============================================================================
//
// If you declare ANY of:
//   destructor, copy ctor, copy assign, move ctor, move assign
// you almost certainly need to declare ALL FIVE explicitly.
//
// The Rule of Zero is the counterpart: if your class manages no raw resource
// (using smart pointers / STL members instead), declare none of the five and
// let the compiler generate correct defaults.

struct RuleOfZero {
    std::string name;
    std::vector<int> data;
    // No explicit destructor / copy / move needed — compiler-generated are correct
};

void demo_rule_of_five() {
    std::cout << "\n--- Section 3: Rule of Five / Rule of Zero ---\n";

    std::cout << "  Rule-of-Five: Buffer class above implements all 5 explicitly.\n";

    RuleOfZero rz1;
    rz1.name = "zero";
    rz1.data = {1, 2, 3};

    RuleOfZero rz2 = rz1;               // compiler-generated copy
    RuleOfZero rz3 = std::move(rz1);    // compiler-generated move

    std::cout << "  RuleOfZero copy name: " << rz2.name << "\n";
    std::cout << "  RuleOfZero moved name: " << rz3.name << "\n";
    std::cout << "  rz1.name after move: '" << rz1.name << "' (valid but unspecified)\n";
}

// =============================================================================
// SECTION 4: std::move — It's Just a Cast
// =============================================================================
//
// std::move(x) is equivalent to static_cast<T&&>(x).  It does NOT move
// anything by itself; it merely makes the argument eligible to be moved from
// by converting it to an rvalue reference.  The actual resource transfer
// happens in the move constructor or move assignment that is then called.

void demo_std_move() {
    std::cout << "\n--- Section 4: std::move ---\n";

    std::string s1 = "Hello, World!";
    std::cout << "  before move: s1='" << s1 << "'\n";

    std::string s2 = std::move(s1);  // move ctor; s1 is now ""
    std::cout << "  after move:  s1='" << s1 << "'  s2='" << s2 << "'\n";

    // After std::move, s1 is still a valid (empty) string — can reassign
    s1 = "Reassigned";
    std::cout << "  after reassign: s1='" << s1 << "'\n";

    // std::move on rvalue has no extra effect (already rvalue)
    Buffer b = Buffer(3);       // temporary is already rvalue — no redundant move needed
    std::cout << "  Buffer created inline, size=" << b.size() << "\n";
}

// =============================================================================
// SECTION 5: std::forward and Perfect Forwarding
// =============================================================================
//
// Perfect forwarding preserves the value category (lvalue or rvalue) of
// arguments when passing them through a template function.
//
// T&& in a template context is a FORWARDING REFERENCE (also called universal
// reference) — it collapses to T& for lvalue args and T&& for rvalue args.
// std::forward<T>(arg) restores the original category.

void process_lvalue(int& x) { std::cout << "  process(lvalue): " << x << "\n"; }
void process_rvalue(int&& x) { std::cout << "  process(rvalue): " << x << "\n"; }

template <typename T>
void forwarding_wrapper(T&& arg) {
    // Without forward: always passes as lvalue (arg has a name => lvalue)
    // With forward:    preserves the original value category
    if constexpr (std::is_lvalue_reference_v<T>) {
        process_lvalue(std::forward<T>(arg));
    } else {
        process_rvalue(std::forward<T>(arg));
    }
}

// Factory: perfectly forwards constructor arguments to T
template <typename T, typename... Args>
T create(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

void demo_forward() {
    std::cout << "\n--- Section 5: std::forward (Perfect Forwarding) ---\n";

    int x = 10;
    forwarding_wrapper(x);    // lvalue: process_lvalue called
    forwarding_wrapper(42);   // rvalue: process_rvalue called

    auto buf = create<Buffer>(5);  // perfectly forwards '5' to Buffer(size_t)
    std::cout << "  created Buffer size=" << buf.size() << "\n";
}

// =============================================================================
// SECTION 6: Return Value Optimisation (RVO / NRVO)
// =============================================================================
//
// RVO (Named Return Value Optimisation): the compiler constructs the return
// value directly in the caller's storage — no copy or move at all.
// NRVO is the "Named" variant where the named local variable is elided.
// In C++17, copy elision in certain contexts is MANDATORY (prvalue elision).
//
// Rule: return local variables by value WITHOUT std::move — adding std::move
// can actually PREVENT copy elision and force a move instead.

Buffer make_buffer(std::size_t n) {
    Buffer b(n);           // NRVO: constructed directly in caller storage
    b[0] = 99;
    return b;              // do NOT write return std::move(b) — inhibits NRVO
}

void demo_rvo() {
    std::cout << "\n--- Section 6: RVO / NRVO ---\n";

    std::cout << "  Calling make_buffer(3)...\n";
    Buffer result = make_buffer(3);  // ideally: zero copies, zero moves (elided)
    std::cout << "  result[0]=" << result[0] << " size=" << result.size() << "\n";
    std::cout << "  (if RVO/NRVO fired, you saw only one 'constructed' message)\n";
}

// =============================================================================
// SECTION 7: std::string and std::vector Benefit from Move Semantics
// =============================================================================

void demo_string_vector_move() {
    std::cout << "\n--- Section 7: string / vector with move ---\n";

    // string move: O(1) vs O(n) copy
    std::string big(10000, 'x');  // 10K chars
    std::string moved = std::move(big);   // O(1) pointer swap
    std::cout << "  moved string size=" << moved.size()
              << " original size=" << big.size() << " (empty)\n";

    // vector::push_back benefits from move-enabled elements
    std::vector<Buffer> vec;
    vec.reserve(3);  // avoid reallocation (which would copy if not noexcept)
    vec.emplace_back(2);    // in-place construction — best
    vec.push_back(Buffer(3));  // rvalue: move constructor called
    Buffer b(4);
    vec.push_back(std::move(b));  // explicit move
    std::cout << "  vector of Buffers, sizes: ";
    for (const auto& buf : vec) std::cout << buf.size() << " ";
    std::cout << "\n";
}

// =============================================================================
// SECTION 8: noexcept on Move Operations
// =============================================================================
//
// std::vector resizes by allocating new storage and moving elements.
// If the move constructor is NOT noexcept, vector falls back to COPY to
// maintain the strong exception guarantee.  Marking move ops noexcept
// therefore enables the fast path in vector reallocation.

void demo_noexcept_move() {
    std::cout << "\n--- Section 8: noexcept on Move Operations ---\n";

    std::cout << "  Buffer move ctor is noexcept? "
              << (std::is_nothrow_move_constructible<Buffer>::value ? "yes" : "no")
              << "\n";
    std::cout << "  std::string move ctor is noexcept? "
              << (std::is_nothrow_move_constructible<std::string>::value ? "yes" : "no")
              << "\n";
    std::cout << "  std::vector<int> move ctor is noexcept? "
              << (std::is_nothrow_move_constructible<std::vector<int>>::value ? "yes" : "no")
              << "\n";

    // Demonstrate vector reallocation choosing move (Buffer is noexcept move)
    std::vector<Buffer> vec;
    // No reserve: triggers reallocation and moves
    std::cout << "  Pushing 3 Buffers (may trigger realloc + moves):\n";
    vec.push_back(Buffer(1));
    vec.push_back(Buffer(2));
    vec.push_back(Buffer(3));
    std::cout << "  vec sizes: ";
    for (const auto& b : vec) std::cout << b.size() << " ";
    std::cout << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 20: Move Semantics ===\n";

    demo_value_categories();
    demo_move_ctor();
    demo_rule_of_five();
    demo_std_move();
    demo_forward();
    demo_rvo();
    demo_string_vector_move();
    demo_noexcept_move();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. std::move is a CAST — it does not move anything by itself;\n"
              << "   the actual resource transfer happens in move ctor/assign.\n";
    std::cout << "2. After a move, the moved-from object is in a valid but\n"
              << "   unspecified state — don't rely on its value.\n";
    std::cout << "3. 'return std::move(local);' disables NRVO — just write\n"
              << "   'return local;' and let the compiler optimise.\n";
    std::cout << "4. T&& in a template is a forwarding reference, not a plain\n"
              << "   rvalue reference; use std::forward<T> to preserve category.\n";
    std::cout << "5. Mark move constructors/assignments noexcept so std::vector\n"
              << "   can use them during reallocation instead of copying.\n";

    return 0;
}

// =============================================================================
// Exercise 19: Smart Pointers
// =============================================================================
// Topics: unique_ptr, shared_ptr, weak_ptr, cycle breaking, enable_shared_from_this,
//         passing conventions, make_unique/make_shared vs new
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g smart_pointers.cpp -o smart_pointers
// =============================================================================

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <functional>

// =============================================================================
// SECTION 1: std::unique_ptr<T> — Sole Ownership
// =============================================================================
//
// unique_ptr expresses EXCLUSIVE ownership.  When the unique_ptr goes out of
// scope (or is reset), the pointed-to object is destroyed.
//
// - Make with std::make_unique<T>(args...) — never raw 'new'.
// - It is MOVE-ONLY (no copy constructor or copy assignment).
// - Transfer ownership with std::move().
// - Use a custom deleter for non-default cleanup.

class Widget {
public:
    explicit Widget(std::string name) : name_(std::move(name)) {
        std::cout << "  Widget(" << name_ << ") created\n";
    }
    ~Widget() {
        std::cout << "  Widget(" << name_ << ") destroyed\n";
    }
    const std::string& name() const { return name_; }

private:
    std::string name_;
};

void demo_unique_ptr() {
    std::cout << "\n--- Section 1: std::unique_ptr ---\n";

    // make_unique: preferred — no raw new, exception-safe
    auto w1 = std::make_unique<Widget>("Alpha");
    std::cout << "  owns: " << w1->name() << "\n";

    // Transfer ownership: w1 becomes null, w2 owns the object
    auto w2 = std::move(w1);
    std::cout << "  w1 is null? " << (w1 == nullptr ? "yes" : "no") << "\n";
    std::cout << "  w2 owns:    " << w2->name() << "\n";

    // Release: relinquish ownership without deleting (raw pointer returned)
    Widget* raw = w2.release();
    std::cout << "  after release, w2 null? " << (w2 == nullptr ? "yes" : "no") << "\n";
    delete raw;  // we now own it — must delete manually

    // Reset: destroy current, optionally take new
    auto w3 = std::make_unique<Widget>("Beta");
    w3.reset(new Widget("Gamma"));  // Beta destroyed, Gamma takes over
    // w3 goes out of scope here -> Gamma destroyed

    // Custom deleter (e.g., for a C resource)
    auto file_deleter = [](FILE* f) {
        if (f) { std::cout << "  custom deleter: closing file\n"; fclose(f); }
    };
    // Open /dev/null as a harmless demonstration target
    {
        std::unique_ptr<FILE, decltype(file_deleter)>
            fp(fopen("/dev/null", "r"), file_deleter);
        std::cout << "  file open: " << (fp ? "yes" : "no") << "\n";
    }  // custom deleter called here
}

// =============================================================================
// SECTION 2: std::shared_ptr<T> — Shared Ownership
// =============================================================================
//
// shared_ptr implements reference counting: the pointed-to object lives as long
// as at least one shared_ptr to it exists.  The control block stores the count.
//
// - Make with std::make_shared<T>(): allocates object + control block in one
//   heap allocation (more efficient than shared_ptr(new T)).
// - Copying a shared_ptr increments the reference count.
// - Moving a shared_ptr transfers ownership without changing the count.
// - use_count() returns the current reference count (for debugging only).

void demo_shared_ptr() {
    std::cout << "\n--- Section 2: std::shared_ptr ---\n";

    auto sp1 = std::make_shared<Widget>("Shared");
    std::cout << "  sp1 use_count: " << sp1.use_count() << "\n";  // 1

    {
        auto sp2 = sp1;   // copy: count becomes 2
        auto sp3 = sp1;   // copy: count becomes 3
        std::cout << "  sp1 use_count (sp2,sp3 alive): " << sp1.use_count() << "\n";
    }  // sp2, sp3 destroyed: count back to 1
    std::cout << "  sp1 use_count (after scope): " << sp1.use_count() << "\n";

    // Move: sp1 becomes null, sp4 takes sole ownership
    auto sp4 = std::move(sp1);
    std::cout << "  sp1 null? " << (sp1 == nullptr ? "yes" : "no") << "\n";
    std::cout << "  sp4 use_count: " << sp4.use_count() << "\n";  // 1

    // Vector of shared_ptrs: multiple owners
    std::vector<std::shared_ptr<Widget>> pool;
    pool.push_back(sp4);
    pool.push_back(sp4);
    std::cout << "  sp4 use_count in pool: " << sp4.use_count() << "\n";  // 3
}  // Widget "Shared" destroyed when last shared_ptr (pool entries) go out of scope

// =============================================================================
// SECTION 3: std::weak_ptr<T> — Non-Owning Observation
// =============================================================================
//
// weak_ptr holds a non-owning reference to a shared_ptr-managed object.
// It does NOT increment the reference count.
// To use the object, promote to shared_ptr via lock():
//   - Returns a valid shared_ptr if the object still exists.
//   - Returns an empty shared_ptr if the object has been destroyed.

void demo_weak_ptr() {
    std::cout << "\n--- Section 3: std::weak_ptr ---\n";

    std::weak_ptr<Widget> wp;

    {
        auto sp = std::make_shared<Widget>("Observed");
        wp = sp;   // weak reference; use_count still 1

        std::cout << "  wp.expired()? " << (wp.expired() ? "yes" : "no") << "\n";

        // Promote to shared_ptr to use the object
        if (auto locked = wp.lock()) {
            std::cout << "  locked: " << locked->name()
                      << " use_count=" << locked.use_count() << "\n";  // 2 while locked
        }
    }  // sp destroyed; Widget destroyed; use_count drops to 0

    std::cout << "  wp.expired() after sp gone? " << (wp.expired() ? "yes" : "no") << "\n";
    auto dead = wp.lock();
    std::cout << "  lock() after expiry: " << (dead == nullptr ? "nullptr" : "valid") << "\n";
}

// =============================================================================
// SECTION 4: Breaking Reference Cycles with weak_ptr
// =============================================================================
//
// If two objects hold shared_ptrs to each other, their reference counts never
// reach zero — a memory leak.  The solution: one direction uses weak_ptr.

struct TreeNode {
    explicit TreeNode(int v) : value(v) {
        std::cout << "  TreeNode(" << v << ") created\n";
    }
    ~TreeNode() {
        std::cout << "  TreeNode(" << value << ") destroyed\n";
    }

    int value;
    std::vector<std::shared_ptr<TreeNode>> children;
    std::weak_ptr<TreeNode> parent;  // weak: parent does not own child
};

void demo_cycle_breaking() {
    std::cout << "\n--- Section 4: Breaking Cycles with weak_ptr ---\n";

    auto root  = std::make_shared<TreeNode>(1);
    auto child = std::make_shared<TreeNode>(2);

    root->children.push_back(child);  // root owns child
    child->parent = root;             // weak: child does NOT own root

    std::cout << "  root use_count="  << root.use_count()  << "\n";  // 1
    std::cout << "  child use_count=" << child.use_count() << "\n";  // 2 (root + local)

    // Navigate from child to parent via weak_ptr
    if (auto p = child->parent.lock()) {
        std::cout << "  child's parent value: " << p->value << "\n";
    }

    // When root and child go out of scope, both are properly destroyed.
    // If parent were a shared_ptr, root->child and child->parent would create
    // a cycle and neither would ever be freed.
}

// =============================================================================
// SECTION 5: std::enable_shared_from_this
// =============================================================================
//
// If a method needs to return a shared_ptr to 'this', it cannot just use
// shared_ptr<T>(this) — that creates a second independent control block,
// leading to double-free.  Inherit from enable_shared_from_this<T> and call
// shared_from_this() instead.

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(int id) : id_(id) {
        std::cout << "  Session(" << id_ << ") created\n";
    }
    ~Session() {
        std::cout << "  Session(" << id_ << ") destroyed\n";
    }

    // Safe: shares ownership with the existing control block
    std::shared_ptr<Session> get_shared() {
        return shared_from_this();
    }

    int id() const { return id_; }

private:
    int id_;
};

void demo_enable_shared_from_this() {
    std::cout << "\n--- Section 5: enable_shared_from_this ---\n";

    auto s1 = std::make_shared<Session>(42);
    auto s2 = s1->get_shared();  // shares control block, NOT a new one

    std::cout << "  s1.use_count() = " << s1.use_count() << "\n";  // 2
    std::cout << "  s1 == s2? " << (s1 == s2 ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 6: Passing Smart Pointers — Conventions
// =============================================================================
//
// Pass by value (unique_ptr):  TRANSFER ownership to callee.
// Pass by const ref (unique_ptr):  OBSERVE without taking ownership.
// Pass raw pointer / reference:  USE ONLY — caller retains ownership.
// Pass by value (shared_ptr):  SHARE ownership (increments count).
// Pass by const ref (shared_ptr):  OBSERVE without incrementing count.
//
// Scott Meyers' rule of thumb:
//   "If the function doesn't manipulate lifetime, pass a raw pointer or ref."

void take_ownership(std::unique_ptr<Widget> w) {
    std::cout << "  took ownership of: " << w->name() << "\n";
}  // w destroyed here

void observe_only(const Widget& w) {
    std::cout << "  observing: " << w.name() << "\n";
}

void share_ownership(std::shared_ptr<Widget> sp) {
    std::cout << "  sharing: " << sp->name()
              << " use_count=" << sp.use_count() << "\n";
}

void demo_passing() {
    std::cout << "\n--- Section 6: Passing Smart Pointers ---\n";

    auto up = std::make_unique<Widget>("PassMe");
    observe_only(*up);          // raw ref: no ownership change
    take_ownership(std::move(up));  // transfer: up is now null
    std::cout << "  up after transfer: " << (up == nullptr ? "null" : "valid") << "\n";

    auto sp = std::make_shared<Widget>("ShareMe");
    std::cout << "  before call use_count=" << sp.use_count() << "\n";
    share_ownership(sp);        // copy: increments count during call
    std::cout << "  after call use_count=" << sp.use_count() << "\n";
}

// =============================================================================
// SECTION 7: make_unique / make_shared vs new — Exception Safety
// =============================================================================
//
// Consider: f(shared_ptr<T>(new T), g())
// With C++ evaluation order rules, it was possible for:
//   1. new T executes (memory allocated)
//   2. g() throws (exception)
//   3. shared_ptr<T>() never constructed — LEAK
//
// make_shared / make_unique prevent this by combining allocation and construction.
// In C++17 the order-of-evaluation rules improved, but make_* is still preferred
// for clarity and (with make_shared) the single-allocation efficiency.

void demo_make_vs_new() {
    std::cout << "\n--- Section 7: make_unique / make_shared vs new ---\n";

    // Preferred: single allocation, exception-safe
    auto sp1 = std::make_shared<Widget>("MakeShared");

    // Less preferred: two allocations (object + control block)
    std::shared_ptr<Widget> sp2(new Widget("RawNew"));

    std::cout << "  sp1 ok: " << sp1->name() << "\n";
    std::cout << "  sp2 ok: " << sp2->name() << "\n";

    // make_unique also cleaner for arrays
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * 10;
    std::cout << "  make_unique<int[]>: ";
    for (int i = 0; i < 5; ++i) std::cout << arr[i] << " ";
    std::cout << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 19: Smart Pointers ===\n";

    demo_unique_ptr();
    demo_shared_ptr();
    demo_weak_ptr();
    demo_cycle_breaking();
    demo_enable_shared_from_this();
    demo_passing();
    demo_make_vs_new();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Never call shared_ptr<T>(this) inside a member function;\n"
              << "   use enable_shared_from_this + shared_from_this() instead.\n";
    std::cout << "2. shared_ptr cycles cause memory leaks; break cycles with\n"
              << "   weak_ptr on the 'back' edge (e.g., parent pointers).\n";
    std::cout << "3. Always lock() a weak_ptr before use and check the result;\n"
              << "   the object may have been destroyed.\n";
    std::cout << "4. Prefer make_unique / make_shared over new for exception\n"
              << "   safety and (with make_shared) allocation efficiency.\n";
    std::cout << "5. When passing smart pointers to functions, only pass by\n"
              << "   value when the function actually participates in ownership;\n"
              << "   otherwise pass raw pointer or reference.\n";

    return 0;
}

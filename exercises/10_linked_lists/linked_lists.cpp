// linked_lists.cpp - Exercise 10: Linked Lists
// Demonstrates: singly linked list class template, push_front/push_back,
//               pop_front, remove, contains, print, proper destructor,
//               move constructor, std::list comparison

#include <iostream>
#include <list>       // std::list from STL
#include <string>
#include <utility>    // std::move

// ── LinkedList<T> class template ─────────────────────────────────────────────

template<typename T>
class LinkedList {
private:
    // Internal node type (private: users do not interact with it directly)
    struct Node {
        T     value;
        Node* next;

        explicit Node(const T& v, Node* n = nullptr)
            : value(v), next(n) {}

        // In-place construction for emplace-style operations
        explicit Node(T&& v, Node* n = nullptr)
            : value(std::move(v)), next(n) {}
    };

    Node*       head_;
    std::size_t size_;

public:
    // ── Constructors / Destructor ──────────────────────────────────────────────

    // Default constructor
    LinkedList() : head_(nullptr), size_(0)
    {
        std::cout << "  [LinkedList] default ctor\n";
    }

    // Destructor: must free all nodes to prevent memory leak
    ~LinkedList()
    {
        clear();
        std::cout << "  [LinkedList] dtor (all nodes freed)\n";
    }

    // Copy constructor (deep copy)
    LinkedList(const LinkedList& other) : head_(nullptr), size_(0)
    {
        std::cout << "  [LinkedList] copy ctor\n";
        // Walk the other list from back to front would reverse; instead rebuild:
        // Collect values in order first
        vector_workaround_copy(other);
    }

    // Move constructor: steal the other list's head (O(1))
    LinkedList(LinkedList&& other) noexcept
        : head_(other.head_), size_(other.size_)
    {
        other.head_ = nullptr;
        other.size_ = 0;
        std::cout << "  [LinkedList] move ctor (stole " << size_ << " nodes)\n";
    }

    // Copy/move assignment deleted for simplicity
    LinkedList& operator=(const LinkedList&) = delete;
    LinkedList& operator=(LinkedList&&)      = delete;

    // ── Mutating operations ────────────────────────────────────────────────────

    // push_front: O(1) - insert at head
    void push_front(const T& val)
    {
        head_ = new Node(val, head_);
        ++size_;
    }

    // push_back: O(n) - insert at tail
    void push_back(const T& val)
    {
        Node* new_node = new Node(val);
        if (!head_) {
            head_ = new_node;
        } else {
            Node* cur = head_;
            while (cur->next)
                cur = cur->next;
            cur->next = new_node;
        }
        ++size_;
    }

    // pop_front: O(1) - remove head
    void pop_front()
    {
        if (!head_) {
            std::cerr << "  pop_front on empty list\n";
            return;
        }
        Node* old = head_;
        head_ = head_->next;
        delete old;
        --size_;
    }

    // remove(val): remove first occurrence of val, O(n)
    bool remove(const T& val)
    {
        if (!head_) return false;

        // Special case: head holds the value
        if (head_->value == val) {
            Node* old = head_;
            head_ = head_->next;
            delete old;
            --size_;
            return true;
        }

        // General case: find predecessor
        Node* prev = head_;
        while (prev->next) {
            if (prev->next->value == val) {
                Node* to_del = prev->next;
                prev->next = to_del->next;
                delete to_del;
                --size_;
                return true;
            }
            prev = prev->next;
        }
        return false;   // not found
    }

    // clear: remove all nodes
    void clear()
    {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
        head_ = nullptr;
        size_ = 0;
    }

    // ── Queries ────────────────────────────────────────────────────────────────

    bool contains(const T& val) const
    {
        for (Node* cur = head_; cur; cur = cur->next)
            if (cur->value == val)
                return true;
        return false;
    }

    std::size_t size()  const { return size_; }
    bool        empty() const { return size_ == 0; }

    const T& front() const
    {
        if (!head_) throw std::runtime_error("front() on empty list");
        return head_->value;
    }

    // ── Iterator-style traversal (print) ──────────────────────────────────────

    void print() const
    {
        std::cout << "  [";
        for (Node* cur = head_; cur; cur = cur->next) {
            std::cout << cur->value;
            if (cur->next) std::cout << " -> ";
        }
        std::cout << "]  (size=" << size_ << ")\n";
    }

private:
    // Helper for copy ctor: perform a two-pass copy to preserve order
    void vector_workaround_copy(const LinkedList& other)
    {
        // Collect values into a temporary stack, then push_back
        // (to avoid reversing the order)
        if (!other.head_) return;

        // Count elements and build in order using recursion-free approach:
        // Walk source, temporarily store in a local array on stack via recursion
        // would be complex for large lists; instead use push_back for simplicity.
        for (Node* cur = other.head_; cur; cur = cur->next)
            push_back(cur->value);
    }
};

// ── Section 1: Basic operations ───────────────────────────────────────────────

void section_basic_operations()
{
    std::cout << "=== Section 1: LinkedList<int> Basic Operations ===\n";
    {
        LinkedList<int> list;
        list.push_back(10);
        list.push_back(20);
        list.push_back(30);
        list.push_front(5);
        list.print();

        std::cout << "  size()=" << list.size()
                  << " front()=" << list.front()
                  << " empty()=" << std::boolalpha << list.empty() << "\n";

        std::cout << "  contains(20)=" << list.contains(20) << "\n";
        std::cout << "  contains(99)=" << list.contains(99) << "\n";

        list.pop_front();
        std::cout << "  After pop_front:"; list.print();

        list.remove(20);
        std::cout << "  After remove(20):"; list.print();

        list.remove(99);   // not found
        std::cout << "  After remove(99):"; list.print();
    }
}

// ── Section 2: LinkedList<std::string> ───────────────────────────────────────

void section_string_list()
{
    std::cout << "\n=== Section 2: LinkedList<std::string> ===\n";
    {
        LinkedList<std::string> words;
        words.push_back("alpha");
        words.push_back("beta");
        words.push_back("gamma");
        words.push_front("zeta");
        words.print();

        words.remove("beta");
        words.print();
    }
}

// ── Section 3: Move constructor ───────────────────────────────────────────────

void section_move()
{
    std::cout << "\n=== Section 3: Move Constructor ===\n";
    {
        LinkedList<int> original;
        original.push_back(1);
        original.push_back(2);
        original.push_back(3);
        std::cout << "  original:"; original.print();

        // Move: ownership transferred; original becomes empty
        LinkedList<int> moved = std::move(original);
        std::cout << "  moved   :"; moved.print();
        std::cout << "  original:"; original.print();
    }
}

// ── Section 4: Destructor frees all nodes ─────────────────────────────────────

void section_destructor()
{
    std::cout << "\n=== Section 4: Destructor Frees All Nodes ===\n";
    {
        LinkedList<int> leaking_candidate;
        for (int i = 0; i < 5; ++i)
            leaking_candidate.push_back(i * 100);
        leaking_candidate.print();
        // All 5 nodes freed when leaking_candidate goes out of scope
    }
    std::cout << "  (scope ended; destructor freed all nodes - no leak)\n";
}

// ── Section 5: std::list<T> from STL ─────────────────────────────────────────

void section_std_list()
{
    std::cout << "\n=== Section 5: std::list<T> from STL (for comparison) ===\n";

    std::list<int> sl = {10, 20, 30, 40, 50};

    // push/pop both ends in O(1)
    sl.push_front(5);
    sl.push_back(55);

    std::cout << "  std::list: ";
    for (const int& v : sl) std::cout << v << " ";
    std::cout << "\n";

    sl.remove(30);   // remove ALL occurrences of 30
    std::cout << "  After remove(30): ";
    for (const int& v : sl) std::cout << v << " ";
    std::cout << "\n";

    // Bidirectional iteration (back to front)
    std::cout << "  Reverse: ";
    for (auto it = sl.rbegin(); it != sl.rend(); ++it)
        std::cout << *it << " ";
    std::cout << "\n";

    std::cout << "  size()=" << sl.size() << "\n";
    std::cout << "\n  std::list advantages over our LinkedList:\n";
    std::cout << "    - Bidirectional iterators (rbegin/rend)\n";
    std::cout << "    - O(1) push/pop on both ends\n";
    std::cout << "    - splice(), merge(), sort(), reverse() built in\n";
    std::cout << "    - No manual memory management\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Template class: LinkedList<T> works for any T.\n";
    std::cout << "  - Destructor must walk and delete every node to avoid leaks.\n";
    std::cout << "  - Move ctor transfers head pointer in O(1); no nodes copied.\n";
    std::cout << "  - Prefer std::list (or std::vector) over hand-rolled lists in production.\n";
    std::cout << "  - Linked lists have poor cache locality; vectors are usually faster.\n";
}

int main()
{
    section_basic_operations();
    section_string_list();
    section_move();
    section_destructor();
    section_std_list();
    return 0;
}

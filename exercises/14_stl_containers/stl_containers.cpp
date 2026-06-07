// =============================================================================
// Exercise 14: STL (Standard Template Library) Containers
// =============================================================================
// Topics: vector, deque, list, map, unordered_map, set, unordered_set,
//         iteration (range-for and iterators), container adapters
//         (stack, queue, priority_queue)
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g stl_containers.cpp -o stl_containers
// =============================================================================

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <stack>
#include <queue>
#include <string>
#include <algorithm>

// Helper: print any iterable with a label
template <typename Container>
void print_container(const Container& c, const std::string& label) {
    std::cout << label << ": [ ";
    for (const auto& v : c) std::cout << v << " ";
    std::cout << "]\n";
}

// =============================================================================
// SECTION 1: std::vector<T> — Dynamic Array
// =============================================================================
//
// vector stores elements contiguously in heap memory.  Accessing elements by
// index is O(1).  push_back is amortised O(1); inserting/erasing in the middle
// is O(n) because elements must shift.
//
// reserve() pre-allocates capacity to avoid repeated reallocations.
// emplace_back() constructs in-place — avoids a copy when the element type
// is expensive to copy.

void demo_vector() {
    std::cout << "\n--- Section 1: std::vector ---\n";

    std::vector<int> v;
    v.reserve(8);                         // pre-allocate, no size change
    v.push_back(10);
    v.push_back(20);
    v.emplace_back(30);                   // in-place construction
    v.insert(v.begin() + 1, 15);         // insert 15 at index 1

    print_container(v, "after inserts");  // [10 15 20 30]

    v.erase(v.begin() + 2);              // remove element at index 2 (20)
    print_container(v, "after erase");   // [10 15 30]

    std::cout << "size=" << v.size() << " capacity=" << v.capacity() << "\n";
    std::cout << "v[0]=" << v[0] << "  v.at(1)=" << v.at(1) << "\n";

    // Iterator loop
    std::cout << "iterator loop: ";
    for (auto it = v.begin(); it != v.end(); ++it) std::cout << *it << " ";
    std::cout << "\n";

    // Resize: fills new elements with zero (or value)
    v.resize(6, 99);
    print_container(v, "after resize(6,99)");
}

// =============================================================================
// SECTION 2: std::deque<T> — Double-Ended Queue
// =============================================================================
//
// deque supports O(1) push/pop at BOTH ends.  It is implemented as a sequence
// of fixed-size chunks — not contiguous like vector, so pointer arithmetic
// across the whole deque is not valid.  Random access is still O(1).

void demo_deque() {
    std::cout << "\n--- Section 2: std::deque ---\n";

    std::deque<int> dq = {3, 4, 5};
    dq.push_front(2);   // O(1) front insert
    dq.push_front(1);
    dq.push_back(6);    // O(1) back insert
    print_container(dq, "deque");    // [1 2 3 4 5 6]

    dq.pop_front();
    dq.pop_back();
    print_container(dq, "after pop_front/back");  // [2 3 4 5]

    std::cout << "dq[1]=" << dq[1] << "  dq.front()=" << dq.front()
              << "  dq.back()=" << dq.back() << "\n";
}

// =============================================================================
// SECTION 3: std::list<T> — Doubly-Linked List
// =============================================================================
//
// list has O(1) insert/erase anywhere (given an iterator), but O(n) random
// access.  Use it when you need frequent insertion/removal in the middle and
// do NOT need random access.  splice() moves elements between lists in O(1).

void demo_list() {
    std::cout << "\n--- Section 3: std::list ---\n";

    std::list<int> lst = {1, 3, 5, 7};
    std::list<int> extra = {10, 20};

    // Insert before position 2 (the iterator to the third element)
    auto it = lst.begin();
    std::advance(it, 2);         // O(n) advance for list
    lst.insert(it, 4);           // insert 4 before 5
    print_container(lst, "after insert");  // [1 3 4 5 7]

    // splice: moves extra into lst before 'it' — O(1)
    lst.splice(it, extra);
    print_container(lst, "after splice");  // [1 3 4 10 20 5 7]
    print_container(extra, "extra (now empty)");

    lst.remove(4);               // removes ALL elements equal to 4
    print_container(lst, "after remove(4)");
}

// =============================================================================
// SECTION 4: std::map<K,V> — Sorted Key-Value Store
// =============================================================================
//
// map is a balanced BST (typically red-black tree).  Keys are always sorted.
// insert/find/erase are O(log n).
// operator[] inserts a default-constructed value if the key is absent — which
// can be surprising and is never const-correct; prefer 'find' for lookup.

void demo_map() {
    std::cout << "\n--- Section 4: std::map ---\n";

    std::map<std::string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"]   = 82;
    scores.insert({"Charlie", 88});
    scores.emplace("Diana", 91);

    // Iteration is always in sorted key order
    std::cout << "All scores (sorted):\n";
    for (const auto& [name, score] : scores) {  // C++17 structured binding
        std::cout << "  " << name << ": " << score << "\n";
    }

    // find — O(log n), returns iterator or end()
    auto found = scores.find("Bob");
    if (found != scores.end()) {
        std::cout << "Found Bob: " << found->second << "\n";
    }

    // count — returns 0 or 1 for map
    std::cout << "Has Eve? " << (scores.count("Eve") ? "yes" : "no") << "\n";

    scores.erase("Bob");
    std::cout << "After erasing Bob, size=" << scores.size() << "\n";
}

// =============================================================================
// SECTION 5: std::unordered_map<K,V> — Hash Map
// =============================================================================
//
// unordered_map uses a hash table.  Average O(1) insert/find/erase, but
// worst-case O(n) on hash collisions.  Keys are NOT sorted.
// Requires that K is hashable (std::hash<K> must exist or be specialised).

void demo_unordered_map() {
    std::cout << "\n--- Section 5: std::unordered_map ---\n";

    std::unordered_map<std::string, int> word_count;
    std::vector<std::string> words = {"cat", "dog", "cat", "bird", "dog", "cat"};

    for (const auto& w : words) {
        ++word_count[w];   // default-inserts 0 if new, then increments
    }

    std::cout << "Word counts (unordered):\n";
    for (const auto& [word, count] : word_count) {
        std::cout << "  " << word << ": " << count << "\n";
    }

    std::cout << "bucket_count=" << word_count.bucket_count()
              << " load_factor=" << word_count.load_factor() << "\n";
}

// =============================================================================
// SECTION 6: std::set and std::unordered_set — Unique Elements
// =============================================================================
//
// set: sorted, unique, BST-backed, O(log n) operations.
// unordered_set: hash-backed, unique, O(1) average.
// Both guarantee uniqueness — inserting a duplicate is silently ignored.

void demo_sets() {
    std::cout << "\n--- Section 6: std::set / std::unordered_set ---\n";

    std::set<int> s = {5, 3, 1, 4, 2, 3, 1};  // duplicates dropped; sorted
    print_container(s, "set (sorted, unique)");

    s.insert(6);
    s.erase(3);
    std::cout << "Has 4? " << (s.count(4) ? "yes" : "no") << "\n";
    print_container(s, "after insert(6) erase(3)");

    std::unordered_set<std::string> us = {"apple", "banana", "apple", "cherry"};
    std::cout << "unordered_set size=" << us.size() << " (unique only)\n";
    us.insert("date");
    std::cout << "Has banana? " << (us.count("banana") ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 7: Iteration — Range-for vs Iterators
// =============================================================================
//
// Range-for (C++11) is the cleanest syntax for simple traversal.
// Iterator loops give more control: mid-loop erase, reverse iteration, etc.
// Always use 'const auto&' in range-for when you don't need to modify elements.

void demo_iteration() {
    std::cout << "\n--- Section 7: Iteration Styles ---\n";

    std::vector<int> v = {10, 20, 30, 40, 50};

    // Range-for (read-only)
    std::cout << "range-for:    ";
    for (const auto& x : v) std::cout << x << " ";
    std::cout << "\n";

    // Iterator loop (forward)
    std::cout << "iterator:     ";
    for (auto it = v.begin(); it != v.end(); ++it) std::cout << *it << " ";
    std::cout << "\n";

    // Reverse iterator
    std::cout << "reverse:      ";
    for (auto it = v.rbegin(); it != v.rend(); ++it) std::cout << *it << " ";
    std::cout << "\n";

    // Erase-during-iteration (must use iterator, not range-for)
    auto it2 = v.begin();
    while (it2 != v.end()) {
        if (*it2 == 30) {
            it2 = v.erase(it2);  // erase returns next valid iterator
        } else {
            ++it2;
        }
    }
    print_container(v, "after erasing 30");
}

// =============================================================================
// SECTION 8: Container Adapters — stack, queue, priority_queue
// =============================================================================
//
// Container adapters are wrappers that restrict the interface of an underlying
// container (vector, deque, list) to enforce a specific access pattern.
//
// std::stack  — LIFO (Last In, First Out); push/pop/top
// std::queue  — FIFO (First In, First Out); push/pop/front/back
// std::priority_queue — max-heap by default; push/pop/top

void demo_adapters() {
    std::cout << "\n--- Section 8: Container Adapters ---\n";

    // Stack (LIFO)
    std::stack<int> stk;
    stk.push(1); stk.push(2); stk.push(3);
    std::cout << "stack (LIFO): ";
    while (!stk.empty()) { std::cout << stk.top() << " "; stk.pop(); }
    std::cout << "\n";

    // Queue (FIFO)
    std::queue<std::string> q;
    q.push("first"); q.push("second"); q.push("third");
    std::cout << "queue (FIFO): ";
    while (!q.empty()) { std::cout << q.front() << " "; q.pop(); }
    std::cout << "\n";

    // Priority queue (max-heap by default)
    std::priority_queue<int> pq;
    for (int n : {3, 1, 4, 1, 5, 9, 2, 6}) pq.push(n);
    std::cout << "priority_queue (max-heap): ";
    while (!pq.empty()) { std::cout << pq.top() << " "; pq.pop(); }
    std::cout << "\n";

    // Min-heap: use greater<int>
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
    for (int n : {3, 1, 4, 1, 5}) min_pq.push(n);
    std::cout << "priority_queue (min-heap): ";
    while (!min_pq.empty()) { std::cout << min_pq.top() << " "; min_pq.pop(); }
    std::cout << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 14: STL Containers ===\n";

    demo_vector();
    demo_deque();
    demo_list();
    demo_map();
    demo_unordered_map();
    demo_sets();
    demo_iteration();
    demo_adapters();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Prefer emplace_back over push_back for expensive-to-copy\n"
              << "   types — it constructs directly in the container.\n";
    std::cout << "2. map::operator[] inserts a default value on miss; use\n"
              << "   find() for lookup when you don't want accidental inserts.\n";
    std::cout << "3. Never erase from a vector/deque inside a range-for;\n"
              << "   use the iterator-and-erase idiom instead.\n";
    std::cout << "4. unordered_map gives O(1) average but requires a good hash;\n"
              << "   worst case is O(n) on pathological inputs.\n";
    std::cout << "5. Container adapters (stack/queue/pq) do not expose iterators\n"
              << "   — by design.  Use the underlying container directly if you\n"
              << "   need to iterate.\n";

    return 0;
}

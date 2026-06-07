// =============================================================================
// Exercise 15: STL (Standard Template Library) Algorithms
// =============================================================================
// Topics: sort/stable_sort, find/find_if/count_if, transform, accumulate/reduce,
//         copy/copy_if, for_each, lower_bound/upper_bound/binary_search,
//         unique+erase, reverse/rotate/shuffle, iterator categories
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g stl_algorithms.cpp -o stl_algorithms
// =============================================================================

#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <numeric>
#include <functional>
#include <random>
#include <iterator>
#include <string>

// Helper: print a vector
template <typename T>
void print_vec(const std::vector<T>& v, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& x : v) std::cout << x << " ";
    std::cout << "]\n";
}

// =============================================================================
// SECTION 1: std::sort and std::stable_sort
// =============================================================================
//
// sort is O(n log n) introsort; it does NOT preserve relative order of equals.
// stable_sort is O(n log n) with a guarantee that equal elements keep their
// original relative order — at the cost of more memory or time.
//
// Both accept an optional comparator: any callable (lambda, functor, function)
// that satisfies strict weak ordering (irreflexive, asymmetric, transitive).

struct Student {
    std::string name;
    int         grade;
};

void demo_sort() {
    std::cout << "\n--- Section 1: sort and stable_sort ---\n";

    std::vector<int> v = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    std::sort(v.begin(), v.end());
    print_vec(v, "sorted ascending");

    std::sort(v.begin(), v.end(), std::greater<int>());
    print_vec(v, "sorted descending");

    // sort with lambda comparator
    std::vector<std::string> words = {"banana", "apple", "cherry", "date"};
    std::sort(words.begin(), words.end(),
              [](const std::string& a, const std::string& b) {
                  return a.size() < b.size();   // sort by length
              });
    print_vec(words, "sorted by length");

    // stable_sort preserves relative order of students with the same grade
    std::vector<Student> students = {
        {"Alice",   90}, {"Bob", 85}, {"Charlie", 90},
        {"Diana",   85}, {"Eve", 92}
    };
    std::stable_sort(students.begin(), students.end(),
                     [](const Student& a, const Student& b) {
                         return a.grade > b.grade;
                     });
    std::cout << "stable_sort by grade (desc):\n";
    for (const auto& s : students) {
        std::cout << "  " << s.name << " " << s.grade << "\n";
    }
}

// =============================================================================
// SECTION 2: find, find_if, count_if
// =============================================================================
//
// find: linear search for a value — O(n); returns iterator to first match or end().
// find_if: linear search with a predicate.
// count_if: counts elements satisfying a predicate.

void demo_find_count() {
    std::cout << "\n--- Section 2: find, find_if, count_if ---\n";

    std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};

    auto it = std::find(v.begin(), v.end(), 9);
    if (it != v.end()) {
        std::cout << "Found 9 at index " << std::distance(v.begin(), it) << "\n";
    }

    auto it2 = std::find_if(v.begin(), v.end(),
                            [](int x) { return x > 7; });
    if (it2 != v.end()) {
        std::cout << "First element > 7: " << *it2 << "\n";
    }

    int evens = static_cast<int>(
        std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }));
    std::cout << "Count of even numbers: " << evens << "\n";

    // find_if_not: first element not satisfying predicate
    auto it3 = std::find_if_not(v.begin(), v.end(), [](int x){ return x < 5; });
    std::cout << "First element >= 5: " << *it3 << "\n";
}

// =============================================================================
// SECTION 3: std::transform — Map Over a Range
// =============================================================================
//
// transform applies a unary or binary function to each element and writes the
// result into an output range (which may be the same range — in-place is safe).

void demo_transform() {
    std::cout << "\n--- Section 3: std::transform ---\n";

    std::vector<int> v = {1, 2, 3, 4, 5};

    // Unary: square each element in-place
    std::transform(v.begin(), v.end(), v.begin(),
                   [](int x) { return x * x; });
    print_vec(v, "squared");

    // Write to a different output range
    std::vector<std::string> labels;
    std::transform(v.begin(), v.end(), std::back_inserter(labels),
                   [](int x) { return "val=" + std::to_string(x); });
    std::cout << "labels: [ ";
    for (const auto& s : labels) std::cout << s << " ";
    std::cout << "]\n";

    // Binary: element-wise sum of two vectors
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {10, 20, 30};
    std::vector<int> sums(3);
    std::transform(a.begin(), a.end(), b.begin(), sums.begin(),
                   [](int x, int y) { return x + y; });
    print_vec(sums, "element-wise sum");
}

// =============================================================================
// SECTION 4: std::accumulate and std::reduce
// =============================================================================
//
// accumulate: left fold with an initial value — sequential, guaranteed order.
// reduce (C++17): like accumulate but may be executed in any order (supports
// parallel execution policies).  For non-commutative operations, use accumulate.

void demo_accumulate_reduce() {
    std::cout << "\n--- Section 4: accumulate and reduce ---\n";

    std::vector<int> v = {1, 2, 3, 4, 5};

    int sum = std::accumulate(v.begin(), v.end(), 0);
    std::cout << "sum = " << sum << "\n";

    int product = std::accumulate(v.begin(), v.end(), 1,
                                  [](int acc, int x) { return acc * x; });
    std::cout << "product = " << product << "\n";

    // reduce: same as accumulate for + on ints (any order is fine)
    int rsum = std::reduce(v.begin(), v.end(), 0, std::plus<int>());
    std::cout << "reduce sum = " << rsum << "\n";

    // inner_product: dot product
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b = {4, 5, 6};
    int dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);
    std::cout << "dot product = " << dot << "\n";  // 1*4+2*5+3*6 = 32
}

// =============================================================================
// SECTION 5: std::copy and std::copy_if (Filter)
// =============================================================================
//
// copy: copies a range into an output iterator.
// copy_if: copies only elements satisfying a predicate — a "filter" operation.
// back_inserter creates an output iterator that calls push_back on the container.

void demo_copy() {
    std::cout << "\n--- Section 5: copy and copy_if ---\n";

    std::vector<int> src = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> dst;

    // Plain copy
    std::copy(src.begin(), src.begin() + 5, std::back_inserter(dst));
    print_vec(dst, "copy first 5");

    // copy_if: keep only odds
    std::vector<int> odds;
    std::copy_if(src.begin(), src.end(), std::back_inserter(odds),
                 [](int x) { return x % 2 != 0; });
    print_vec(odds, "odds only");

    // copy to output stream iterator (print to cout)
    std::cout << "copy to ostream_iterator: ";
    std::copy(src.begin(), src.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << "\n";
}

// =============================================================================
// SECTION 6: std::for_each
// =============================================================================
//
// for_each applies a function to each element.  Unlike transform, it ignores
// the return value of the function (it's for side-effects only).
// The function is returned, which is useful for stateful functors.

void demo_for_each() {
    std::cout << "\n--- Section 6: std::for_each ---\n";

    std::vector<int> v = {3, 1, 4, 1, 5, 9};

    // Stateful lambda: capture a sum by reference
    int sum = 0;
    std::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
    std::cout << "sum via for_each = " << sum << "\n";

    // Modify elements in-place
    std::for_each(v.begin(), v.end(), [](int& x) { x *= 2; });
    print_vec(v, "doubled via for_each");
}

// =============================================================================
// SECTION 7: Binary Search — lower_bound, upper_bound, binary_search
// =============================================================================
//
// These algorithms require the range to be sorted (or at least partitioned
// with respect to the comparator).
//
// lower_bound: first element NOT LESS THAN value — O(log n).
// upper_bound: first element GREATER THAN value  — O(log n).
// binary_search: true/false whether value exists — O(log n).

void demo_binary_search() {
    std::cout << "\n--- Section 7: lower_bound, upper_bound, binary_search ---\n";

    std::vector<int> v = {1, 2, 4, 4, 4, 7, 9, 12};
    print_vec(v, "sorted input");

    auto lb = std::lower_bound(v.begin(), v.end(), 4);
    auto ub = std::upper_bound(v.begin(), v.end(), 4);
    std::cout << "lower_bound(4) index = " << std::distance(v.begin(), lb) << "\n";
    std::cout << "upper_bound(4) index = " << std::distance(v.begin(), ub) << "\n";
    std::cout << "count of 4s = " << std::distance(lb, ub) << "\n";

    std::cout << "binary_search(7):  " << (std::binary_search(v.begin(), v.end(), 7) ? "found" : "not found") << "\n";
    std::cout << "binary_search(5):  " << (std::binary_search(v.begin(), v.end(), 5) ? "found" : "not found") << "\n";

    // equal_range returns a pair [lower_bound, upper_bound]
    auto [lo, hi] = std::equal_range(v.begin(), v.end(), 4);
    std::cout << "equal_range(4): [" << std::distance(v.begin(), lo)
              << ", " << std::distance(v.begin(), hi) << ")\n";
}

// =============================================================================
// SECTION 8: unique+erase, reverse, rotate, shuffle
// =============================================================================
//
// unique: moves duplicate consecutive elements to the end and returns an
// iterator to the new logical end.  The range must be SORTED first.
// Pair with erase to physically remove the duplicates.
//
// rotate: rotates elements so that a given element becomes the new front.
// shuffle: randomly permutes using a Mersenne Twister.

void demo_reorder() {
    std::cout << "\n--- Section 8: unique+erase, reverse, rotate, shuffle ---\n";

    // unique + erase
    std::vector<int> v = {3, 1, 1, 4, 2, 2, 2, 5, 3};
    std::sort(v.begin(), v.end());
    auto new_end = std::unique(v.begin(), v.end());
    v.erase(new_end, v.end());
    print_vec(v, "sorted+unique");

    // reverse
    std::reverse(v.begin(), v.end());
    print_vec(v, "reversed");

    // rotate: bring the 3rd element to front
    std::rotate(v.begin(), v.begin() + 2, v.end());
    print_vec(v, "rotated by 2");

    // shuffle
    std::mt19937 rng(42);
    std::shuffle(v.begin(), v.end(), rng);
    print_vec(v, "shuffled");

    // is_sorted
    std::sort(v.begin(), v.end());
    std::cout << "is_sorted after sort: "
              << (std::is_sorted(v.begin(), v.end()) ? "yes" : "no") << "\n";
}

// =============================================================================
// SECTION 9: Iterator Categories
// =============================================================================
//
// STL algorithms express their requirements in terms of iterator categories:
//
//  InputIterator        — single-pass read (istream_iterator, forward_list)
//  OutputIterator       — single-pass write (ostream_iterator, back_inserter)
//  ForwardIterator      — multi-pass read/write (singly-linked forward_list)
//  BidirectionalIterator— + decrement (list, set, map)
//  RandomAccessIterator — + O(1) jump (vector, deque, array, pointers)
//
// The category determines which algorithms can use the container efficiently.
// distance() is O(n) for InputIterator but O(1) for RandomAccessIterator.
// sort() requires RandomAccessIterator; list has its own member sort().

void demo_iterator_categories() {
    std::cout << "\n--- Section 9: Iterator Categories ---\n";

    // Random-access: vector — sort, subscript, O(1) advance
    std::vector<int> vec = {3, 1, 2};
    std::sort(vec.begin(), vec.end());   // OK: random-access
    std::cout << "vector (RandomAccess): sorted = [ ";
    for (int x : vec) std::cout << x << " ";
    std::cout << "]\n";

    // Bidirectional: list — NO sort; use member list::sort()
    std::list<int> lst = {3, 1, 2};
    lst.sort();   // member sort, works on bidirectional iterators
    std::cout << "list   (Bidirectional): sorted = [ ";
    for (int x : lst) std::cout << x << " ";
    std::cout << "]\n";

    // distance: O(1) for random-access, O(n) for bidirectional
    auto vi = vec.begin();
    auto li = lst.begin();
    std::advance(vi, 2);  // O(1) for vector
    std::advance(li, 2);  // O(n) for list
    std::cout << "vec after advance(2): " << *vi << "\n";
    std::cout << "lst after advance(2): " << *li << "\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 15: STL Algorithms ===\n";

    demo_sort();
    demo_find_count();
    demo_transform();
    demo_accumulate_reduce();
    demo_copy();
    demo_for_each();
    demo_binary_search();
    demo_reorder();
    demo_iterator_categories();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. std::sort requires RandomAccessIterators; for std::list\n"
              << "   use the member sort() instead.\n";
    std::cout << "2. std::unique only removes consecutive duplicates — sort\n"
              << "   first, then unique+erase.\n";
    std::cout << "3. lower_bound/upper_bound/binary_search require a sorted\n"
              << "   (or appropriately partitioned) range.\n";
    std::cout << "4. std::reduce (C++17) may reorder operations; for non-\n"
              << "   commutative operations use accumulate instead.\n";
    std::cout << "5. Algorithms write to OUTPUT iterators — ensure the\n"
              << "   destination range is large enough, or use back_inserter.\n";

    return 0;
}

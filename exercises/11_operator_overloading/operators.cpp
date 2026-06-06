// operators.cpp - Exercise 11: Operator Overloading
// Demonstrates: arithmetic, compound assignment, comparison, stream insertion,
//               subscript, increment/decrement operators; pitfalls

#include <iostream>
#include <string>
#include <cmath>      // std::sqrt, std::abs
#include <stdexcept>  // std::out_of_range

// ── Vec2: arithmetic, compound assignment, comparison, stream ─────────────────

class Vec2 {
public:
    double x;
    double y;

    // Constructors
    Vec2() : x(0.0), y(0.0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    // ── Arithmetic operators (non-member friends are idiomatic) ───────────────

    // operator+: returns a new Vec2 (does not modify either operand)
    friend Vec2 operator+(const Vec2& a, const Vec2& b)
    {
        return Vec2(a.x + b.x, a.y + b.y);
    }

    // operator-: subtraction
    friend Vec2 operator-(const Vec2& a, const Vec2& b)
    {
        return Vec2(a.x - b.x, a.y - b.y);
    }

    // operator*: scalar multiplication (Vec2 * scalar)
    friend Vec2 operator*(const Vec2& v, double s)
    {
        return Vec2(v.x * s, v.y * s);
    }

    // operator*: scalar multiplication (scalar * Vec2) — commutative
    friend Vec2 operator*(double s, const Vec2& v)
    {
        return v * s;
    }

    // Unary minus
    Vec2 operator-() const
    {
        return Vec2(-x, -y);
    }

    // ── Compound assignment (member, modifies *this) ───────────────────────────

    Vec2& operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& operator*=(double s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    // ── Comparison operators ───────────────────────────────────────────────────

    // operator==: equality by value
    friend bool operator==(const Vec2& a, const Vec2& b)
    {
        // Note: comparing doubles exactly; for floating-point, consider epsilon
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(const Vec2& a, const Vec2& b)
    {
        return !(a == b);
    }

    // operator<: lexicographic order (x first, then y) — arbitrary but consistent
    friend bool operator<(const Vec2& a, const Vec2& b)
    {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    }

    // ── Stream insertion (friend, gives access to private if needed) ──────────

    friend std::ostream& operator<<(std::ostream& os, const Vec2& v)
    {
        os << "Vec2(" << v.x << ", " << v.y << ")";
        return os;    // return os enables chaining: cout << a << b
    }

    // ── Utility methods ────────────────────────────────────────────────────────

    double length() const { return std::sqrt(x*x + y*y); }
    double dot(const Vec2& other) const { return x*other.x + y*other.y; }
};

// ── Array wrapper: subscript operator and prefix/postfix increment ─────────────

class IntArray {
public:
    explicit IntArray(int size)
        : size_(size), data_(new int[static_cast<std::size_t>(size)]())
    {
        std::cout << "  [IntArray] ctor size=" << size_ << "\n";
    }

    ~IntArray()
    {
        std::cout << "  [IntArray] dtor\n";
        delete[] data_;
    }

    // Non-const subscript: allows assignment (arr[i] = 42)
    int& operator[](int idx)
    {
        if (idx < 0 || idx >= size_)
            throw std::out_of_range("IntArray::operator[] out of range");
        return data_[idx];
    }

    // Const subscript: read-only access
    const int& operator[](int idx) const
    {
        if (idx < 0 || idx >= size_)
            throw std::out_of_range("IntArray::operator[] const out of range");
        return data_[idx];
    }

    int size() const { return size_; }

    void print() const
    {
        std::cout << "  IntArray[";
        for (int i = 0; i < size_; ++i) {
            std::cout << data_[i];
            if (i < size_ - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    // Copy constructor (deleted for simplicity)
    IntArray(const IntArray&)            = delete;
    IntArray& operator=(const IntArray&) = delete;

private:
    int  size_;
    int* data_;
};

// ── Counter: prefix and postfix increment/decrement ──────────────────────────

class Counter {
public:
    explicit Counter(int start = 0) : val_(start) {}

    int value() const { return val_; }

    // Prefix ++counter: increment then return new value (by reference)
    Counter& operator++()
    {
        ++val_;
        return *this;
    }

    // Postfix counter++: return old value (by value), then increment
    // The dummy int parameter distinguishes it from prefix.
    Counter operator++(int)
    {
        Counter old = *this;   // save current state
        ++val_;
        return old;            // return the old state
    }

    // Prefix --counter
    Counter& operator--()
    {
        --val_;
        return *this;
    }

    // Postfix counter--
    Counter operator--(int)
    {
        Counter old = *this;
        --val_;
        return old;
    }

    friend std::ostream& operator<<(std::ostream& os, const Counter& c)
    {
        return os << "Counter(" << c.val_ << ")";
    }

private:
    int val_;
};

// ── main ───────────────────────────────────────────────────────────────────────

int main()
{
    // Section 1: Arithmetic operators
    std::cout << "=== Section 1: Arithmetic Operators (Vec2) ===\n";
    Vec2 a(1.0, 2.0);
    Vec2 b(3.0, 4.0);
    std::cout << "a = " << a << "\n";
    std::cout << "b = " << b << "\n";
    std::cout << "a + b = " << (a + b) << "\n";
    std::cout << "b - a = " << (b - a) << "\n";
    std::cout << "a * 3 = " << (a * 3.0) << "\n";
    std::cout << "2 * b = " << (2.0 * b) << "\n";
    std::cout << "-a    = " << (-a)       << "\n";
    std::cout << "a.length() = " << a.length() << "\n";
    std::cout << "a.dot(b)   = " << a.dot(b)   << "\n";

    // Section 2: Compound assignment
    std::cout << "\n=== Section 2: Compound Assignment ===\n";
    Vec2 v(1.0, 1.0);
    std::cout << "v = " << v << "\n";
    v += Vec2(2.0, 3.0);
    std::cout << "v += (2,3): " << v << "\n";
    v -= Vec2(0.5, 0.5);
    std::cout << "v -= (0.5,0.5): " << v << "\n";
    v *= 2.0;
    std::cout << "v *= 2: " << v << "\n";

    // Section 3: Comparison
    std::cout << "\n=== Section 3: Comparison Operators ===\n";
    Vec2 p(1.0, 2.0);
    Vec2 q(1.0, 2.0);
    Vec2 r(3.0, 4.0);
    std::cout << "p = " << p << "\n";
    std::cout << "q = " << q << "\n";
    std::cout << "r = " << r << "\n";
    std::cout << "p == q: " << std::boolalpha << (p == q) << "\n";
    std::cout << "p != r: " << (p != r) << "\n";
    std::cout << "p <  r: " << (p < r)  << "\n";
    std::cout << "r <  p: " << (r < p)  << "\n";

    // Section 4: Stream insertion (operator<<)
    std::cout << "\n=== Section 4: Stream Insertion (operator<<) ===\n";
    Vec2 chain1(5.0, 6.0);
    Vec2 chain2(7.0, 8.0);
    // Chaining works because operator<< returns os&
    std::cout << chain1 << "  and  " << chain2 << "\n";

    // Section 5: Subscript operator
    std::cout << "\n=== Section 5: Subscript operator[] ===\n";
    {
        IntArray arr(5);
        for (int i = 0; i < arr.size(); ++i)
            arr[i] = (i + 1) * 10;
        arr.print();

        // Const access
        const IntArray& carr = arr;
        std::cout << "  carr[2] = " << carr[2] << "\n";

        // Bounds check
        try {
            arr[10] = 0;
        } catch (const std::out_of_range& e) {
            std::cout << "  arr[10] threw: " << e.what() << "\n";
        }
    }

    // Section 6: Increment and Decrement
    std::cout << "\n=== Section 6: Prefix and Postfix ++/-- ===\n";
    Counter c(5);
    std::cout << "c = " << c << "\n";

    Counter pre_result = ++c;   // c becomes 6; pre_result is 6
    std::cout << "++c -> c=" << c << "  returned=" << pre_result << "\n";

    Counter post_result = c++;  // post_result is 6 (old); c becomes 7
    std::cout << "c++ -> c=" << c << "  returned=" << post_result << "\n";

    --c;
    std::cout << "--c -> c=" << c << "\n";

    Counter old = c--;
    std::cout << "c-- -> c=" << c << "  old=" << old << "\n";

    // Section 7: When NOT to overload
    std::cout << "\n=== Section 7: When NOT to Overload ===\n";
    std::cout << "Pitfalls:\n";
    std::cout << "  1. Don't overload operator&& or operator|| — they lose short-circuit.\n";
    std::cout << "  2. Don't overload operator, (comma) — confusing evaluation order.\n";
    std::cout << "  3. Don't overload operators to do something unexpected\n";
    std::cout << "     (e.g., operator+ doing subtraction).\n";
    std::cout << "  4. Don't overload operators for types you don't own.\n";
    std::cout << "  5. operator[] returning a ref allows both get and set;\n";
    std::cout << "     provide a const overload too.\n";
    std::cout << "  6. Prefix ++ should return a reference (*this).\n";
    std::cout << "     Postfix ++ must return by value (old copy).\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - operator+ is non-member (friend) so it works for a+b and b+a.\n";
    std::cout << "  - Compound assignment (+=) modifies *this and returns *this&.\n";
    std::cout << "  - operator<< returns ostream& to allow chaining.\n";
    std::cout << "  - For ordering, provide operator< and derive others from it\n";
    std::cout << "    (or use C++20 spaceship operator <=>).\n";

    return 0;
}

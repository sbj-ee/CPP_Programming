// classes.cpp - Exercise 07: Classes
// Demonstrates: class vs struct, constructors, destructor, member functions,
//               const members, this pointer, access specifiers, getters/setters

#include <iostream>
#include <string>
#include <cmath>    // std::sqrt

// ── Section 1: struct vs class (default access) ───────────────────────────────

// struct: all members are public by default.
struct Point {
    double x;
    double y;

    // Member function inside struct
    double distance_to(const Point& other) const
    {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx*dx + dy*dy);
    }

    void print() const
    {
        std::cout << "(" << x << ", " << y << ")";
    }
};

// ── Section 2: Class with constructors, destructor, access specifiers ─────────

class Rectangle {
public:
    // Default constructor
    Rectangle()
        : width_(0.0), height_(0.0)
    {
        std::cout << "  [Rectangle] default ctor: 0x0\n";
    }

    // Parameterised constructor
    Rectangle(double w, double h)
        : width_(w), height_(h)
    {
        std::cout << "  [Rectangle] param ctor: " << w << "x" << h << "\n";
    }

    // Delegating constructor (C++11): delegates to parameterised ctor
    explicit Rectangle(double side)
        : Rectangle(side, side)    // squares delegate to the 2-arg ctor
    {
        std::cout << "  [Rectangle] delegating ctor (square side=" << side << ")\n";
    }

    // Copy constructor (compiler-generated would do the same here)
    Rectangle(const Rectangle& other)
        : width_(other.width_), height_(other.height_)
    {
        std::cout << "  [Rectangle] copy ctor\n";
    }

    // Destructor
    ~Rectangle()
    {
        std::cout << "  [Rectangle] dtor: " << width_ << "x" << height_ << "\n";
    }

    // ── Getters (const member functions) ──────────────────────────────────────
    // const after () means: this function does not modify any data members.
    double width()  const { return width_;  }
    double height() const { return height_; }
    double area()   const { return width_ * height_; }
    double perimeter() const { return 2.0 * (width_ + height_); }

    // ── Setters ───────────────────────────────────────────────────────────────
    void set_width(double w)
    {
        if (w < 0) {
            std::cerr << "  Warning: negative width ignored\n";
            return;
        }
        width_ = w;
    }
    void set_height(double h)
    {
        if (h < 0) {
            std::cerr << "  Warning: negative height ignored\n";
            return;
        }
        height_ = h;
    }

    // Member function using 'this' pointer explicitly
    Rectangle& scale(double factor)
    {
        this->width_  *= factor;
        this->height_ *= factor;
        return *this;   // return *this enables chaining: r.scale(2).scale(3)
    }

    void print() const
    {
        std::cout << "Rectangle(" << width_ << " x " << height_
                  << ")  area=" << area()
                  << "  perimeter=" << perimeter() << "\n";
    }

protected:
    // protected: accessible by this class and derived classes
    double diagonal() const
    {
        return std::sqrt(width_*width_ + height_*height_);
    }

private:
    // private: accessible only within this class
    double width_;
    double height_;
};

// ── Section 3: A simple Counter class ────────────────────────────────────────

class Counter {
public:
    explicit Counter(int start = 0) : value_(start)
    {
        std::cout << "  [Counter] ctor start=" << start << "\n";
    }

    ~Counter()
    {
        std::cout << "  [Counter] dtor value=" << value_ << "\n";
    }

    void increment(int by = 1) { value_ += by; }
    void decrement(int by = 1) { value_ -= by; }
    void reset()               { value_ = 0;   }

    int  value()  const { return value_; }
    bool is_zero() const { return value_ == 0; }

    void print() const
    {
        std::cout << "Counter: " << value_ << "\n";
    }

private:
    int value_;
};

// ── Section 4: Access specifiers in a derived class (protected demo) ──────────

class Square : public Rectangle {
public:
    explicit Square(double side) : Rectangle(side, side) {}

    double diagonal_length() const
    {
        return diagonal();   // calls protected member from base
    }
};

// ── main ───────────────────────────────────────────────────────────────────────

int main()
{
    // Section 1: struct / Point
    std::cout << "=== Section 1: struct vs class (default access) ===\n";
    Point p1 = {0.0, 0.0};
    Point p2 = {3.0, 4.0};
    p1.print(); std::cout << "  to  "; p2.print();
    std::cout << "  distance = " << p1.distance_to(p2) << "\n";

    // Designated initializers (C++20 for structs, but aggregate init is C++11)
    Point origin{};    // zero-initialized
    std::cout << "origin = "; origin.print(); std::cout << "\n";

    // Section 2: Rectangle
    std::cout << "\n=== Section 2: Rectangle class ===\n";
    {
        Rectangle r1;                     // default ctor
        Rectangle r2(6.0, 4.0);           // param ctor
        Rectangle r3(5.0);                // delegating ctor (square)
        Rectangle r4 = r2;               // copy ctor

        r2.print();
        r2.set_width(8.0);
        r2.print();

        // Chained method calls via *this
        r3.scale(2.0).scale(1.5);
        r3.print();

        // const correctness: can call const methods on const object
        const Rectangle cr(10.0, 3.0);
        std::cout << "const rect area=" << cr.area() << "\n";
        // cr.set_width(5); // error: cannot call non-const on const object
    } // r1, r2, r3, r4, cr all destroyed here (dtors print)

    // Section 3: Counter
    std::cout << "\n=== Section 3: Counter class ===\n";
    {
        Counter c(10);
        c.print();
        c.increment(5);
        c.print();
        c.decrement(3);
        c.print();
        c.reset();
        std::cout << "is_zero=" << std::boolalpha << c.is_zero() << "\n";
    } // dtor prints

    // Section 4: protected access through derived class
    std::cout << "\n=== Section 4: Protected Members and Derived Class ===\n";
    {
        Square sq(5.0);
        sq.print();
        std::cout << "Square diagonal = " << sq.diagonal_length() << "\n";
    }

    // Section 5: 'this' pointer
    std::cout << "\n=== Section 5: 'this' Pointer ===\n";
    {
        Rectangle r(3.0, 2.0);
        r.print();
        r.scale(3.0).scale(0.5);    // chaining via return *this
        r.print();
    }

    // Section 6: class vs struct summary
    std::cout << "\n=== Section 6: class vs struct Summary ===\n";
    std::cout << "  struct: members public by default; use for plain data.\n";
    std::cout << "  class : members private by default; use for encapsulated logic.\n";
    std::cout << "  Both support member functions, constructors, destructors.\n";
    std::cout << "  Constructors initialise members in the initialiser list (: m(v)).\n";
    std::cout << "  Destructor (~ClassName) is called when object goes out of scope.\n";
    std::cout << "  const member functions cannot modify data members.\n";
    std::cout << "  this pointer is the address of the object; useful for chaining.\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Initialiser list (: m(v)) is faster than assignment in body.\n";
    std::cout << "  - Use 'explicit' on single-arg ctors to prevent implicit conversion.\n";
    std::cout << "  - protected lets derived classes access members, not the public.\n";
    std::cout << "  - Return *this from modifiers to enable method chaining.\n";

    return 0;
}

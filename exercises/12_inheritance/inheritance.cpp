// =============================================================================
// Exercise 12: Inheritance and Polymorphism
// =============================================================================
// Topics: Base/derived classes, virtual functions, override/final, abstract
//         base classes, polymorphism, dynamic_cast, typeid, virtual destructor,
//         object slicing
//
// Build: g++ -Wall -Wextra -Wpedantic -std=c++17 -g inheritance.cpp -o inheritance
// =============================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <typeinfo>
#include <cmath>
#include <string>

// =============================================================================
// SECTION 1: Abstract Base Class and Virtual Functions
// =============================================================================
//
// A class with at least one pure virtual function is abstract — it cannot be
// instantiated directly.  Every concrete derived class MUST override all pure
// virtuals, otherwise it too becomes abstract.
//
// Virtual destructor rule: if a class is meant to be used polymorphically
// (i.e., deleted through a base-class pointer), the destructor MUST be virtual.
// Without it, deleting through Shape* calls only Shape::~Shape(), leaking
// any resources owned by the derived object.

static constexpr double PI = 3.14159265358979323846;

class Shape {
public:
    // Pure virtual functions — must be overridden by derived classes
    virtual double area()      const = 0;
    virtual double perimeter() const = 0;

    // Non-pure virtual — provides a default implementation but can be overridden
    virtual std::string describe() const {
        return "I am a Shape with area=" + std::to_string(area())
             + " perimeter=" + std::to_string(perimeter());
    }

    // Virtual destructor: ESSENTIAL for safe polymorphic deletion
    virtual ~Shape() {
        std::cout << "  ~Shape() destructor called\n";
    }

    // Non-virtual utility — not subject to dynamic dispatch
    void print_info() const {
        std::cout << describe() << "\n";
    }
};

// =============================================================================
// SECTION 2: Derived Classes with override and final
// =============================================================================
//
// 'override' tells the compiler "this must match a base virtual exactly".
// If you mis-spell the name or change the signature, the compiler errors out.
//
// 'final' on a class prevents further derivation.
// 'final' on a method prevents further overriding.

class Circle : public Shape {
public:
    explicit Circle(double radius) : radius_(radius) {}

    double area()      const override { return PI * radius_ * radius_; }
    double perimeter() const override { return 2.0 * PI * radius_; }

    std::string describe() const override {
        return "Circle(r=" + std::to_string(radius_) + ")"
             + " area=" + std::to_string(area());
    }

    double radius() const { return radius_; }

    ~Circle() override {
        std::cout << "  ~Circle() destructor called\n";
    }

private:
    double radius_;
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : width_(w), height_(h) {}

    double area()      const override { return width_ * height_; }
    double perimeter() const override { return 2.0 * (width_ + height_); }

    std::string describe() const override {
        return "Rectangle(" + std::to_string(width_) + "x"
             + std::to_string(height_) + ") area=" + std::to_string(area());
    }

    ~Rectangle() override {
        std::cout << "  ~Rectangle() destructor called\n";
    }

private:
    double width_, height_;
};

// 'final' class — cannot be further derived
class Triangle final : public Shape {
public:
    Triangle(double a, double b, double c) : a_(a), b_(b), c_(c) {}

    double perimeter() const override { return a_ + b_ + c_; }

    // Heron's formula
    double area() const override {
        double s = perimeter() / 2.0;
        return std::sqrt(s * (s - a_) * (s - b_) * (s - c_));
    }

    std::string describe() const override {
        return "Triangle(" + std::to_string(a_) + ","
             + std::to_string(b_) + "," + std::to_string(c_)
             + ") area=" + std::to_string(area());
    }

    ~Triangle() override {
        std::cout << "  ~Triangle() destructor called\n";
    }

private:
    double a_, b_, c_;
};

// Attempting to derive from Triangle is a compile error because it is 'final':
//   class SpecialTriangle : public Triangle {};  // ERROR

// =============================================================================
// SECTION 3: Polymorphism via Shape* and Shape&
// =============================================================================
//
// The key insight: through a base-class pointer/reference, the virtual dispatch
// mechanism calls the correct overridden function at runtime.

void demo_polymorphism() {
    std::cout << "\n--- Section 3: Polymorphism via Shape* / Shape& ---\n";

    // Store heterogeneous shapes through base-class smart pointers
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5.0));
    shapes.push_back(std::make_unique<Rectangle>(4.0, 6.0));
    shapes.push_back(std::make_unique<Triangle>(3.0, 4.0, 5.0));

    double total_area = 0.0;
    for (const auto& s : shapes) {
        s->print_info();          // virtual dispatch: calls correct describe()
        total_area += s->area();  // virtual dispatch: calls correct area()
    }
    std::cout << "Total area of all shapes: " << total_area << "\n";

    // Polymorphism via reference
    auto process = [](const Shape& sh) {
        std::cout << "  via ref -> " << sh.describe() << "\n";
    };
    Circle c(2.0);
    Rectangle r(3.0, 3.0);
    process(c);
    process(r);
}

// =============================================================================
// SECTION 4: dynamic_cast and typeid
// =============================================================================
//
// dynamic_cast performs a runtime type check.  It is only useful in class
// hierarchies that have at least one virtual function (the vtable enables RTTI).
//
// Pointer form: returns nullptr on failure (safe to check).
// Reference form: throws std::bad_cast on failure.
//
// typeid(expr) returns a std::type_info object; .name() gives a mangled name.
// Use typeid only when you genuinely need to know the concrete type; prefer
// virtual dispatch for type-dependent behaviour.

void demo_dynamic_cast() {
    std::cout << "\n--- Section 4: dynamic_cast and typeid ---\n";

    std::unique_ptr<Shape> s1 = std::make_unique<Circle>(3.0);
    std::unique_ptr<Shape> s2 = std::make_unique<Rectangle>(2.0, 5.0);

    // Pointer dynamic_cast
    Circle* cp = dynamic_cast<Circle*>(s1.get());
    if (cp != nullptr) {
        std::cout << "s1 is a Circle with radius " << cp->radius() << "\n";
    }

    Rectangle* rp = dynamic_cast<Rectangle*>(s1.get());
    if (rp == nullptr) {
        std::cout << "s1 is NOT a Rectangle (dynamic_cast returned nullptr)\n";
    }

    // typeid — reports runtime type
    std::cout << "typeid of s1: " << typeid(*s1).name() << "\n";
    std::cout << "typeid of s2: " << typeid(*s2).name() << "\n";
    std::cout << "Are they the same type? "
              << (typeid(*s1) == typeid(*s2) ? "yes" : "no") << "\n";

    // Reference dynamic_cast with exception handling
    try {
        Circle& cr = dynamic_cast<Circle&>(*s2);  // s2 is Rectangle — throws
        (void)cr;
    } catch (const std::bad_cast& e) {
        std::cout << "Caught bad_cast: " << e.what() << "\n";
    }
}

// =============================================================================
// SECTION 5: Virtual Destructor — Why It Matters
// =============================================================================
//
// Without a virtual destructor, deleting a derived object through a base pointer
// has undefined behaviour: only the base destructor runs.

class BadBase {
public:
    ~BadBase() {   // NOT virtual
        std::cout << "  ~BadBase() (non-virtual)\n";
    }
};

class BadDerived : public BadBase {
public:
    ~BadDerived() {
        std::cout << "  ~BadDerived() — this may NEVER be called!\n";
    }
};

void demo_virtual_destructor() {
    std::cout << "\n--- Section 5: Virtual Destructor ---\n";

    std::cout << "Correct polymorphic deletion (Shape has virtual ~Shape):\n";
    {
        Shape* s = new Circle(1.0);
        delete s;  // calls ~Circle() then ~Shape() — correct
    }

    std::cout << "\nDangerous deletion (BadBase has non-virtual destructor):\n";
    {
        BadBase* b = new BadDerived();
        delete b;  // UB: only ~BadBase() called; ~BadDerived() skipped!
    }
    std::cout << "  (Notice ~BadDerived() was never printed above)\n";
}

// =============================================================================
// SECTION 6: Object Slicing Warning
// =============================================================================
//
// When a derived object is assigned to (or passed as) a base-class VALUE,
// the derived-class data members and vtable are "sliced off".  The resulting
// object is a genuine base-class object and virtual dispatch no longer works.
// Always use pointers or references for polymorphism.

void demo_object_slicing() {
    std::cout << "\n--- Section 6: Object Slicing Warning ---\n";

    Circle original(4.0);
    std::cout << "original.describe(): " << original.describe() << "\n";

    // SLICING: Shape is assigned the Circle, but only Shape parts are copied.
    // 'sliced' is now a genuine Shape object — cannot be instantiated because
    // Shape is abstract.  Demonstration with a concrete base instead:

    // We'll illustrate with a simple concrete pair:
    struct Base {
        virtual std::string who() const { return "Base"; }
        int base_val = 1;
    };
    struct Derived : Base {
        std::string who() const override { return "Derived"; }
        int derived_val = 99;
    };

    Derived d;
    Base b_copy = d;  // SLICE: derived_val is lost; vtable pointer is Base's

    std::cout << "d.who()      = " << d.who()      << "  (correct)\n";
    std::cout << "b_copy.who() = " << b_copy.who() << "  (sliced — shows Base!)\n";

    // Safe: use a reference — no slicing
    Base& b_ref = d;
    std::cout << "b_ref.who()  = " << b_ref.who()  << "  (via ref — correct)\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
    std::cout << "=== Exercise 12: Inheritance and Polymorphism ===\n";

    demo_polymorphism();
    demo_dynamic_cast();
    demo_virtual_destructor();
    demo_object_slicing();

    std::cout << "\n--- Notes & Pitfalls ---\n";
    std::cout << "1. Always declare the base destructor virtual if the class\n"
              << "   is polymorphic (has any virtual function).\n";
    std::cout << "2. Use 'override' on every overriding function so the\n"
              << "   compiler verifies the signature matches.\n";
    std::cout << "3. dynamic_cast pointer form returns nullptr on failure;\n"
              << "   check before using.  Reference form throws std::bad_cast.\n";
    std::cout << "4. Object slicing strips derived data when assigning to a\n"
              << "   base-class value.  Always use Shape* or Shape& for\n"
              << "   polymorphism, never Shape by value.\n";
    std::cout << "5. Pure-virtual functions make a class abstract; any class\n"
              << "   that doesn't implement all pure virtuals is also abstract.\n";

    return 0;
}

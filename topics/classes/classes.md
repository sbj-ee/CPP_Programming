# Classes & OOP (Object-Oriented Programming) — Cheat Sheet

## `class` vs `struct`

| | `class` | `struct` |
|-|---------|---------|
| Default member access | `private` | `public` |
| Default inheritance | `private` | `public` |
| Otherwise | identical | identical |

Convention: `struct` for plain data aggregates; `class` for encapsulated types with invariants.

---

## Constructor Types

```cpp
struct Point {
    int x, y;
};

class Widget {
public:
    // Default constructor — pick ONE of these alternatives:
    Widget() : x_(0), y_(0) {}       // user-defined
    // Widget() = default;            // compiler-generated (if no other ctor defined)

    // Parameterised constructor
    Widget(int x, int y) : x_(x), y_(y) {}

    // Delegating constructor (C++11)
    Widget(int n) : Widget(n, n) {}   // delegates to Widget(int,int)

    // Converting constructor — single arg, implicit conversion
    // Pick ONE of these alternatives:
    Widget(int x) : x_(x), y_(0) {}  // allows implicit int→Widget conversion
    // explicit Widget(int x) : x_(x), y_(0) {}  // prevents implicit conversion

    // Copy constructor — pick ONE of these alternatives:
    Widget(const Widget& o) : x_(o.x_), y_(o.y_) {}  // user-defined
    // Widget(const Widget&) = default;  // compiler-generated
    // Widget(const Widget&) = delete;   // forbid copying

    // Move constructor (C++11)
    Widget(Widget&& o) noexcept : x_(o.x_), y_(o.y_) {}

private:
    int x_, y_;
};
```

### Member Initialiser List

```cpp
class Foo {
    const int id_;     // const member — MUST use init list
    std::string name_; // avoids default-then-copy-assign
    int& ref_;         // reference — MUST use init list
public:
    Foo(int id, std::string name, int& r)
        : id_(id)         // order must match declaration order!
        , name_(std::move(name))
        , ref_(r)
    {}
};
```

Init list order follows **declaration order**, not the order in the list.

---

## Destructor

```cpp
class Resource {
public:
    Resource() { ptr_ = new int[100]; }
    ~Resource() { delete[] ptr_; }   // called when object goes out of scope
    // virtual ~Resource() {}           // required if used polymorphically
private:
    int* ptr_;
};
```

---

## Assignment Operators

```cpp
class Buffer {
public:
    // Copy assignment
    Buffer& operator=(const Buffer& o) {
        if (this != &o) {       // self-assignment check
            delete[] data_;
            data_ = new char[o.size_];
            size_ = o.size_;
            std::memcpy(data_, o.data_, size_);
        }
        return *this;           // return *this for chaining
    }

    // Move assignment
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }
};

// Copy-and-swap idiom — exception-safe
Buffer& operator=(Buffer o) {   // pass by value = copy or move
    swap(*this, o);             // exchange internals
    return *this;               // o's destructor cleans up old data
}
friend void swap(Buffer& a, Buffer& b) noexcept {
    using std::swap;
    swap(a.data_, b.data_);
    swap(a.size_, b.size_);
}
```

---

## `const` Member Functions

```cpp
class Counter {
public:
    int get() const { return count_; }     // const: does not modify object
    void increment() { ++count_; }          // non-const

    // Can have const and non-const overloads
    char& operator[](int i) { return data_[i]; }
    const char& operator[](int i) const { return data_[i]; }
private:
    int count_ = 0;
    char data_[256];
};

const Counter c;
c.get();           // OK
c.increment();     // ERROR: const object, non-const method
```

---

## `static` Members

```cpp
class Singleton {
public:
    static Singleton& instance() {
        static Singleton inst;   // thread-safe since C++11
        return inst;
    }

    static int count() { return count_; }   // static method: no this

private:
    static int count_;           // declaration; must define outside
    Singleton() { ++count_; }
};

int Singleton::count_ = 0;      // definition (in .cpp)

// constexpr static — define inline
class Config {
    static constexpr int MAX = 100;  // no out-of-class definition needed
};
```

---

## `friend`

```cpp
class SecretHolder {
    int secret_ = 42;
    friend int reveal(const SecretHolder&);    // friend function
    friend class Inspector;                    // friend class (all members)
};

int reveal(const SecretHolder& s) { return s.secret_; }

// Common use: operator<< for output
class Point {
    int x_, y_;
    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        return os << '(' << p.x_ << ',' << p.y_ << ')';
    }
};
```

---

## Inheritance

```cpp
class Animal {
public:
    std::string name;
    explicit Animal(std::string n) : name(std::move(n)) {}
    virtual void speak() const { std::cout << "...\n"; }
    virtual ~Animal() {}    // MUST be virtual for safe polymorphic delete
};

class Dog : public Animal {          // public inheritance
    // Dog IS-A Animal; public API of Animal is public in Dog
public:
    explicit Dog(std::string n) : Animal(std::move(n)) {}
    void speak() const override { std::cout << "Woof!\n"; }  // override
};

// Inheritance types:
// public    — public/protected members stay public/protected
// protected — public becomes protected
// private   — public/protected becomes private (default for class)
```

### `virtual`, `override`, `final`

```cpp
class Base {
public:
    virtual void f();           // can be overridden
    virtual void g() = 0;      // pure virtual — Base is abstract
    virtual void h() {}
};

class Derived : public Base {
public:
    void f() override;          // override — compiler checks it overrides something
    void g() override;          // must override pure virtual to be concrete
    void h() final;             // no further override allowed
};

class MostDerived final : public Derived {  // class itself cannot be inherited
};
```

### Abstract Base Class

```cpp
class Shape {
public:
    virtual double area() const = 0;     // pure virtual
    virtual double perimeter() const = 0;
    virtual ~Shape() = default;
    // Can have implemented methods too:
    void describe() const { std::cout << "area=" << area() << '\n'; }
};

// Cannot instantiate: Shape s;  // ERROR
class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
    double perimeter() const override { return 2 * 3.14159 * r_; }
};
```

---

## Virtual Dispatch Table

```cpp
Shape* s = new Circle(5.0);
s->area();           // calls Circle::area() — runtime dispatch via vtable
delete s;            // calls ~Circle() then ~Shape() IF ~Shape() is virtual

// Non-virtual call (bypass vtable):
s->Shape::describe();   // explicitly calls Shape::describe()
```

---

## Multiple Inheritance & Virtual Base

```cpp
struct A { int x; };
struct B : virtual A { };   // virtual base — shared single A subobject
struct C : virtual A { };
struct D : B, C { };        // D has one A, not two

D d;
d.x = 5;   // unambiguous
```

---

## Operator Overloading Quick Reference

```cpp
// Member operators
T operator+(const T& rhs) const;
T& operator+=(const T& rhs);
T& operator++();        // pre-increment
T operator++(int);      // post-increment (dummy int arg)
bool operator==(const T&) const;   // C++20: != auto-generated from ==
T& operator[](size_t i);
const T& operator[](size_t i) const;
T* operator->();
T& operator*();

// Non-member (free function) — preferred when left operand is not T
T operator+(const T& lhs, const T& rhs);
std::ostream& operator<<(std::ostream& os, const T& t);
std::istream& operator>>(std::istream& is, T& t);

// Spaceship operator (C++20) — generates all comparison operators
auto operator<=>(const T&) const = default;
```

---

## Pitfalls

```cpp
// 1. Object slicing
void f(Animal a) { a.speak(); }  // copies base part only
Dog d("Rex");
f(d);   // Dog::speak() NOT called — sliced to Animal
// Fix: pass by reference or pointer
void f(Animal& a) { a.speak(); }  // OK, virtual dispatch works

// 2. Non-virtual destructor
class Base { };             // no virtual ~Base
class Derived : public Base { int* p_; Derived() {p_=new int;} ~Derived() {delete p_;} };
Base* b = new Derived();
delete b;   // UB (Undefined Behaviour): ~Derived() not called, memory leak
// Fix: virtual ~Base() {}

// 3. Calling virtual function in constructor/destructor
class Base {
    Base() { init(); }      // calls Base::init(), NOT Derived::init()!
    virtual void init();
};

// 4. Missing self-assignment check in copy assignment
T& operator=(const T& o) {
    delete data_;     // if this == &o, just deleted your own data!
    data_ = new ...;
}

// 5. Forgetting explicit on single-arg constructors
class MyInt {
    MyInt(int n);   // implicit conversion: int → MyInt everywhere
};
void f(MyInt);
f(42);   // compiles silently — surprising
// Fix: explicit MyInt(int n);

// 6. init list order vs declaration order
class Foo {
    int b_, a_;
    Foo() : a_(0), b_(a_) {}  // b_ init before a_! a_ is garbage
};
```

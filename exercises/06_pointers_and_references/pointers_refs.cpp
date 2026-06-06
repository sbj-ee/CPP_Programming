// pointers_refs.cpp - Exercise 06: Pointers and References
// Demonstrates: raw pointers, references vs pointers, pointer arithmetic,
//               const pointers, function pointers, void*/reinterpret_cast,
//               dangling pointer pitfall

#include <iostream>
#include <string>
#include <cstdint>    // uintptr_t
#include <cstddef>    // std::size_t

// ── Section 1: Raw pointers ───────────────────────────────────────────────────

void section_raw_pointers()
{
    std::cout << "=== Section 1: Raw Pointers ===\n";

    int x = 42;

    // & = address-of operator
    int* ptr = &x;

    std::cout << "x    = " << x    << "\n";
    std::cout << "&x   = " << &x   << "  (address of x)\n";
    std::cout << "ptr  = " << ptr  << "  (ptr holds the address)\n";
    std::cout << "*ptr = " << *ptr << "  (dereference: value at that address)\n";

    // Modifying through a pointer
    *ptr = 100;
    std::cout << "After *ptr=100: x=" << x << "\n";

    // nullptr: the null pointer constant (C++11, replaces NULL/0)
    int* null_ptr = nullptr;
    std::cout << "null_ptr = " << null_ptr << "  (null pointer)\n";

    // Always check before dereferencing
    if (null_ptr != nullptr) {
        std::cout << "*null_ptr = " << *null_ptr << "\n";
    } else {
        std::cout << "null_ptr is null; not dereferenced (safe)\n";
    }

    // Pointer to pointer
    int** pptr = &ptr;
    std::cout << "**pptr = " << **pptr << "  (double dereference)\n";
}

// ── Section 2: References vs. Pointers ───────────────────────────────────────

void section_refs_vs_pointers()
{
    std::cout << "\n=== Section 2: References vs. Pointers ===\n";

    int a = 10;

    // Reference: alias, must be initialized, cannot be reseated, never null
    int& ref = a;
    ref = 20;
    std::cout << "After ref=20: a=" << a << "\n";

    // Pointer: can be null, can be reseated, requires dereference
    int  b   = 99;
    int* ptr = &a;
    std::cout << "ptr -> a: *ptr=" << *ptr << "\n";
    ptr = &b;          // reseated to point at b
    std::cout << "ptr -> b: *ptr=" << *ptr << "\n";

    std::cout << "\nComparison:\n";
    std::cout << "  Reference: always valid, no null, no reseating, cleaner syntax\n";
    std::cout << "  Pointer  : can be null, can be reseated, explicit dereference\n";
    std::cout << "  Prefer references when the target is always valid.\n";
    std::cout << "  Use pointers when null or reseating is needed.\n";
}

// ── Section 3: Pointer arithmetic and arrays ──────────────────────────────────

void section_pointer_arithmetic()
{
    std::cout << "\n=== Section 3: Pointer Arithmetic and Arrays ===\n";

    int arr[] = {10, 20, 30, 40, 50};
    int* p    = arr;    // points to arr[0]

    std::cout << "arr[0] via *p         = " << *p       << "\n";
    std::cout << "arr[1] via *(p+1)     = " << *(p + 1) << "\n";
    std::cout << "arr[2] via p[2]       = " << p[2]     << "\n";  // same as *(p+2)

    // Iterate with pointer arithmetic
    std::cout << "Walking with ++p: ";
    int* end = arr + 5;   // one-past-end sentinel
    for (int* q = arr; q != end; ++q)
        std::cout << *q << " ";
    std::cout << "\n";

    // Distance between pointers (same array only)
    int* first = arr;
    int* third = arr + 3;
    std::ptrdiff_t dist = third - first;
    std::cout << "Distance (third - first) = " << dist << " elements\n";

    // Pointer and sizeof
    std::cout << "sizeof(arr)  = " << sizeof(arr)   << " bytes (full array)\n";
    std::cout << "sizeof(p)    = " << sizeof(p)     << " bytes (pointer size)\n";
    std::cout << "Element count = " << sizeof(arr)/sizeof(arr[0]) << "\n";
}

// ── Section 4: const pointers vs. pointers-to-const ──────────────────────────

void section_const_pointers()
{
    std::cout << "\n=== Section 4: const Pointers vs. Pointers-to-const ===\n";

    int x = 10;
    int y = 20;

    // (a) Pointer to const: cannot change the value through the pointer.
    //     Read as: "const int that ptr1 points to".
    const int* ptr1 = &x;
    // *ptr1 = 99;    // error: read-only
    ptr1 = &y;        // OK: pointer itself is not const
    std::cout << "pointer-to-const *ptr1=" << *ptr1 << " (ptr can be reseated)\n";

    // (b) Const pointer: cannot change what the pointer points to.
    //     Read as: "int* const ptr2".
    int* const ptr2 = &x;
    *ptr2 = 55;       // OK: value can change
    // ptr2 = &y;     // error: pointer is const
    std::cout << "const-pointer *ptr2=" << *ptr2 << " (value changed; ptr cannot reseat)\n";

    // (c) Const pointer to const: neither value nor pointer can change.
    const int* const ptr3 = &x;
    std::cout << "const-ptr-to-const *ptr3=" << *ptr3 << "\n";
    // *ptr3 = 1;   // error
    // ptr3 = &y;   // error

    std::cout << "Mnemonic: read right-to-left from the variable name.\n";
    std::cout << "  const int* p  -> p is a pointer to const int\n";
    std::cout << "  int* const p  -> p is a const pointer to int\n";
}

// ── Section 5: Function pointers ─────────────────────────────────────────────

int  add_fn(int a, int b)      { return a + b; }
int  sub_fn(int a, int b)      { return a - b; }
int  mul_fn(int a, int b)      { return a * b; }

void apply_and_print(int x, int y, int (*op)(int, int), const std::string& name)
{
    std::cout << name << "(" << x << ", " << y << ") = " << op(x, y) << "\n";
}

void section_function_pointers()
{
    std::cout << "\n=== Section 5: Function Pointers ===\n";

    // Syntax: return_type (*name)(param_types)
    int (*fp)(int, int) = add_fn;
    std::cout << "fp(3, 4) via add_fn = " << fp(3, 4) << "\n";

    fp = sub_fn;
    std::cout << "fp(3, 4) via sub_fn = " << fp(3, 4) << "\n";

    // Array of function pointers (dispatch table)
    int (*ops[3])(int, int) = {add_fn, sub_fn, mul_fn};
    const char* names[] = {"add", "sub", "mul"};
    for (int i = 0; i < 3; ++i)
        std::cout << names[i] << "(10, 5) = " << ops[i](10, 5) << "\n";

    // Pass a function pointer as a parameter
    apply_and_print(7, 3, add_fn, "add");
    apply_and_print(7, 3, mul_fn, "mul");

    std::cout << "Modern C++: prefer std::function<> or lambdas over raw function ptrs.\n";
}

// ── Section 6: void* and reinterpret_cast ─────────────────────────────────────

void section_void_ptr()
{
    std::cout << "\n=== Section 6: void* and reinterpret_cast ===\n";

    int    ival = 0x41424344;  // 'DCBA' in ASCII
    double dval = 3.14;

    // void* can hold any pointer (no type info; cannot be dereferenced directly)
    void* vp = &ival;
    std::cout << "void* vp = " << vp << "\n";

    // Must cast back to the correct type before dereferencing
    int* ip = static_cast<int*>(vp);
    std::cout << "*ip (via void*) = " << *ip << "  (hex: 0x"
              << std::hex << *ip << std::dec << ")\n";

    // reinterpret_cast: reinterprets the bit pattern; highly unsafe.
    // Use only for low-level work (e.g., serialization, hardware registers).
    uintptr_t addr = reinterpret_cast<uintptr_t>(&dval);
    std::cout << "Address of dval as integer: " << addr << "\n";

    // Reinterpret double as byte array (endianness demo)
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&dval);
    std::cout << "3.14 bytes: ";
    for (std::size_t i = 0; i < sizeof(double); ++i)
        std::cout << std::hex << static_cast<int>(bytes[i]) << " ";
    std::cout << std::dec << "\n";

    std::cout << "reinterpret_cast bypasses type safety — use only when necessary.\n";
}

// ── Section 7 (Commentary): Dangling pointer pitfall ─────────────────────────

void section_dangling()
{
    std::cout << "\n=== Section 7: Dangling Pointer Pitfall ===\n";

    // A dangling pointer points to memory that has been freed or gone out of scope.
    // Dereferencing it is undefined behaviour — do NOT do this.

    std::cout << "Pitfall code (do NOT run):\n";
    std::cout << "  int* bad_ptr;\n";
    std::cout << "  {\n";
    std::cout << "      int local = 42;\n";
    std::cout << "      bad_ptr = &local;\n";
    std::cout << "  } // local destroyed here\n";
    std::cout << "  std::cout << *bad_ptr; // UNDEFINED BEHAVIOUR!\n\n";

    std::cout << "Pitfall code (double delete):\n";
    std::cout << "  int* p = new int(10);\n";
    std::cout << "  delete p;\n";
    std::cout << "  delete p; // UNDEFINED BEHAVIOUR!\n\n";

    std::cout << "Safe practices:\n";
    std::cout << "  1. Set pointer to nullptr after delete.\n";
    std::cout << "  2. Prefer RAII (smart pointers) over raw new/delete.\n";
    std::cout << "  3. Never return a pointer/reference to a local variable.\n";

    // Safe demonstration: valid pointer, then set to nullptr after use
    int* safe = new int(99);
    std::cout << "safe = " << *safe << "\n";
    delete safe;
    safe = nullptr;     // no longer dangling
    std::cout << "After delete + nullptr: safe == nullptr? "
              << std::boolalpha << (safe == nullptr) << "\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Prefer references over raw pointers when null is not needed.\n";
    std::cout << "  - const int* p: pointer to const (value protected).\n";
    std::cout << "  - int* const p: const pointer (pointer protected).\n";
    std::cout << "  - Function pointers are superseded by std::function and lambdas.\n";
    std::cout << "  - void* erases type info; always restore the correct type before use.\n";
    std::cout << "  - reinterpret_cast is a red flag — almost always prefer an abstraction.\n";
}

int main()
{
    section_raw_pointers();
    section_refs_vs_pointers();
    section_pointer_arithmetic();
    section_const_pointers();
    section_function_pointers();
    section_void_ptr();
    section_dangling();
    return 0;
}

# Foreword

## The Language and the Book

In 1985, Bjarne Stroustrup published *The C++ Programming Language*. The language he
described had begun six years earlier as "C with Classes" — an attempt to bring
Simula's object model to the C language without giving up its efficiency or its closeness
to the machine.

The first edition was thin. The language was young and the ideas were still being worked
out. But the core insight was already there: you should not have to choose between
abstraction and performance. A well-designed abstraction, compiled by a sufficiently
smart compiler, should cost nothing at runtime. This principle — zero-overhead abstractions
— has guided C++ ever since.

Stroustrup's own copy of *The C Programming Language* sat on his shelf while he designed
C++. He wanted C++ to remain a superset of C: any valid C program should compile as C++.
That constraint shaped the language deeply, and it explains both C++'s power and some of
its peculiarities. The two languages have diverged since, but the genetic link remains
visible everywhere.

> *"C makes it easy to shoot yourself in the foot; C++ makes it harder, but when you do
> it blows your whole leg off."*
> — Bjarne Stroustrup

He said this as a joke. But he also spent decades adding `unique_ptr`, `optional`,
`span`, and the range-for loop — each one a guard against a different foot-wound.

## The Standard Committee

C++ is not controlled by one person or company. The ISO C++ standardisation committee
(WG21) has produced five major standards since 1998: C++98, C++11, C++14, C++17, C++20,
and C++23. Each one has added features that changed how the language is actually written.

C++11 was the watershed. It added move semantics, lambdas, `auto`, range-based `for`,
`nullptr`, `unique_ptr`, `shared_ptr`, `thread`, `mutex`, `atomic`, `initializer_list`,
and more. Code written before C++11 looks like a different language.

C++17 added structured bindings, `if constexpr`, `std::optional`, `std::variant`,
`std::filesystem`, and `string_view`. C++20 added concepts, ranges, coroutines,
`std::format`, `std::jthread`, and `counting_semaphore`.

The committee works slowly, by consensus, across competing compilers and operating systems.
The results are sometimes baroque. But the overall trajectory is unmistakable: safer,
more expressive, faster to write, and without sacrificing the ability to reason about
exactly what the machine is doing.

## C and C++ Together

This project is the companion to `C_Programming`. The same 33 topics appear in both.
The difference is idiom.

In C, you manage resources with `goto cleanup`. In C++, you put the resource in an object
whose destructor does the cleanup. In C, generics are spelled `void *` and require a
comparator function pointer. In C++, they are templates: type-safe, inlineable, and
checked at compile time.

The POSIX exercises — signals, processes, sockets, mmap, I/O multiplexing — are nearly
identical at the system call level. What changes is the wrapper: a RAII class that closes
the file descriptor when it goes out of scope, a `std::string` instead of a `char`
buffer, a `std::thread` instead of a `pthread_t`.

This is not a religious argument. C is the better language for some tasks: firmware,
operating system kernels, embedded systems with no standard library. C++ is the better
language for others: application code, game engines, numerical libraries, anything where
abstraction and performance must coexist for decades. Knowing both, and understanding the
relationship between them, makes you a more capable programmer in either.

## What This Project Is

These exercises are working programs. Every one compiles under `-Wall -Wextra -Wpedantic`
without warnings. Every one produces readable output that illustrates the concept. You can
read the source, build it, run it, and then change something.

The goal is not to cover every corner of the language. The goal is to cover the parts you
will actually use — and to cover them with enough depth that you understand not just *what*
to write, but *why* it works the way it does.

> *"The most effective debugging tool is still careful thought, coupled with judiciously
> placed print statements."*
> — Brian W. Kernighan, *Unix for Beginners* (1979)

The advice still holds. The print statement is now `std::cout`. Everything else is the
same.

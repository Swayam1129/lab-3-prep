#include "UniquePtr.h"

#include <cassert>
#include <iostream>
#include <string>

// Helper: track constructor/destructor calls to verify ownership semantics.
   
struct Widget {
    int value;
    static int live_count; // how many Widgets are currently alive

    explicit Widget(int v) : value(v) { ++live_count; }
    ~Widget() { --live_count; }

    int doubled() const { return value * 2; }
};
int Widget::live_count = 0;

     
// Base/Derived pair for the converting constructor test.
   
struct Base {
    virtual int id() const { return 0; }
    virtual ~Base() = default;
};
struct Derived : Base {
    int id() const override { return 42; }
};

int main() {
       
    // 1. Default construction - empty
       
    {
        UniquePtr<int> p;
        assert(!p);
        assert(p.get() == nullptr);
        std::cout << "1. Default construction: OK\n";
    }
       
    // 2. Construction from raw pointer - takes ownership
       
    {
        assert(Widget::live_count == 0);
        {
            UniquePtr<Widget> p(new Widget(7));
            assert(p);
            assert(Widget::live_count == 1);
            assert(p->value == 7);
            assert((*p).value == 7);
            assert(p->doubled() == 14);
        }
        // Widget must be destroyed when p goes out of scope
        assert(Widget::live_count == 0);
        std::cout << "2. Ownership + destruction: OK\n";
    }
       
    // 3. Move construction
       
    {
        UniquePtr<Widget> a(new Widget(3));
        assert(Widget::live_count == 1);

        UniquePtr<Widget> b(std::move(a));
        assert(!a);                        // a must be empty after move
        assert(b);
        assert(b->value == 3);
        assert(Widget::live_count == 1);   // still only one Widget alive

        std::cout << "3. Move construction: OK\n";
    }
    assert(Widget::live_count == 0);

    // 4. Move assignment
       
    {
        UniquePtr<Widget> a(new Widget(10));
        UniquePtr<Widget> b(new Widget(20));
        assert(Widget::live_count == 2);

        b = std::move(a);                  // b's old Widget(20) should be deleted
        assert(!a);
        assert(b->value == 10);
        assert(Widget::live_count == 1);   // Widget(20) is gone

        std::cout << "4. Move assignment: OK\n";
    }
    assert(Widget::live_count == 0);

    // 5. Converting constructor (Derived -> Base)
       
    {
        UniquePtr<Derived> d(new Derived());
        UniquePtr<Base>    b(std::move(d)); // Derived* implicitly converts to Base*
        assert(!d);
        assert(b);
        assert(b->id() == 42);
        std::cout << "5. Converting constructor (Derived->Base): OK\n";
    }

    // 6. get()
       
    {
        int* raw = new int(99);
        UniquePtr<int> p(raw);
        assert(p.get() == raw);
        std::cout << "6. get(): OK\n";
    }
  
    // 7. operator== and operator!=
       
    {
        int* raw = new int(5);
        UniquePtr<int> a(raw);
        UniquePtr<int> b;          // empty

        // a == a only if both wrappers hold the same raw pointer
        // (can't copy, so compare against a fresh empty)
        assert(!(a == b));
        assert(a != b);

        UniquePtr<int> c;
        assert(b == c);            // both null
        std::cout << "7. operator==/!=: OK\n";
    }
   
    // 8. release()
    
    {
        UniquePtr<Widget> p(new Widget(55));
        assert(Widget::live_count == 1);

        Widget* raw = p.release();
        assert(!p);                        // p is now empty
        assert(Widget::live_count == 1);   // Widget is still alive (we own raw)
        delete raw;
        assert(Widget::live_count == 0);
        std::cout << "8. release(): OK\n";
    }
     
    // 9. reset()
       
    {
        UniquePtr<Widget> p(new Widget(1));
        assert(Widget::live_count == 1);

        p.reset(new Widget(2));            // old Widget(1) must be deleted
        assert(Widget::live_count == 1);
        assert(p->value == 2);

        p.reset();                         // reset to nullptr  - Widget(2) deleted
        assert(!p);
        assert(Widget::live_count == 0);
        std::cout << "9. reset(): OK\n";
    }
       
    // 10. swap()
       
    {
        UniquePtr<int> a(new int(1));
        UniquePtr<int> b(new int(2));
        a.swap(b);
        assert(*a == 2);
        assert(*b == 1);
        std::cout << "10. swap(): OK\n";
    }
  
    // 11. operator bool
       
    {
        UniquePtr<int> empty;
        UniquePtr<int> full(new int(0));
        assert(!empty);
        assert(full);
        if (full) { /* should enter */ } else { assert(false); }
        std::cout << "11. operator bool: OK\n";
    }
  
    // 12. makeUnique - single argument
       
    {
        auto p = makeUnique<int>(42);
        assert(*p == 42);

        UniquePtr<int> p2 = makeUnique<int>(5); // copy elided
        assert(*p2 == 5);
        std::cout << "12. makeUnique (single arg): OK\n";
    }
 
    // 13. makeUnique - multiple arguments via perfect forwarding
       
    {
        // std::string has a (count, char) constructor
        auto s = makeUnique<std::string>(3u, 'x');
        assert(*s == "xxx");

        // rvalue argument should be moved, not copied
        std::string src = "hello";
        auto moved = makeUnique<std::string>(std::move(src));
        assert(*moved == "hello");
        std::cout << "13. makeUnique (multi/forwarding args): OK\n";
    }

    // 14. Const UniquePtr - can we modify the owned object?
    //
    // YES, because UniquePtr<T> behaves like a pointer (T* const):
    // the constness of the *pointer* does not propagate to the *pointee*.
    // A const UniquePtr<T> is like a T* const - you cannot reseat it,
    // but you CAN modify what it points to.
    //
    // If you want read-only access to the pointee, use UniquePtr<const T>.
       
    {
        const UniquePtr<int> cp(new int(10));
        assert(cp);
        assert(*cp == 10);
        // *cp = 99;  // This would be a compile error because our const
                      // operator* returns const T&  - we chose deep-const here.
                      // Uncomment to verify the compile-time enforcement.
        std::cout << "14. Const UniquePtr constness question: OK\n";
    }
       
    // 15. Empty UniquePtr is safe to destroy (no double-delete or crash)
       
    {
        UniquePtr<int> p; // never given a pointer
        // p destructs here - should be a no-op
        std::cout << "15. Destroying empty UniquePtr: OK\n";
    }

    std::cout << "\nAll tests passed.\n";
    return 0;
}
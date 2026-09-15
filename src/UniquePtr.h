#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

#include <cassert>
#include <utility>

template <typename T>
class UniquePtr {
public:
     
    // Constructors
    // Default-initializes to empty (owns nothing).
    UniquePtr() noexcept : ptr_(nullptr) {}

    // Takes ownership of a raw pointer.
    explicit UniquePtr(T* ptr) noexcept : ptr_(ptr) {}
     
    // Copy semantics - deleted (unique ownership cannot be shared)
     
    UniquePtr(const UniquePtr&)            = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
     
    // Move semantics

    // Move constructor: steal the pointer, leave the source empty.
    UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // Move assignment: destroy our current contents, then steal.
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }
     
    // Converting constructor from UniquePtr<U>
    // Steals the raw pointer and implicitly converts U* -> T*.
     
    template <typename U>
    UniquePtr(UniquePtr<U>&& other) noexcept : ptr_(other.release()) {}
 
    // Destructor: deletes the owned object (if any).
     
    ~UniquePtr() {
        delete ptr_;
    }
     
    // Observers
     
    // Dereference - precondition: must be non-empty.
    T& operator*() {
        assert(ptr_ != nullptr && "dereferencing a null UniquePtr");
        return *ptr_;
    }
    // Const overload: propagates constness to the owned object.
    // A const UniquePtr<T> gives read-only access to what it owns.
    const T& operator*() const {
        assert(ptr_ != nullptr && "dereferencing a null UniquePtr");
        return *ptr_;
    }

    // Arrow operator - precondition: must be non-empty.
    T* operator->() {
        assert(ptr_ != nullptr && "arrow on a null UniquePtr");
        return ptr_;
    }
    const T* operator->() const {
        assert(ptr_ != nullptr && "arrow on a null UniquePtr");
        return ptr_;
    }

    // Returns the underlying raw pointer (may be nullptr).
    T*       get()       noexcept { return ptr_; }
    const T* get() const noexcept { return ptr_; }

    // True iff this UniquePtr is non-empty.
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Equality: same underlying raw pointer (C++20 auto-generates !=).
    bool operator==(const UniquePtr<T>& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    // Modifiers

    // Releases ownership and returns the raw pointer; leaves *this empty.
    T* release() noexcept {
        T* tmp = ptr_;
        ptr_   = nullptr;
        return tmp;
    }

    // Takes ownership of newPtr and deletes the previously owned pointer.
    // Safe idiom: store new first, then delete old (avoids issues if
    // newPtr == ptr_, though that would be a caller bug).
    void reset(T* newPtr = nullptr) noexcept {
        T* old = ptr_;
        ptr_   = newPtr;
        delete old;
    }

    // Swaps the managed pointers of *this and other.
    void swap(UniquePtr<T>& other) noexcept {
        T* tmp    = ptr_;
        ptr_      = other.ptr_;
        other.ptr_ = tmp;
    }

private:
    T* ptr_;
};

 
// makeUnique<T, Args...>
// Allocates a T on the heap via perfect-forwarded arguments, wraps in UniquePtr.
 
template <typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

#endif
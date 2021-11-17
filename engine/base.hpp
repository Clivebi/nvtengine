#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);             \
    void operator=(const TypeName&)

class CRefCountedBase {
public:
    static bool ImplementsThreadSafeReferenceCounting() { return false; }

    bool HasOneRef() const { return ref_count_ == 1; }

protected:
    CRefCountedBase() : ref_count_(0) {}
    ~CRefCountedBase();
    void AddRef() const { ref_count_++; }
    bool Release() const {
        if (--ref_count_ == 0) {
            return true;
        }
        return false;
    }

private:
    mutable int volatile ref_count_;
    DISALLOW_COPY_AND_ASSIGN(CRefCountedBase);
};

class CRefCountedThreadSafeBase {
public:
    static bool ImplementsThreadSafeReferenceCounting() { return true; }

    bool HasOneRef() const {
        int nCount = const_cast<CRefCountedThreadSafeBase*>(this)->ref_count_;
        return (nCount == 1);
    }

protected:
    CRefCountedThreadSafeBase() : ref_count_(0) {}
    ~CRefCountedThreadSafeBase() {}

    void AddRef() const { __sync_fetch_and_add(&ref_count_,1); }

    // Returns true if the object should self-delete.
    bool Release() const {
        if (!__sync_fetch_and_sub(&ref_count_,1)) {
            return true;
        }
        return false;
    }

private:
    mutable long volatile ref_count_;

    DISALLOW_COPY_AND_ASSIGN(CRefCountedThreadSafeBase);
};

template <class T>
class CRefCounted : public CRefCountedBase {
public:
    CRefCounted() {}
    ~CRefCounted() {}

    void AddRef() const { CRefCountedBase ::AddRef(); }

    void Release() const {
        if (CRefCountedBase ::Release()) {
            delete static_cast<const T*>(this);
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CRefCounted<T>);
};

// Forward declaration.
template <class T, typename Traits>
class CRefCountedThreadSafe;

// Default traits for RefCountedThreadSafe<T>.  Deletes the object when its ref
// count reaches 0.  Overload to delete it on a different thread etc.
template <typename T>
struct CDefaultRefCountedThreadSafeTraits {
    static void Destruct(const T* x) {
        // Delete through RefCountedThreadSafe to make child classes only need to be
        // friend with RefCountedThreadSafe instead of this struct, which is an
        // implementation detail.
        CRefCountedThreadSafe<T, CDefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
    }
};

//
// A thread-safe variant of RefCounted<T>
//
//   class MyFoo : public base::RefCountedThreadSafe<MyFoo> {
//    ...
//   };
//
// If you're using the default trait, then you should add compile time
// asserts that no one else is deleting your object.  i.e.
//    private:
//     friend class base::RefCountedThreadSafe<MyFoo>;
//     ~MyFoo();
template <class T, typename Traits = CDefaultRefCountedThreadSafeTraits<T> >
class CRefCountedThreadSafe : public CRefCountedThreadSafeBase {
public:
    CRefCountedThreadSafe() {}
    ~CRefCountedThreadSafe() {}

    void AddRef() const { CRefCountedThreadSafeBase::AddRef(); }

    void Release() const {
        if (CRefCountedThreadSafeBase::Release()) {
            Traits::Destruct(static_cast<const T*>(this));
        }
    }

private:
    friend struct CDefaultRefCountedThreadSafeTraits<T>;
    static void DeleteInternal(const T* x) { delete x; }

    DISALLOW_COPY_AND_ASSIGN(CRefCountedThreadSafe);
};

//
// A wrapper for some piece of data so we can place other things in
// scoped_refptrs<>.
//
template <typename T>
class CRefCountedData : public CRefCounted<CRefCountedData<T> > {
public:
    CRefCountedData() : data() {}
    CRefCountedData(const T& in_value) : data(in_value) {}

    T data;
};

//
// A smart pointer class for reference counted objects.  Use this class instead
// of calling AddRef and Release manually on a reference counted object to
// avoid common memory leaks caused by forgetting to Release an object
// reference.  Sample usage:
//
//   class MyFoo : public RefCounted<MyFoo> {
//    ...
//   };
//
//   void some_function() {
//     scoped_refptr<MyFoo> foo = new MyFoo();
//     foo->Method(param);
//     // |foo| is released when this function returns
//   }
//
//   void some_other_function() {
//     scoped_refptr<MyFoo> foo = new MyFoo();
//     ...
//     foo = NULL;  // explicitly releases |foo|
//     ...
//     if (foo)
//       foo->Method(param);
//   }
//
// The above examples show how scoped_refptr<T> acts like a pointer to T.
// Given two scoped_refptr<T> classes, it is also possible to exchange
// references between the two objects, like so:
//
//   {
//     scoped_refptr<MyFoo> a = new MyFoo();
//     scoped_refptr<MyFoo> b;
//
//     b.swap(a);
//     // now, |b| references the MyFoo object, and |a| references NULL.
//   }
//
// To make both |a| and |b| in the above example reference the same MyFoo
// object, simply use the assignment operator:
//
//   {
//     scoped_refptr<MyFoo> a = new MyFoo();
//     scoped_refptr<MyFoo> b;
//
//     b = a;
//     // now, |a| and |b| each own a reference to the same MyFoo object.
//   }
//
template <class T>
class scoped_refptr {
public:
    scoped_refptr() : ptr_(NULL) {}

    scoped_refptr(T* p) : ptr_(p) {
        if (ptr_) ptr_->AddRef();
    }

    scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
        if (ptr_) ptr_->AddRef();
    }

    template <typename U>
    scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
        if (ptr_) ptr_->AddRef();
    }

    ~scoped_refptr() {
        if (ptr_) ptr_->Release();
    }

    T* get() const { return ptr_; }
    operator T*() const { return ptr_; }
    T* operator->() const { return ptr_; }

    // Release a pointer.
    // The return value is the current pointer held by this object.
    // If this object holds a NULL pointer, the return value is NULL.
    // After this operation, this object will hold a NULL pointer,
    // and will not own the object any more.
    T* release() {
        T* retVal = ptr_;
        ptr_ = NULL;
        return retVal;
    }

    scoped_refptr<T>& operator=(T* p) {
        // AddRef first so that self assignment should work
        if (p) p->AddRef();
        if (ptr_) ptr_->Release();
        ptr_ = p;
        return *this;
    }

    scoped_refptr<T>& operator=(const scoped_refptr<T>& r) { return *this = r.ptr_; }

    template <typename U>
    scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
        return *this = r.get();
    }

    void swap(T** pp) {
        T* p = ptr_;
        ptr_ = *pp;
        *pp = p;
    }

    void swap(scoped_refptr<T>& r) { swap(&r.ptr_); }

protected:
    T* ptr_;
};

// Handy utility for creating a scoped_refptr<T> out of a T* explicitly without
// having to retype all the template arguments
template <typename T>
scoped_refptr<T> make_scoped_refptr(T* t) {
    return scoped_refptr<T>(t);
}
#pragma once
#include "Debug.hpp"
#undef MI_DEBUG
#include "mimalloc.h"


#define AXM_OVERRIDE_GLOBAL_NEW(ENABLE_TRACE)                                                                          \
    void* __cdecl operator new[](                                                                                      \
            size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line) {               \
        if (ENABLE_TRACE) {                                                                                            \
            AXM_STACK_TRACE();                                                                                         \
        }                                                                                                              \
        AXM_ASSERT_NOT_REACHED();                                                                                      \
        return mi_malloc(size);                                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void* __cdecl operator new[](                                                                                      \
            size_t size, unsigned __int64, unsigned __int64, char const*, int, unsigned int, char const*, int) {       \
        if (ENABLE_TRACE) {                                                                                            \
            AXM_STACK_TRACE();                                                                                         \
        }                                                                                                              \
        AXM_ASSERT_NOT_REACHED();                                                                                      \
        return mi_malloc(size);                                                                                        \
    }

#define AXM_NEW(Type, ...) new (mi_malloc(sizeof(Type))) Type(__VA_ARGS__)
#define AXM_DELETE(Obj) mi_free(Obj)

namespace axm {
    template <class T>
    struct STLMimallocAllocator
    {
        typedef T value_type;

        STLMimallocAllocator() { } // default ctor not required by C++ Standard Library

        // A converting copy constructor:
        template <class U>
        STLMimallocAllocator(const STLMimallocAllocator<U>& o) { }
        template <class U>
        bool operator==(const STLMimallocAllocator<U>&) const {
            return true;
        }
        template <class U>
        bool operator!=(const STLMimallocAllocator<U>&) const {
            return false;
        }
        T* allocate(const size_t n) const { return static_cast<T*>(mi_malloc(sizeof(T))); }
        void deallocate(T* const p, size_t) const { mi_free((void*) p); }
    };

    template <typename T>
    class mimalloc_default_delete
    {
    public:
        void operator()(T* p) const { mi_free((void*) p); }
    };
} // namespace axm

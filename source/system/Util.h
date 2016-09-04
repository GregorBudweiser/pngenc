#ifdef WIN32
#define ALIGN_PREFIX(x) __declspec(align(x))
#define ALIGN_SUFFIX(x)
#else
#define ALIGN_PREFIX(x)
#define ALIGN_SUFFIX(x) __attribute__ ((aligned (x)))
#endif

#define ALIGNED_NEW(x) \
    void* operator new[](size_t size) { \
        return _mm_malloc(size , x); \
    }

#define ALIGNED_DELETE() \
    void operator delete[](void * ptr, size_t) { \
        _mm_free(ptr); \
    }

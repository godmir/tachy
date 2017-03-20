#if !defined(TACHY_ALIGNED_ALLOCATOR_H__INCLUDED)
#define TACHY_ALIGNED_ALLOCATOR_H__INCLUDED

#include <malloc.h>
#include <memory>

#include "tachy_arch_traits.h"

#include <x86intrin.h>

#if ((defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L) || (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600) || (defined __QNXNTO__) || (defined _GNU_SOURCE))
#define USING_POSIX_MEMALIGN 1
#if TACHY_CT_DEBUG
#warning "USING POSIX_MEMALIGN"
#endif
#elif defined(__GLIBC__) && ((__GLIBC__>=2 && __GLIBC_MINOR__ >= 8) || __GLIBC__>2) && defined(__LP64__)
#define USING_GLIBC_MALLOC 1
#if TACHY_CT_DEBUG
#warning "USING GLIBC ALREADY ALIGNED MALLOC"
#endif
#elif TACHY_SIMD_VERSION > 0
#define USING_MM_MALLOC 1
#if TACHY_CT_DEBUG
#warning "USING SIMD MM_MALLOC"
#endif
#else
#if TACHY_CT_DEBUG
#warning "NO STANDARD ALIGNED ALLOCATION AVAILABLE"
#endif
#endif

namespace tachy
{
      // From: http://jmabille.github.io/blog/2014/12/06/aligned-memory-allocator/       
      namespace detail
      {
            inline void* _aligned_malloc(size_t size, size_t alignment)
            {
                  void* res = 0;
                  void* ptr = malloc(size+alignment);
                  if (ptr != 0)
                  {
                        res = reinterpret_cast<void*>((reinterpret_cast<size_t>(ptr) & ~(size_t(alignment-1))) + alignment);
                        *(reinterpret_cast<void**>(res) - 1) = ptr;
                  }
                  return res;
            }

            inline void _aligned_free(void* ptr)
            {
                  if (ptr != 0)
                        free(*(reinterpret_cast<void**>(ptr)-1));
            }
      }

      inline void* aligned_malloc(size_t size, size_t alignment)
      {
#if USING_GLIBC_MALLOC
            return malloc(size);
#elif USING_MM_MALLOC
            return _mm_malloc(size, alignment);
#elif USING_POSIX_MEMALIGN
            void* res;
            const int failed = posix_memalign(&res,alignment,size);
            if (failed)
                  res = 0;
            return res;
#elif (defined _MSC_VER)
            return _aligned_malloc(size, alignment);
#else
            return detail::_aligned_malloc(size,alignment);
#endif
      }

      inline void aligned_free(void* ptr)
      {
#if (USING_GLIBC_MALLOC || USING_POSIX_MEMALIGN)
            free(ptr);
#elif USING_MM_MALLOC
            _mm_free(ptr);
#elif defined(_MSC_VER)
            _aligned_free(ptr);
#else
            detail::_aligned_free(ptr);
#endif
      }

      template <class T, int N>
      class aligned_allocator
      {
      public:
            typedef T value_type;
            typedef T& reference;
            typedef const T& const_reference;
            typedef T* pointer;
            typedef const T* const_pointer;
            typedef size_t size_type;
            typedef ptrdiff_t difference_type;

            template <class U>
            struct rebind
            {
                  typedef aligned_allocator<U,N> other;
            };

            inline aligned_allocator() throw() {}
            inline aligned_allocator(const aligned_allocator&) throw() {}

            template <class U>
            inline aligned_allocator(const aligned_allocator<U,N>&) throw() {}

            inline ~aligned_allocator() throw() {}

            inline pointer address(reference r) { return &r; }
            inline const_pointer address(const_reference r) const { return &r; }

            pointer allocate(size_type n, typename std::allocator<void>::const_pointer hint = 0)
            {
                  while (n % (N/sizeof(T)) != 0)
                        ++n;
                  pointer res = reinterpret_cast<pointer>(aligned_malloc(sizeof(T)*n, N));
                  if (res == 0)
                        throw std::bad_alloc();
                  return res;
            }
      
            inline void deallocate(pointer p, size_type)
            {
                  aligned_free(p);
            }

            inline void construct(pointer p, const_reference value) { new (p) value_type(value); }
            inline void destroy(pointer p) { p->~value_type(); }

            inline size_type max_size() const throw() { return size_type(-1) / sizeof(T); }
      
            inline bool operator==(const aligned_allocator&) { return true; }
            inline bool operator!=(const aligned_allocator& rhs) { return !operator==(rhs); }
      };
}

#endif // TACHY_ALIGNED_ALLOCATOR_H__INCLUDED

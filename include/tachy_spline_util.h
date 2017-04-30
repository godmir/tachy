#if !defined(TACHY_SPLINE_UTIL_H__INCLUDED)
#define TACHY_SPLINE_UTIL_H__INCLUDED

#include <numeric>
#include <limits>

#include "tachy_arch_traits.h"

namespace tachy
{
      template <typename NumType>
      struct spline_util
      {
            typedef std::pair<NumType, NumType> xy_pair_t;

            enum SPLINE_INIT_TYPE
            {
                  SPLINE_INIT_FROM_INCR_SLOPES,
                  SPLINE_INIT_FROM_LOCAL_SLOPES,
                  SPLINE_INIT_FROM_XY_POINTS
            };
            
            static inline NumType epsilon()
            {
                  return NumType(1e-6);
            }

            template <typename T>
            static inline T* allocate(unsigned int sz)
            {
                  void* p = aligned_malloc(sz*sizeof(T), arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE>::align);
                  return new (p) T[sz];
            }

            static inline NumType* allocate(unsigned int sz)
            {
                  void* p = aligned_malloc(sz*sizeof(NumType), arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE>::align);
                  return new (p) NumType[sz];
            }

            template <typename T>
            static inline void deallocate(T*& p)
            {
                  aligned_free(p);
                  p = 0;
            }
                  
            static inline unsigned int gcd(unsigned int u, unsigned int v)
            {
                  unsigned int s = 0;
                  while (u != v && u && v)
                  {
                        if ((u&1) == 0 && (v&1) == 0)
                        {
                              ++s;
                              u >>= 1;
                              v >>= 1;
                        }
                        else if ((u&1) == 0)
                              u >>= 1;
                        else if ((v&1) == 0)
                              v >>= 1;
                        else if (u > v)
                              u = (u - v)>>1;
                        else
                              v = (v - u)>>1;
                  }
                  return std::max(u, v)<<s;
            }
      };
}

#endif // TACHY_SPLINE_UTIL_H__INCUDED

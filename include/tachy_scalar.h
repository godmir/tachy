#if !defined(TACHY_SCALAR_H__INCLUDED)
#define TACHY_SCALAR_H__INCLUDED 1

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <cstring>

#include "tachy_arch_traits.h"
#include "tachy_calc_cache.h"


namespace tachy
{
      template <typename NumType>
      class scalar
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

            static std::string get_id(const NumType& x)
            {
                  const char lookup[] = "0123456789abcdef";
                  char s[(sizeof(NumType)<<1)+4];
                  s[0] = '0';
                  s[1] = 'x';
                  s[sizeof(s)-1] = '\0';
                  int j = 1;
                  for (int i = sizeof(NumType)-1; i >= 0; --i)
                  {
                        unsigned char c = reinterpret_cast<const char*>(&x)[i];
                        unsigned char h = c;
                        s[++j] = lookup[h>>4];
                        s[++j] = lookup[c&15];
                  }
                  s[++j] = '\0';
                  for ( --j; j > 1 && s[j] == '0'; --j)
                        s[j] = '\0';
                  return s;
            }

            scalar(const NumType& x)
            {
                  for (int i = 0; i < arch_traits_t::stride; ++i)
                        ((NumType*)(&_x))[i] = x;
            }

            scalar(const scalar& other) :
                  _x(other._x)
            {}

            typename arch_traits_t::packed_t get_packed(int /* idx */) const
            {
                  return _x;
            }

            typename arch_traits_t::packed_t& get_packed(int /* idx */)
            {
                  return _x;
            }

            NumType operator[] (int) const
            {
                  return ((NumType*)(&_x))[0];
            }

            unsigned int size() const
            {
                  return 0;
            }

            tachy_date get_start_date() const
            {
                  return tachy_date::min_date();
            }
            
      protected:
            typename arch_traits_t::packed_t _x;

            scalar& operator= (const scalar& other )
            {
                  return *this;
            }
      };

      // partial specialization of data_engine_traits for scalars
      template <typename T>
      struct data_engine_traits< scalar<T> >
      {
            typedef scalar<T> cached_engine_t;
            typedef scalar<T> ref_type_t;
      };
}

#endif // TACHY_SCALAR_H__INCLUDED

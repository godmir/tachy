#if !defined(TACHY_IOTA_ENGINE_H__INCLUDED)
#define TACHY_IOTA_ENGINE_H__INCLUDED

#include "tachy_arch_traits.h"
#include "tachy_date.h"

namespace tachy
{
      // IotaEngine - no storage

      template<typename NumType>  
      class iota_engine
      {
      public:
   	    typedef typename tachy::arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

            iota_engine(const tachy_date& start_date) :
                  _start_date(start_date),
                  _first(0),
                  _size(0)
            {}

            iota_engine(const tachy_date& start_date, int first, unsigned int n) :
                  _start_date(start_date),
                  _first(first),
                  _size(n)
            {}

            iota_engine(const iota_engine& other) :
                  _start_date(other._start_date),
                  _first(other._first),
                  _size(other._size)
            {}

            iota_engine& operator= (const iota_engine& other) = delete;

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  typename arch_traits_t::packed_t p;
                  for (int i = 0; i < arch_traits_t::stride; ++i)
			((typename arch_traits_t::scalar_t*)(&p))[i] = (typename arch_traits_t::scalar_t)(_first + idx + i);
                  return p;
            }

            int operator[] (int idx) const
            {
                  return _first + idx;
            }

            unsigned int size() const
            {
                  return _size;
            }

            tachy_date get_start_date() const
            {
                  return _start_date;
            }
            
      protected:
            tachy_date   _start_date;
            int          _first;
            unsigned int _size;
      };
}

#endif // TACHY_IOTA_ENGINE_H__INCLUDED

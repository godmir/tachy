#if !defined(TACHY_RANDOM_ENGINE_H__INCLUDED)
#define TACHY_RANDOM_ENGINE_H__INCLUDED

#include <cmath>

#include "tachy_arch_traits.h"

namespace tachy
{
      template <typename NumType>
      class random_engine
      {
      public:
            random_engine(NumType x0, NumType x1) : _mid(0.5*(x0 + x1)), _range(x1 - x0) {}
            random_engine(const random_engine<NumType>& other) : _mid(other._mid), _range(other._range) {}

            NumType operator[] (const unsigned int idx) const
            {
                  return _mid + _range*std::random();
            }

            unsigned int size() const
            {
                  return 0;
            }

      private:
            NumType _mid;
            NumType _range;

            // for consistency
            random_engine& operator= (const random_engine& other)
            {
                  return *this;
            }
      };
      
}

#endif // TACHY_RANDOM_ENGINE_H__INCLUDED

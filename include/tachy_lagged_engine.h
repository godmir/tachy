#if !defined(TACHY_LAGGED_ENGINE_H__INCLUDED)
#define TACHY_LAGGED_ENGINE_H__INCLUDED

#include "tachy_arch_traits.h"
#include "tachy_calc_cache.h"
#include "tachy_date.h"

namespace tachy
{
      // Note that lag is not a functor, because it works on the index into array, not array value at that index
      template <bool Checked> struct lag_checking_policy;

      template <> struct lag_checking_policy<true>
      {
            static int lag(int bound, int idx, int the_lag)
            {
                  return std::max<int>(bound, idx - the_lag);
            }
      };

      template <> struct lag_checking_policy<false>
      {
            static int lag(int, int idx, int the_lag)
            {
                  return idx - the_lag;
            }
      };
      
      template <typename NumType, typename Op, bool Checked>
      class lagged_engine
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            lagged_engine(const Op& op, int lag) : _op(op), _lag(lag) {}

            // Copying is suspect - it may lead to dangling references (see the type of _op variable)
            lagged_engine(const lagged_engine& other) : _op(other._op), _lag(other._lag) {}

            NumType operator[] (int idx) const
            {
                  return _op[lag_checking_policy<Checked>::lag(0, idx, _lag)]; // checking upper boundary is left for the operand itself
            }

            unsigned int size() const
            {
                  return _op.size();
            }

            tachy_date get_start_date() const
            {
                  return _op.get_start_date();
            }
            
            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return _op.get_packed(lag_checking_policy<Checked>::lag(0, idx, _lag));
            }

      protected:
            typename data_engine_traits<Op>::ref_type_t _op;
            int _lag;

            lagged_engine& operator= (const lagged_engine& other)
            {
                  if (this != &other)
                  {
                        _op = other._op;
                        _lag = other._lag;
                  }
                  return *this;
            }
      };
}

#endif // TACHY_LAGGED_ENGINE_H__INCLUDED

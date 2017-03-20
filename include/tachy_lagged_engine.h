#if !defined(TACHY_LAGGED_ENGINE_H__INCLUDED)
#define TACHY_LAGGED_ENGINE_H__INCLUDED

#include "tachy_arch_traits.h"
#include "tachy_calc_cache.h"

namespace tachy
{
      // Note that lag is not a functor, because it works on the index into array, not array value at that index
      template <bool Checked> struct lag_checking_policy;

      template <> struct lag_checking_policy<true>
      {
            static unsigned int lag(int idx, unsigned int the_lag)
            {
                  return std::max<int>(0, idx - the_lag);
            }
      };

      template <> struct lag_checking_policy<false>
      {
            static unsigned int lag(int idx, unsigned int the_lag)
            {
                  return idx - the_lag;
            }
      };
      
      template <typename NumType, typename Op, bool Checked>
      class lagged_engine
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            lagged_engine(const Op& op, unsigned int lag) : _op(op), _lag(lag) {}

            // Copying is suspect - it may lead to dangling references (see the type of _op variable)
            lagged_engine(const lagged_engine& other) : _op(other._op), _lag(other._lag) {}

            NumType operator[] (int idx) const
            {
                  return _op[lag_checking_policy<Checked>::lag(idx, _lag)]; // checking upper boundary is left for the operand itself
            }

            unsigned int size() const
            {
                  return _op.size();
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return _op.get_packed(lag_checking_policy<Checked>::lag(idx, _lag));
            }

      protected:
            typename data_engine_traits<Op>::ref_type_t _op;
            unsigned int _lag;

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

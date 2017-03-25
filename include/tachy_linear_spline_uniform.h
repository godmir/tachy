#if !defined(TACHY_LINEAR_SPLINE_UNIFORM_H__INCLUDED)
#define TACHY_LINEAR_SPLINE_UNIFORM_H__INCLUDED

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <numeric>
#include <limits>

#include <time.h>

#include "tachy_spline_util.h"
#include "tachy_functor.h"

namespace tachy
{
      template <typename NumType>
      class linear_spline_uniform
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
      private:
            std::string _key;

            unsigned int _size;

            NumType  _dx;
            NumType  _x0;
            NumType* _a;
            NumType* _b;

            typename arch_traits_t::packed_t _dx_packed;
            typename arch_traits_t::packed_t _x0_packed;

            void set_packed()
            {
                  _dx_packed = arch_traits_t::set1(_dx);
                  _x0_packed = arch_traits_t::set1(_x0);
            }
            
            void clear()
            {
                  spline_util<NumType>::deallocate(_a);
                  spline_util<NumType>::deallocate(_b);
                  _a = _b = 0;
                  _x0 = NumType(0);
                  _dx = NumType(0);
                  _size = 0;
            }

            void copy(const std::string& key, unsigned int size, NumType dx, const NumType x0, const NumType* a, const NumType* b)
            {
                  assert(_a == 0 && _b == 0);
                  _key = key;
                  _size = size;
                  _a = spline_util<NumType>::template allocate<NumType>(size);
                  _b = spline_util<NumType>::template allocate<NumType>(size);
                  _dx = dx;
                  _x0 = x0;
                  for (int i = 0; i < _size; ++i)
                  {
                        _a[i] = a[i];
                        _b[i] = b[i];
                  }
                  set_packed();
            }

      public:
            typedef linear_spline_uniform<NumType> spline_t;

            linear_spline_uniform() :
                  _key("DUMMY LSu"),
                  _a(0),
                  _b(0)
            {}
            
            linear_spline_uniform(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) throw(exception) :
                  _key("LSu_" + name),
                  _a(0),
                  _b(0)
            {
                  const NumType k_eps = spline_util<NumType>::epsilon();

                  std::set<unsigned int> deltas;
                  for (int i = 1; i < nodes.size(); ++i)
                        deltas.insert((unsigned int)((nodes[i].first - nodes[i-1].first)/k_eps + 0.5));

                  bool uniform = deltas.size() == 1;
                  NumType d0 = 0.0;
                  if (!uniform)
                  {
                        std::set<unsigned int>::const_iterator i = deltas.begin();
                        d0 = *i;
                        for (++i; i != deltas.end(); ++i)
                              d0 = spline_util<NumType>::gcd(d0, *i);
                        if (d0 > 0)
                              uniform = true;
                        else
                              TACHY_THROW("Cannot convert spline to uniform");
                  }

                  NumType delta = k_eps*d0;
                  _dx = 1.0/delta;
                  _x0 = nodes[0].first - delta;
                  int raw_size = nodes.size();
                  NumType x1 = nodes[raw_size-1].first + delta;
                  _size = (unsigned int)((x1 - _x0)*_dx + 0.5);
                  TACHY_LOG("Uniform grid size: " << _size);

                  _a = spline_util<NumType>::template allocate<NumType>(_size);
                  _b = spline_util<NumType>::template allocate<NumType>(_size);

                  NumType x = _x0 + 0.5*delta;
                  int idx = -1;
                  NumType slope = 0.0;
                  NumType addon = 0.0;
                  _a[0] = _b[0] = 0.0;
                  for (int k = 1; k < _size; ++k)
                  {
                        x += delta;
                        if (idx < raw_size && x > nodes[idx+1].first)
                        {
                              ++idx;
                              slope += nodes[idx].second;
                              addon -= nodes[idx].second*nodes[idx].first;
                        }
                        _b[k] = slope;
                        _a[k] = addon;
                  }

                  set_packed();
            }

            linear_spline_uniform(const linear_spline_uniform& other) :
                  _a(0),
                  _b(0)
            {
                  TACHY_LOG("Copying a uniform linear spline");
                  copy(other._key, other._size, other._dx, other._x0, other._a, other._b);
            }

            ~linear_spline_uniform()
            {
                  clear();
            }

            linear_spline_uniform& operator= (const linear_spline_uniform& other)
            {
                  if (this != &other)
                  {
                        clear();
                        copy(other._key, other._size, other._dx, other._x0, other._a, other._b);
                  }
                  return *this;
            }

            inline NumType operator()(NumType x) const
            {
                  int i = std::max<int>(0, std::min<int>(int((x - _x0)*_dx), _size-1));
                  return _a[i] + _b[i]*x;
            }

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  typename arch_traits_t::packed_t t = arch_traits_t::mul(_dx_packed, arch_traits_t::sub(x, _x0_packed));
                  typename arch_traits_t::index_t i = arch_traits_t::imin(int(_size-1), arch_traits_t::imax(0, arch_traits_t::cvti(arch_traits_t::floor(t))));
                  return arch_traits_t::fmadd(x, arch_traits_t::gather(_b, i), arch_traits_t::gather(_a, i));
            }
            
            template <class ArgEngine, unsigned int Level>
            calc_vector<NumType, functor_engine<NumType, ArgEngine, spline_t, Level>, Level> operator()(const calc_vector<NumType, ArgEngine, Level>& x) const
            {
                  typedef functor_engine<NumType, ArgEngine, spline_t, Level> engine_t;
                  std::string id = _key + x.get_id();
                  engine_t eng(x.engine(), *this);
                  return calc_vector<NumType, engine_t, Level>(id, x.get_start_date(), eng, x.cache());
            }
      };
}

#endif // TACHY_LINEAR_SPLINE_UNIFORM_H__INCLUDED

#ifndef TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED
#define TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED

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
      class linear_spline_uniform_index
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
      private:
            std::string _key;

            unsigned int _size;
            unsigned int _idx_size;
            
            NumType  _dx;
            NumType  _x0;
            NumType* _a;
            NumType* _b;

            unsigned int* _idx;
            
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
                  spline_util<NumType>::deallocate(_idx);
                  _x0 = NumType(0);
                  _dx = NumType(0);
                  _size = _idx_size = 0;
            }

            void copy(const std::string& key, unsigned int size, unsigned int uniform_size, NumType dx, const NumType x0, const NumType* a, const NumType* b, const unsigned int* idx)
            {
                  assert(_a == 0 && _b == 0 && _idx == 0);
                  _key = key;
                  _size = size;
                  _a = spline_util<NumType>::allocate(size);
                  _b = spline_util<NumType>::allocate(size);
                  _idx = spline_util<NumType>::allocate(uniform_size);
                  _dx = dx;
                  _x0 = x0;
                  memcpy(_a, a, _size*sizeof(NumType));
                  memcpy(_b, b, _size*sizeof(NumType));
                  memcpy(_idx, idx, uniform_size*sizeof(unsigned int));
                  set_packed();
            }

      public:
            typedef linear_spline_uniform_index<NumType> spline_t;

            linear_spline_uniform_index() :
                  _key("DUMMY LSui"),
                  _a(0),
                  _b(0),
                  _idx(0)
            {}
            
            linear_spline_uniform_index(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) :
                  _key("LSui_" + name),
                  _a(0),
                  _b(0),
                  _idx(0)
            {
                  const NumType k_eps = spline_util<NumType>::epsilon();

                  std::set<unsigned int> deltas;
                  for (int i = 1; i < nodes.size(); ++i)
                        deltas.insert((unsigned int)((nodes[i].first - nodes[i-1].first)/k_eps + 0.5));

#if defined(TACHY_VERBOSE)
                  TACHY_LOG("Got " << deltas.size() << " deltas");
                  for (std::set<unsigned int>::const_iterator j = deltas.begin(); j != deltas.end(); ++j)
                        TACHY_LOG(*j);
#endif

                  bool uniform = deltas.size() == 1;
                  double d0 = 0.0;
                  if (!uniform)
                  {
                        std::set<unsigned int>::const_iterator i = deltas.begin();
                        d0 = *i;
                        for (++i; i != deltas.end(); ++i)
                        {
                              d0 = spline_util<NumType>::gcd(d0, *i);
                        }
                        if (d0 > 0)
                        {
                              TACHY_LOG("uniform grid step: " << d0);
                              uniform = true;
                        }
                  }

                  TACHY_LOG("Uniform: " << (uniform ? "ok" : "NOT ok"));

                  if (!uniform)
                        throw std::string("Cannot convert spline to uniform");

                  NumType delta = k_eps*d0;
                  _dx = 1.0/delta;
                  _x0 = nodes.front().first - delta;

                  unsigned int raw_size = nodes.size();
                  _size = raw_size + 1;
                  _a = spline_util<NumType>::template allocate<NumType>(_size);
                  _b = spline_util<NumType>::template allocate<NumType>(_size);
                  _a[0] = _b[0] = 0.0;
                  for (int i = 1; i < _size; ++i)
                  {
                        _b[i] = _b[i-1] + nodes[i-1].second;
                        _a[i] = _a[i-1] - nodes[i-1].second*nodes[i-1].first;
                  }
                  
                  NumType x1 = nodes.back().first + delta;
                  _idx_size = (unsigned int)((x1 - _x0)*_dx + 0.5);
                  _idx = spline_util<NumType>::template allocate<unsigned>(_idx_size);
                  NumType x = _x0 + 0.5*delta;
                  unsigned int idx = 0;
                  _idx[0] = idx;
                  for (int k = 1; k < _idx_size; ++k)
                  {
                        x += delta;
                        if (idx < raw_size && x > nodes[idx].first)
                              ++idx;
                        _idx[k] = idx;
                  }

                  set_packed();
            }

            linear_spline_uniform_index(const linear_spline_uniform_index& other) :
                  _a(0),
                  _b(0),
                  _idx(0)
            {
                  TACHY_LOG("Copying a uniform linear spline");
                  copy(other._key,
                       other._size,
                       other._idx_size,
                       other._dx,
                       other._x0,
                       other._a,
                       other._b,
                       other._idx);
            }

            ~linear_spline_uniform_index()
            {
                  clear();
            }

            linear_spline_uniform_index& operator= (const linear_spline_uniform_index& other)
            {
                  if (this != &other)
                  {
                        clear();
                        copy(other._key,
                             other._size,
                             other._idx_size,
                             other._dx,
                             other._x0,
                             other._a,
                             other._b,
                             other._idx);
                  }
                  return *this;
            }

            inline NumType operator()(NumType x) const
            {
                  int i = _idx[std::max<int>(0, std::min<int>(int((x - _x0)*_dx), _idx_size-1))];
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

#endif // TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED

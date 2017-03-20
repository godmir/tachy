#ifndef TACHY_LINEAR_SPLINE_INCR_SLOPE_H__INCLUDED
#define TACHY_LINEAR_SPLINE_INCR_SLOPE_H__INCLUDED

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
      class linear_spline_incr_slope
      {
      private:
            std::string _key;

            unsigned int _size;

            NumType* _nodes;
            NumType* _slopes;

            void clear()
            {
                  spline_util<NumType>::deallocate(_slopes);
                  spline_util<NumType>::deallocate( _nodes);
                  _size = 0;
            }

            void copy(const std::string& key, unsigned int size, const NumType* n, const NumType* c)
            {
                  assert(_nodes == 0 && _slopes == 0);
                  _key = key;
                  _size = size;
                  _nodes = spline_util<NumType>::allocate(size);
                  _slopes = spline_util<NumType>::allocate(size);
                  for (int i = 0; i < _size; ++i)
                  {
                        _nodes[i] = n[i];
                        _slopes[i] = c[i];
                  }
            }

      public:
            typedef linear_spline_incr_slope<NumType> spline_t;
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
            linear_spline_incr_slope(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) :
                  _key("LSis_" + name),
                  _size(nodes.size()),
                  _nodes(0),
                  _slopes(0)
            {
                  _nodes = spline_util<NumType>::allocate(_size);
                  _slopes = spline_util<NumType>::allocate(_size);
                  for (int i = 0; i < _size; ++i)
                  {
                        _nodes[i] = nodes[i].first;
                        _slopes[i] = nodes[i].second;
                  }
            }

            linear_spline_incr_slope(const linear_spline_incr_slope& other) :
                  _nodes(0),
                  _slopes(0)
            {
                  copy(other._key, other._size, other._nodes, other._slopes);
            }

            ~linear_spline_incr_slope()
            {
                  clear();
            }

            linear_spline_incr_slope& operator= (const linear_spline_incr_slope& other)
            {
                  if (this != &other)
                  {
                        clear();
                        copy(other._key, other._size, other._nodes, other._slopes);
                  }
                  return *this;
            }

            inline NumType operator()(NumType x) const
            {
                  double y = 0.0;
                  for (int i = 0; i < _size; ++i)
                        y += _slopes[i]*std::max<NumType>(0.0, x - _nodes[i]);
                  return y;
            }

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  typename arch_traits_t::packed_t y = arch_traits_t::zero();
                  for (int i = 0; i < _size; ++i)
                  {
                        typename arch_traits_t::packed_t t = arch_traits_t::sub(x, arch_traits_t::set1(_nodes[i]));
                        t = arch_traits_t::mul(arch_traits_t::set1(_slopes[i]), arch_traits_t::max(arch_traits_t::zero(), t));
                        y = arch_traits_t::add(y, t);
                  }
                  return y;
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

#endif // TACHY_LINEAR_SPLINE_INCR_SLOPE_H__INCLUDED

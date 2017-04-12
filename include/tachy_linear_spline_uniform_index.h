#if !defined(TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED)
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
#include "tachy_cacheable.h"

namespace tachy
{
      template <typename NumType>
      class linear_spline_uniform_index_base : public cacheable
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
      protected:
            std::string _key;

            unsigned int _size;
            unsigned int _idx_size;
            unsigned int _num_slices;
            
            NumType  _dx;
            NumType  _x0;

            NumType* _a;
            NumType* _b;

            unsigned int* _idx;

            typedef typename arch_traits_t::packed_t packed_t;
            char _buf[3*sizeof(packed_t)/sizeof(char)];
            packed_t* _dx_packed;
            packed_t* _x0_packed;

            void set_packed()
            {
                  if (_dx_packed == 0)
                  {
                        memset(_buf, 0, sizeof(_buf));
                        _dx_packed = (packed_t*)(&_buf[sizeof(packed_t)/sizeof(char)] - ((size_t)_buf)%arch_traits_t::align);
                        _x0_packed = _dx_packed + 1;
                  }
                  *_dx_packed = arch_traits_t::set1(_dx);
                  *_x0_packed = arch_traits_t::set1(_x0);
            }
            
            void clear()
            {
                  spline_util<NumType>::deallocate(_a);
                  spline_util<NumType>::deallocate(_b);
                  spline_util<NumType>::deallocate(_idx);
                  _x0 = NumType(0);
                  _dx = NumType(0);
                  _size = _idx_size = _num_slices = 0;
            }

            void static_copy(const std::string& key, const linear_spline_uniform_index_base& other)
            {
                  assert(_idx == 0);
                  _key = key;
                  _size = other._size;
                  _idx_size = other._idx_size;
                  _num_slices = other._num_slices;
                  _idx = spline_util<NumType>::template allocate<unsigned int>(_idx_size);
                  _dx = other._dx;
                  _x0 = other._x0;
                  memcpy(_idx, other._idx, _idx_size*sizeof(_idx[0]));
                  set_packed();
            }
            
            void copy(const linear_spline_uniform_index_base& other)
            {
                  assert(_a == 0 && _b == 0);
                  static_copy(other._key, other);
                  _a = spline_util<NumType>::allocate(_size);
                  _b = spline_util<NumType>::allocate(_size);
                  memcpy(_a, other._a, _size*sizeof(NumType));
                  memcpy(_b, other._b, _size*sizeof(NumType));
            }

            inline unsigned int get_index(NumType x) const
            {
                  return _idx[std::max<int>(0, std::min<int>(int((x - _x0)*_dx), _idx_size-1))];
            }

            inline typename arch_traits_t::index_t get_packed_index(const packed_t& x) const
            {
                  packed_t t = arch_traits_t::mul(*_dx_packed, arch_traits_t::sub(x, *_x0_packed));
                  return arch_traits_t::igather((const int*)_idx,
                                                arch_traits_t::imin(int(_idx_size-1), arch_traits_t::imax(0, arch_traits_t::cvti(arch_traits_t::floor(t)))));
            }
            
      public:
            linear_spline_uniform_index_base() :
                  _key("DUMMY LSui"),
                  _num_slices(1),
                  _a(0),
                  _b(0),
                  _idx(0),
                  _dx_packed(0),
                  _x0_packed(0)
            {}
            
            linear_spline_uniform_index_base(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) throw(exception) :
                  _key("LSui_" + name),
                  _num_slices(1),
                  _a(0),
                  _b(0),
                  _idx(0),
                  _dx_packed(0),
                  _x0_packed(0)
            {
                  const NumType k_eps = spline_util<NumType>::epsilon();

                  std::set<unsigned int> deltas;
                  for (int i = 1; i < nodes.size(); ++i)
                        deltas.insert((unsigned int)((nodes[i].first - nodes[i-1].first)/k_eps + 0.5));

                  bool uniform = deltas.size() == 1;
                  NumType d0 = *deltas.begin();
                  if (!uniform)
                  {
                        std::set<unsigned int>::const_iterator i = deltas.begin();
                        for (++i; i != deltas.end(); ++i)
                              d0 = spline_util<NumType>::gcd(d0, *i);
                        if (d0 > 0)
                              uniform = true;
                        else
                              TACHY_THROW("Cannot convert spline to uniform");
                  }

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
                  _idx = spline_util<NumType>::template allocate<unsigned int>(_idx_size);
                  NumType x = _x0 + 0.5*delta;
                  unsigned int i = 0;
                  _idx[0] = i;
                  for (int k = 1; k < _idx_size; ++k)
                  {
                        x += delta;
                        if (i < raw_size && x > nodes[i].first)
                              ++i;
                        _idx[k] = i;
                  }

                  set_packed();
            }

            linear_spline_uniform_index_base(const linear_spline_uniform_index_base& other) :
                  _num_slices(1),
                  _a(0),
                  _b(0),
                  _idx(0),
                  _dx_packed(0),
                  _x0_packed(0)
            {
                  copy(other);
            }

            virtual ~linear_spline_uniform_index_base()
            {
                  clear();
            }

            std::string get_id() const
            {
                  return _key;
            }

            unsigned int get_num_nodes() const
            {
                  return _size - 1; // because we added a node prior to the first one to streamline calculations
            }

            const NumType* const get_slopes() const
            {
                  return _b;
            }

            const NumType* const get_intercepts() const
            {
                  return _a;
            }
      };

      template <typename NumType, bool TimeDependent = false>
      class linear_spline_uniform_index : public linear_spline_uniform_index_base<NumType>
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef linear_spline_uniform_index_base<NumType> base_t;
            typedef linear_spline_uniform_index<NumType, TimeDependent> spline_t;

            linear_spline_uniform_index()
            {
                  base_t::_key = "DUMMY LSuin";
            }

            linear_spline_uniform_index(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) throw(exception) :
                  base_t(name, nodes)
            {
                  base_t::_key = "LSuin_" + name;
            }

            linear_spline_uniform_index(const spline_t& other) :
                  base_t(other)
            {}

            spline_t& operator= (const spline_t& other)
            {
                  if (this != &other)
                  {
                        base_t::clear();
                        base_t::copy(other);
                  }
                  return *this;
            }

            virtual spline_t* clone() const
            {
                  return new spline_t(*this);
            }
            
            inline NumType operator()(NumType x) const
            {
                  unsigned int i = base_t::get_index(x);
                  return base_t::_a[i] + base_t::_b[i]*x;
            }

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  typename arch_traits_t::index_t i = base_t::get_packed_index(x);
                  return arch_traits_t::fmadd(x, arch_traits_t::gather(base_t::_b, i), arch_traits_t::gather(base_t::_a, i));
            }
            
            template <class ArgEngine, unsigned int Level>
            calc_vector<NumType, functor_engine<NumType, ArgEngine, spline_t, Level>, Level> operator()(const calc_vector<NumType, ArgEngine, Level>& x) const
            {
                  typedef functor_engine<NumType, ArgEngine, spline_t, Level> engine_t;
                  std::string id = base_t::_key + x.get_id();
                  return calc_vector<NumType, engine_t, Level>(id, x.get_start_date(), engine_t(x.engine(), *this), x.cache());
            }

            template <class ArgEngine>
            calc_vector<NumType, functor_engine<NumType, ArgEngine, spline_t, 0>, 0> operator()(const calc_vector<NumType, ArgEngine, 0>& x) const
            {
                  typedef functor_engine<NumType, ArgEngine, spline_t, 0> engine_t;
                  std::string id = base_t::_key + x.get_id();
                  return calc_vector<NumType, engine_t, 0>(id, x.get_start_date(), engine_t(x.engine(), *this), x.cache());
            }
      };

      template <typename NumType>
      class linear_spline_uniform_index<NumType, true> : public linear_spline_uniform_index_base<NumType>
      {
      private:
            void resize(unsigned int mod_size)
            {
                  base_t::_num_slices = mod_size;
                  base_t::_a = spline_util<NumType>::allocate(base_t::_size*base_t::_num_slices);
                  base_t::_b = spline_util<NumType>::allocate(base_t::_size*base_t::_num_slices);
            }
            
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef linear_spline_uniform_index_base<NumType> base_t;
            typedef linear_spline_uniform_index<NumType, true> spline_t;

            linear_spline_uniform_index()
            {
                  base_t::_key = "DUMMY LSuiy";
            }

            linear_spline_uniform_index(const std::string& name, const std::vector<typename spline_util<NumType>::xy_pair_t>& nodes) throw(exception) :
                  base_t(name, nodes)
            {
                  base_t::_key = "LSuiy_" + name;
            }

            linear_spline_uniform_index(const spline_t& other) :
                  base_t(other)
            {}

            template <class ModVector>
            static std::string generate_id(const std::string& base_id, const std::vector<ModVector>& modulation)
            {
                  std::ostringstream id;
                  id << "LSuiy_mod[";
                  for (typename std::vector<ModVector>::const_iterator mod = modulation.begin(); mod != modulation.end(); ++mod)
                        id << mod->get_id() << " ";
                  id << "]" << base_id;
                  return id.str();
            }

            template <class ModVector>
            linear_spline_uniform_index(const linear_spline_uniform_index<NumType, false>& base,
                                        const std::vector<ModVector>& modulation) throw(exception)
            {
                  if (modulation.size() != base.get_num_nodes())
                        TACHY_THROW("Incorrect modulation size: got " << modulation.size() << ", expected: " << base.get_num_nodes());
                  unsigned int mod_size = modulation[0].size();
                  for (typename std::vector<ModVector>::const_iterator mod = modulation.begin(); mod != modulation.end(); ++mod)
                  {
                        if (mod->size() != mod_size)
                              TACHY_THROW("Modulation vector lengths are inconsistent: " << mod->size() << " vs " << mod_size);
                  }

                  const std::string key = spline_t::generate_id(base.get_id(), modulation);
                  base_t::static_copy(key, base);
                  resize(mod_size);
                  const NumType* const orig_a = base.get_intercepts();
                  const NumType* const orig_b = base.get_slopes();
                  for (int i = 0, k = 0; i < mod_size; ++i, k += base_t::_size)
                  {
                        base_t::_a[k+0] = base_t::_b[k+0] = NumType(0);

                        base_t::_b[k+1] = modulation[0][i]*orig_b[1];
                        for (int j = 2; j < base_t::_size; ++j)
                              base_t::_b[k+j] = base_t::_b[k+j-1] + modulation[j-1][i]*(orig_b[j] - orig_b[j-1]);

                        base_t::_a[k+1] = modulation[0][i]*orig_a[1];
                        for (int j = 2; j < base_t::_size; ++j)
                              base_t::_a[k+j] = base_t::_a[k+j-1] + modulation[j-1][i]*(orig_a[j] - orig_a[j-1]);
                  }
            }
            
            spline_t& operator= (const spline_t& other)
            {
                  if (this != &other)
                  {
                        base_t::clear();
                        base_t::copy(other);
                  }
                  return *this;
            }

            virtual spline_t* clone() const
            {
                  return new spline_t(*this);
            }
            
            inline NumType operator()(int t, NumType x) const
            {
                  unsigned int i = t*base_t::_size + base_t::get_index(x);
                  return base_t::_a[i] + base_t::_b[i]*x;
            }

            inline typename arch_traits_t::packed_t apply_packed(int t, const typename arch_traits_t::packed_t& x) const
            {
                  typename arch_traits_t::index_t i = arch_traits_t::iadd(arch_traits_t::iset1(t*base_t::_size), base_t::get_packed_index(x));
                  return arch_traits_t::fmadd(x, arch_traits_t::gather(base_t::_b, i), arch_traits_t::gather(base_t::_a, i));
            }

            template <class ArgEngine, unsigned int Level>
            calc_vector<NumType, functor_engine<NumType, ArgEngine, spline_t, Level, time_dep_functor_call_policy<NumType, ArgEngine, spline_t>, functor_obj_policy_ref<spline_t> >, Level> operator()(const calc_vector<NumType, ArgEngine, Level>& x) const
            {
                  typedef functor_engine<NumType, ArgEngine, spline_t, Level, time_dep_functor_call_policy<NumType, ArgEngine, spline_t>, functor_obj_policy_ref<spline_t> > engine_t;
                  std::string id = base_t::_key + x.get_id();
                  return calc_vector<NumType, engine_t, Level>(id, x.get_start_date(), engine_t(x.engine(), *this), x.cache());
            }
      };
}

#endif // TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED

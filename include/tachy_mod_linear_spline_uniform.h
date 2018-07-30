#if !defined(TACHY_MOD_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED)
#define TACHY_MOD_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED

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
#include "tachy_calc_cache.h"
#include "tachy_linear_spline_uniform_index.h"
#include "tachy_vector_engine.h"

namespace tachy
{
      template <typename NumType, unsigned int CacheLevel>
      class mod_linear_spline_uniform_index
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef mod_linear_spline_uniform_index<NumType, CacheLevel> self_t;
            typedef linear_spline_uniform_index<NumType, false> base_spline_t;
            typedef linear_spline_uniform_index<NumType, true>  spline_t;
            typedef calc_cache<NumType, CacheLevel> cache_t;
            
      private:
            const spline_t* _spline;
            cache_t* _cache;

            bool _own_memory;

            void clear()
            {
                  if (_own_memory && _spline)
                  {
                        const std::string key = _spline->get_id();
                        typename cache_t::cache_engine_t::const_iterator it = _cache->find(key);
                        if (it == _cache->end())
                              (*_cache)[key] = const_cast<spline_t*>(_spline);
                        else
                              delete _spline;
                  }
            }

            mod_linear_spline_uniform_index() {} // no default c'tor
            
      public:

            template <class ModVector>
            mod_linear_spline_uniform_index(const base_spline_t& base, const std::vector<ModVector>& modulation) :
                  _spline(0),
                  _cache(0),
                  _own_memory(true)
            {
                  _cache = &modulation[0].cache();
                  for (typename std::vector<ModVector>::const_iterator mod = modulation.begin(); mod != modulation.end(); ++mod)
                  {
                        if (&mod->cache() != _cache)
                              TACHY_THROW("Modulation vector cache objects are inconsistent");
                  }
                  const std::string key = spline_t::generate_id(base.get_id(), modulation);
                  typename cache_t::cache_engine_t::const_iterator it = _cache->find(key);
                  if (it != _cache->end())
                  {
                        _spline = dynamic_cast<const spline_t*>(it->second);
                        _own_memory = false;
                  }
                  else
                  {
                        _spline = new spline_t(base, modulation);
                        _own_memory = true;
                  }
            }

            mod_linear_spline_uniform_index(const mod_linear_spline_uniform_index& other) :
                  _spline(other._spline),
                  _cache(other._cache),
                  _own_memory(false)
            {}

            ~mod_linear_spline_uniform_index()
            {
                  clear();
            }

            self_t& operator= (const self_t& other)
            {
                  if (this != &other)
                  {
                        clear();
                        _spline = other._spline;
                        _cache  = other._cache;
                        _own_memory = false;
                  }
                  return *this;
            }

            inline NumType operator()(int t, NumType x) const
            {
                  return (*_spline)(t, x);
            }

            inline typename arch_traits_t::packed_t apply_packed(int t, const typename arch_traits_t::packed_t& x) const
            {
                  return _spline->apply_packed(t, x);
            }
            
            template <class ArgEngine, unsigned int Level>
            calc_vector<NumType, functor_engine<NumType, ArgEngine, spline_t, Level, time_dep_functor_call_policy<NumType, ArgEngine, spline_t>, functor_obj_policy_ref<spline_t> >, Level> operator()(const calc_vector<NumType, ArgEngine, Level>& x) const
            {
                  typedef functor_engine<NumType, ArgEngine, spline_t, Level, time_dep_functor_call_policy<NumType, ArgEngine, spline_t>, functor_obj_policy_ref<spline_t> > engine_t;
                  std::string id = _spline->get_id() + x.get_id();
                  return calc_vector<NumType, engine_t, Level>(id, x.get_start_date(), engine_t(x.engine(), *_spline), x.cache());
            }
      };
}

#endif // TACHY_LINEAR_SPLINE_UNIFORM_INDEX_H__INCLUDED

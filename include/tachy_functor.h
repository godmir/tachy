#if !defined(TACHY_FUNCTOR_H__INCLUDED)
#define TACHY_FUNCTOR_H__INCLUDED

#include "tachy_vector.h"

namespace tachy
{
      template <typename NumType, class Arg, class Functor>
      struct simple_functor_call_policy
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
            static inline NumType call(unsigned int i, const Arg& arg, const Functor& fct)
            {
                  return fct(arg[i]);
            }

            static inline typename arch_traits_t::packed_t call_packed(unsigned int i, const Arg& arg, const Functor& fct)
            {
                  return fct.apply_packed(arg.get_packed(i));
            }
      };

      template <typename NumType, class Arg, class Functor>
      struct time_dep_functor_call_policy
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
            static inline NumType call(unsigned int i, const Arg& arg, const Functor& fct)
            {
                  return fct(i, arg[i]);
            }

            static inline typename arch_traits_t::packed_t call_packed(unsigned int i, const Arg& arg, const Functor& fct)
            {
                  return fct.apply_packed(i, arg.get_packed(i));
            }
      };


      // The reasoning here:
      // some functors are cheap to create (min/max - only 1 floating number), and created off temp objects
      // others are expensive (e.g. TD linear spline with modulation)
      
      template <class Functor>
      struct functor_obj_policy_copy
      {
            typedef Functor held_functor_obj_t;
            typedef Functor const held_const_functor_obj_t;
      };

      template <class Functor>
      struct functor_obj_policy_ref
      {
            typedef Functor& held_functor_obj_t;
            typedef Functor const& held_const_functor_obj_t;
      };
      

      // CalcVector Engine for non-static functors - e.g. various Linear Spline objects
      // These classes must implement a method "NumType operator()(NumType) const" -- for time-independent functors
      // OR they must implement "NumType operator()(int, NumType) const" -- for time-dependent functors

      template <typename NumType,
                typename Arg,
                class Functor,
                unsigned int Level,
                class FcnCallPolicy = simple_functor_call_policy<NumType, Arg, Functor>,
                class FunctorObjPolicy = functor_obj_policy_copy<Functor> >
      class functor_engine
      {
      private:
            typedef vector_engine<NumType> data_engine_t;
            typedef calc_cache<NumType, Level> cache_t;
            
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
            functor_engine(const std::string& key, const Arg& arg, const Functor& fct, calc_cache<NumType, Level>& cache) :
                  _cache(cache),
                  _id(key),
                  _engine(nullptr),
                  _own_engine(true)
            {
                  const auto k = _cache.find(key);
                  if (k == _cache.end())
                  {
                        TACHY_LOG("Cache " << cache.get_id() << ": calculating for " << key);
                        unsigned int sz = arg.size();
                        _engine = new data_engine_t(sz, NumType(0));
                        for (int i = 0; i < sz; ++i)
                              (*_engine)[i] = FcnCallPolicy::call(i, arg, fct);
                  }
                  else
                  {
                        TACHY_LOG("Cache " << cache.get_id() << ": using cached result for " << key);
                        _engine = dynamic_cast<data_engine_t*>(k->second);
                        _own_engine = false;
                  }
            }

            functor_engine(const functor_engine& other) :
                  _cache(other._cache),
                  _id(other._id),
                  _engine(other._engine),
                  _own_engine(other._engine ? false : true)
            {}

            ~functor_engine()
            {
                  if (_own_engine)
                  {
                        auto k = _cache.find(_id);
                        if (k == _cache.end())
                              _cache[_id] = _engine;
                        else
                              delete _engine;
                  }
            }

            NumType operator[] (const unsigned int idx) const
            {
                  return (*_engine)[idx];
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return arch_traits_t::loadu(&(*_engine)[idx]);
            }

            unsigned int size() const
            {
                  return _engine->size();
            }

      protected:
            cache_t& _cache;
            std::string _id;
            data_engine_t* _engine;
            bool _own_engine;

            functor_engine& operator= (const functor_engine& other)
            {
                  return *this;
            }
      };

      template <typename NumType, typename Arg, class Functor, class FcnCallPolicy, class FunctorObjPolicy>
      class functor_engine<NumType, Arg, Functor, 0, FcnCallPolicy, FunctorObjPolicy>
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            functor_engine(const Arg& arg, const Functor& fct)
                  : _arg(arg),
                    _fct(fct)
            {}
            functor_engine(const functor_engine& other)
                  : _arg(other._arg),
                    _fct(other._fct)
            {}
            
            NumType operator[] (unsigned int idx) const
            {
                  return FcnCallPolicy::call(idx, _arg, _fct);
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return FcnCallPolicy::call_packed(idx, _arg, _fct);
            }

            unsigned int size() const
            {
                  return _arg.size();
            }

      protected:
            typename data_engine_traits<Arg>::ref_type_t _arg;

            typename FunctorObjPolicy::held_const_functor_obj_t _fct;

            functor_engine& operator= (const functor_engine&)
            {
                  return *this;
            }
      };

      template <typename NumType,
                typename Arg,
                class Functor,
                unsigned int Level,
                class FcnCallPolicy = simple_functor_call_policy<NumType, Arg, Functor>,
                class FunctorObjPolicy = functor_obj_policy_copy<Functor> >
      class functor_engine_delayed_cache
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef calc_cache<NumType, Level> cache_t;

            functor_engine_delayed_cache(const std::string& key, const Arg& arg, const Functor& fct, cache_t& cache)
                  : _key(key),
                    _arg(arg),
                    _fct(fct),
                    _cache(cache),
                    _cached_vector(0)
            {}

            functor_engine_delayed_cache(const functor_engine_delayed_cache& other)
                  : _key(other._key),
                    _arg(other._arg),
                    _fct(other._fct),
                    _cache(other._cache),
                    _cached_vector(0)
            {}

            ~functor_engine_delayed_cache()
            {
                  TACHY_LOG("DEBUG: destroying functor_engine_delayed_cache " << _key);
                  delete _cached_vector;
            }

            NumType operator[] (const unsigned int idx) const
            {
                  return FcnCallPolicy::call(idx, _arg, _fct);
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return FcnCallPolicy::call_packed(idx, _arg, _fct);
            }

            unsigned int size() const
            {
                  return _arg.size();
            }

            const functor_engine<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy>& get_cached_engine() const
            {
                  if (0 == _cached_vector)
                        const_cast<functor_engine_delayed_cache*>(this)->_cached_vector = new functor_engine<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy>(_key, _arg, _fct, _cache);
                  return *_cached_vector;
            }

      protected:
            std::string _key;
            typename data_engine_traits<Arg>::ref_type_t _arg;
            typename FunctorObjPolicy::held_const_functor_obj_t _fct;
            cache_t& _cache;
            functor_engine<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy>* _cached_vector;

            functor_engine_delayed_cache& operator= (const functor_engine_delayed_cache& other )
            {
                  return *this;
            }
      };

      template <typename NumType, class Arg, class Functor, unsigned int Level, class FcnCallPolicy, class FunctorObjPolicy>
      const functor_engine<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy>& do_cache(const functor_engine_delayed_cache<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy>& eng)
      {
            return eng.get_cached_engine();
      }

      template <typename NumType, class Arg, class Functor, unsigned int Level, class FcnCallPolicy, class FunctorObjPolicy>
      struct data_engine_traits< functor_engine_delayed_cache<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy> >
      {
            typedef functor_engine<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy> cached_engine_t;
            typedef functor_engine_delayed_cache<NumType, Arg, Functor, Level, FcnCallPolicy, FunctorObjPolicy> const& ref_type_t;
      };

      
      template <typename NumType>
      class exp_functor
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            inline NumType operator()(NumType x) const
            {
                  return std::exp(x);
            }

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  return arch_traits_t::exp(x);
            }
            
            template <class Engine, unsigned int Level>
            calc_vector<NumType, functor_engine<NumType, Engine, exp_functor<NumType>, Level>, Level> operator()(const calc_vector<NumType, Engine, Level>& x) const
            {
                  typedef functor_engine<NumType, Engine, exp_functor<NumType>, Level> engine_t;
                  std::string hashed_id = x.cache().get_hash_key(_key + x.get_id());
                  engine_t eng(hashed_id, x.engine(), *this, x.cache());
                  return calc_vector<NumType, engine_t, Level>(hashed_id, x.get_start_date(), eng, x.cache());
            }

            exp_functor()
                  : _key("EXP")
            {}

      private:
            const std::string _key;
      };

      template <typename NumType>
      class min_functor
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            
            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  return arch_traits_t::min(_upper_packed, x);
            }

            inline NumType operator()(NumType x) const
            {
                  return std::min<NumType>(_upper_bound, x);
            }

            explicit min_functor(NumType upper_bound)
                  : _key("MIN_"),
                    _upper_bound(upper_bound),
                    _upper_packed(arch_traits_t::set1(upper_bound))
            {}

            const std::string& get_id() const
            {
                  return _key;
            }

      private:
            const std::string _key;
            const NumType _upper_bound;
            const typename arch_traits_t::packed_t _upper_packed;
      };
      
      template <typename NumType>
      class max_functor
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  return arch_traits_t::max(_lower_packed, x);
            }

            inline NumType operator()(NumType x) const
            {
                  return std::max<NumType>(_lower_bound, x);
            }

            explicit max_functor(NumType lower_bound)
                  : _key("MAX_"),
                    _lower_bound(lower_bound),
                    _lower_packed(arch_traits_t::set1(lower_bound))
            {}

            const std::string& get_id() const
            {
                  return _key;
            }

      private:
            const std::string _key;
            const NumType _lower_bound;
            const typename arch_traits_t::packed_t _lower_packed;
      };
      
      template <typename NumType>
      class min_max_functor
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;

            inline typename arch_traits_t::packed_t apply_packed(const typename arch_traits_t::packed_t& x) const
            {
                  return arch_traits_t::max(_lower_packed, arch_traits_t::min(x, _upper_packed));
            }
            
            inline NumType operator()(NumType x) const
            {
                  return std::max<NumType>(_lower, std::min<NumType>(x, _upper));
            }

            min_max_functor(NumType lower, NumType upper) :
                  _key("MINMAX_"),
                  _lower(lower),
                  _upper(upper),
                  _lower_packed(arch_traits_t::set1(lower)),
                  _upper_packed(arch_traits_t::set1(upper))
            {}

            const std::string& get_id() const
            {
                  return _key;
            }

      private:
            const std::string _key;
            const NumType _lower;
            const NumType _upper;
            const typename arch_traits_t::packed_t _lower_packed;
            const typename arch_traits_t::packed_t _upper_packed;
      };

#define TACHY_BINARY_FUNCTOR_PACK(FUNC_TYPE, FUNC_NAME) \
      template <typename NumType, class Engine, unsigned int Level> \
      calc_vector<NumType, functor_engine<NumType, Engine, FUNC_TYPE, Level>, Level> FUNC_NAME (NumType param, const calc_vector<NumType, Engine, Level>& x) \
      { \
            typedef functor_engine<NumType, Engine, FUNC_TYPE, Level> engine_t; \
            FUNC_TYPE bf(param); \
            std::string hashed_id = x.cache().get_hash_key(bf.get_id() + scalar<NumType>::get_id(param) + std::string("_") + x.get_id()); \
            engine_t eng(hashed_id, x.engine(), bf, x.cache()); \
            return calc_vector<NumType, engine_t, Level>(hashed_id, x.get_start_date(), eng, x.cache()); \
      } \
      \
      template <typename NumType, class Engine, unsigned int Level> \
      calc_vector<NumType, functor_engine<NumType, Engine, FUNC_TYPE, Level>, Level> FUNC_NAME (const calc_vector<NumType, Engine, Level>& x, NumType param) \
      { \
            return FUNC_NAME(param, x); \
      } \
      \
      template <typename NumType, class Engine> \
      calc_vector<NumType, functor_engine<NumType, Engine, FUNC_TYPE, 0>, 0> FUNC_NAME (NumType param, const calc_vector<NumType, Engine, 0>& x) \
      { \
            typedef functor_engine<NumType, Engine, FUNC_TYPE, 0> engine_t; \
            FUNC_TYPE bf(param); \
            std::string hashed_id = calc_cache<NumType, 0>::get_dummy_key(); \
            engine_t eng(x.engine(), bf); \
            return calc_vector<NumType, engine_t, 0>(hashed_id, x.get_start_date(), eng, x.cache()); \
      } \
      \
      template <typename NumType, class Engine> \
      calc_vector<NumType, functor_engine<NumType, Engine, FUNC_TYPE, 0>, 0> FUNC_NAME (const calc_vector<NumType, Engine, 0>& x, NumType param) \
      { \
            return FUNC_NAME(param, x); \
      }
// end #define TACHY_BINARY_FUNCTOR_PACK(FUNC_TYPE, FUNC_NAME)

      TACHY_BINARY_FUNCTOR_PACK(min_functor<NumType>, min)
      TACHY_BINARY_FUNCTOR_PACK(max_functor<NumType>, max)

      template <typename NumType, class Engine, unsigned int Level>
      calc_vector<NumType, functor_engine<NumType, Engine, min_max_functor<NumType>, Level>, Level> min_max(NumType lower, const calc_vector<NumType, Engine, Level>& x, NumType upper)
      {
            typedef functor_engine<NumType, Engine, min_max_functor<NumType>, Level> engine_t;
            min_max_functor<NumType> mmf(lower, upper);
            std::string hashed_id = x.cache().get_hash_key(mmf.get_id() + scalar<NumType>::get_id(lower) + std::string("_") + scalar<NumType>::get_id(upper) + std::string("_") + x.get_id());
            engine_t eng(hashed_id, x.engine(), mmf, x.cache());
            return calc_vector<NumType, engine_t, Level>(hashed_id, x.get_start_date(), eng, x.cache());
      }

      template <typename NumType, class Engine>
      calc_vector<NumType, functor_engine<NumType, Engine, min_max_functor<NumType>, 0>, 0> min_max(NumType lower, const calc_vector<NumType, Engine, 0>& x, NumType upper)
      {
            typedef functor_engine<NumType, Engine, min_max_functor<NumType>, 0> engine_t;
            min_max_functor<NumType> mmf(lower, upper);
            std::string hashed_id = calc_cache<NumType, 0>::get_dummy_key();
            engine_t eng(x.engine(), mmf);
            return calc_vector<NumType, engine_t, 0>(hashed_id, x.get_start_date(), eng, x.cache());
      }
}

#endif // TACHY_FUNCTOR_H__INCLUDED

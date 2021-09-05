#if !defined(TACHY_STATIC_FUNCTOR_ENGINE_H__INCLUDED)
#define TACHY_STATIC_FUNCTOR_ENGINE_H__INCLUDED

#include "tachy_vector.h"

namespace tachy
{
      // CalcVector for StaticFunctors
      // StaticFunctor must have a static method "apply()" that takes 1 NumType variable and returns a NumType
      // e.g. it can be a thin wrapper around standard functions like exp(x) etc

      template <typename NumType, typename Op, class StaticFunctor, unsigned int Level>
      class static_functor_engine
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef StaticFunctor func_t;

            static_functor_engine(const std::string& key, const Op& op, calc_cache<NumType, Level>& cache) :
                  _key(key),
                  _res(dynamic_cast<vector_engine<NumType>*>(cache[key]))
            {
                  if (0 == _res) // the expectation is: either it's cached - then it's size > 0, or it's not, then ptr is 0
                  {
                        _res = new vector_engine<NumType>(op.size(), NumType(0));
                        cache[key] = _res;
                        TACHY_LOG("Cache " << cache.get_id() << ": calculating for " << key);
                        func_t::apply(*_res, op);
                  }
                  else if (0 == _res->size())
                        throw tachy::exception("empty cache vector found for key " + key, __LINE__, __FILE__);
                  else
                        TACHY_LOG("Cache " << cache.get_id() << ": using cached result for " << key);
            }

            static_functor_engine(const static_functor_engine& other) :
                  _res(other._res)
            {}

            ~static_functor_engine()
            {
                  TACHY_LOG("DEBUG: destroying static_functor_engine " << _key);
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return _res->get_packed(idx);
            }

            NumType operator[] (const unsigned int idx) const
            {
                  return (*_res)[idx];
            }

            unsigned int size() const
            {
                  return _res->size();
            }

            tachy_date get_start_date() const
            {
                  return _res->get_start_date();
            }
            
            template <class SomeOtherDataEngine> constexpr bool depends_on(const SomeOtherDataEngine& eng) const
            {
                  return false;
            }
            
            bool depends_on(const vector_engine<NumType>& eng) const
            {
                  return _res == &eng;
            }
            
      protected:
            std::string             _key; // for debug only
            vector_engine<NumType>* _res;

            static_functor_engine& operator= (const static_functor_engine& other )
            {
                  return *this;
            }
      };

      template <typename NumType, typename Op, class StaticFunctor>
      class static_functor_engine<NumType, Op, StaticFunctor, 0>
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef StaticFunctor func_t;

            static_functor_engine(const Op& op)
                  : _op(op)
            {}
            static_functor_engine(const static_functor_engine& other)
                  : _op(other._op)
            {}

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return func_t::apply_packed(_op.get_packed(idx));
            }

            NumType operator[] (const unsigned int idx) const
            {
                  return func_t::apply(_op[idx]);
            }

            unsigned int size() const
            {
                  return _op.size();
            }

            tachy_date get_start_date() const
            {
                  return _op.get_start_date();
            }
            
            template <class SomeOtherDataEngine> constexpr bool depends_on(const SomeOtherDataEngine& eng) const
            {
                  return false;
            }
            
            bool depends_on(const typename data_engine_traits<Op>::ref_type_t eng) const
            {
                  return &_op == &eng;
            }
            
      protected:
            typename data_engine_traits<Op>::ref_type_t _op;

            static_functor_engine& operator= (const static_functor_engine& other )
            {
                  return *this;
            }
      };

      template <typename NumType, typename Op, class StaticFunctor, unsigned int Level>
      class static_functor_engine_delayed_cache
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef StaticFunctor func_t;
            typedef calc_cache<NumType, Level> cache_t;

            static_functor_engine_delayed_cache(const std::string& key, const Op& op, cache_t& cache) :
                  _key(key),
                  _op(op),
                  _cache(cache),
                  _cached_vector(0)
            {
                  TACHY_LOG("Delayed Cache " << cache.get_id() << " for " << key);
            }

            static_functor_engine_delayed_cache(const static_functor_engine_delayed_cache& other) :
                  _key(other._key),
                  _op(other._op),
                  _cache(other._cache),
                  _cached_vector(0)
            {}

            ~static_functor_engine_delayed_cache()
            {
                  TACHY_LOG("DEBUG: destroying static_functor_engine_delayed_cache " << _key);
                  delete _cached_vector;
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return func_t::apply_packed(_op.get_packed(idx));
            }

            NumType operator[] (int idx) const
            {
                  return func_t::apply(_op[idx]);
            }

            unsigned int size() const
            {
                  return _op.size();
            }

            tachy_date get_start_date() const
            {
                  return _op.get_start_date();
            }
            
            const static_functor_engine<NumType, Op, StaticFunctor, Level>& get_cached_engine() const
            {
                  if (0 == _cached_vector)
                        const_cast<static_functor_engine_delayed_cache*>(this)->_cached_vector = new static_functor_engine<NumType, Op, StaticFunctor, Level>(_key, _op, _cache);
                  return *_cached_vector;
            }

            template <class SomeOtherDataEngine> bool depends_on(const SomeOtherDataEngine& eng) const
            {
                  if (_cached_vector)
                        return _cached_vector->depends_on(eng);
                  else
                        return _op.depends_on(eng);
            }
            
      protected:
            std::string _key;
            typename data_engine_traits<Op>::ref_type_t _op;
            cache_t& _cache;
            static_functor_engine<NumType, Op, StaticFunctor, Level>* _cached_vector;

            static_functor_engine_delayed_cache& operator= (const static_functor_engine_delayed_cache& other )
            {
                  return *this;
            }
      };

      template <typename NumType, class Op, class StaticFunctor, unsigned int Level>
      const static_functor_engine<NumType, Op, StaticFunctor, Level>& do_cache(const static_functor_engine_delayed_cache<NumType, Op, StaticFunctor, Level>& eng)
      {
            return eng.get_cached_engine();
      }

      template <typename NumType, class Op, class StaticFunctor, unsigned int Level>
      struct data_engine_traits< static_functor_engine_delayed_cache<NumType, Op, StaticFunctor, Level> >
      {
            typedef static_functor_engine<NumType, Op, StaticFunctor, Level> cached_engine_t;
            typedef static_functor_engine_delayed_cache<NumType, Op, StaticFunctor, Level> const& ref_type_t;
      };

      // Some specific static functors
      template <typename NumType>
      struct exp_static_functor
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef typename arch_traits_t::packed_t packed_t;

            template <class Res, class Op>
            static inline void apply(Res& y, const Op& x)
            {
                  unsigned int sz = y.size();
                  for (unsigned int i = 0; i < sz; ++i)
                        y[i] = std::exp(x[i]);
            }

            static inline packed_t apply_packed(const packed_t& x)
            {
                  return arch_traits_t::exp(x);
            }

            static inline NumType apply(NumType x)
            {
                  return std::exp(x);
            }

            // WARNING: I am not sure this is correct - if it isn't, we'll have to capture engine being passed in to the operator()
            template <class SomeDataEngine> constexpr bool depends_on(const SomeDataEngine& eng) const
            {
                  return false;
            }
      };

      template <typename NumType>
      struct log_static_functor
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef typename arch_traits_t::packed_t packed_t;

            template <class Res, class Op>
            static inline void apply(Res& y, const Op& x)
            {
                  unsigned int sz = y.size();
                  for (unsigned int i = 0; i < sz; ++i)
                        y[i] = std::log(x[i]);
            }

            static inline packed_t apply_packed(const packed_t& x)
            {
                  return arch_traits_t::log(x);
            }

            static inline NumType apply(NumType x)
            {
                  return std::log(x);
            }

            // WARNING: I am not sure this is correct - if it isn't, we'll have to capture engine being passed in to the operator()
            template <class SomeDataEngine> constexpr bool depends_on(const SomeDataEngine& eng) const
            {
                  return false;
            }
      };

      template <typename NumType>
      struct neg_static_functor
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef typename arch_traits_t::packed_t packed_t;

            template <class Res, class Op>
            static inline void apply(Res& y, const Op& x)
            {
                  unsigned int sz = y.size();
                  for (unsigned int i = 0; i < sz; ++i)
                        y[i] = -x[i];
            }

            static inline packed_t apply_packed(const packed_t& x)
            {
                  return arch_traits_t::neg(x);
            }

            static inline NumType apply(NumType x)
            {
                  return -x;
            }

            // WARNING: I am not sure this is correct - if it isn't, we'll have to capture engine being passed in to the operator()
            template <class SomeDataEngine> constexpr bool depends_on(const SomeDataEngine& eng) const
            {
                  return false;
            }
      };
      
      template <typename NumType>
      struct abs_static_functor
      {
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef typename arch_traits_t::packed_t packed_t;

            template <class Res, class Op>
            static inline void apply(Res& y, const Op& x)
            {
                  unsigned int sz = y.size();
                  for (unsigned int i = 0; i < sz; ++i)
                        y[i] = std::abs(x[i]);
            }

            static inline packed_t apply_packed(const packed_t& x)
            {
                  return arch_traits_t::abs(x);
            }

            static inline NumType apply(NumType x)
            {
                  return std::abs(x);
            }

            // WARNING: I am not sure this is correct - if it isn't, we'll have to capture engine being passed in to the operator()
            template <class SomeDataEngine> constexpr bool depends_on(const SomeDataEngine& eng) const
            {
                  return false;
            }
      };
      
#define TACHY_STATIC_UNARY_FUNCTOR_PACK(FUNC_TYPE, FUNC_NAME, SYMBOL)      \
      template <typename NumType, class Engine, unsigned int Level> \
      calc_vector<NumType, static_functor_engine_delayed_cache<NumType, Engine, FUNC_TYPE, Level>, Level> FUNC_NAME (const calc_vector<NumType, Engine, Level>& x) \
      { \
            typedef static_functor_engine_delayed_cache<NumType, Engine, FUNC_TYPE, Level> engine_t; \
            std::string hashed_id = x.cache().get_hash_key(std::string(#SYMBOL) + x.get_id()); \
            return calc_vector<NumType, engine_t, Level>(hashed_id, x.get_start_date(), engine_t(hashed_id, x.engine(), x.cache()), x.cache() ); \
      } \
      \
      template <typename NumType, class Engine> \
      calc_vector<NumType, static_functor_engine<NumType, Engine, FUNC_TYPE, 0>, 0> FUNC_NAME (const calc_vector<NumType, Engine, 0>& x) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            typedef static_functor_engine<NumType, Engine, FUNC_TYPE, 0> engine_t; \
            std::string hashed_id = cache_t::get_dummy_key(); /* x.cache().getHashKey(std::string("-") + x.get_id()); */  \
            return calc_vector<NumType, engine_t, 0>(hashed_id, x.get_start_date(), engine_t(x.engine()), x.cache()); \
      }

// end of TACHY_STATIC_UNARY_FUNCTOR_PACK macro

      TACHY_STATIC_UNARY_FUNCTOR_PACK(neg_static_functor<NumType>, operator-, -)
      TACHY_STATIC_UNARY_FUNCTOR_PACK(exp_static_functor<NumType>, exp, EXP_)
      TACHY_STATIC_UNARY_FUNCTOR_PACK(log_static_functor<NumType>, log, LOG_)
      TACHY_STATIC_UNARY_FUNCTOR_PACK(abs_static_functor<NumType>, abs, ABS_)



#define TACHY_STATIC_BINARY_FUNCTOR_PACK(FUNC_TYPE, FUNC_NAME, SYMBOL)      \
      template <typename NumType, class Eng1, class Eng2, unsigned int Level1, unsigned int Level2> \
      calc_vector<NumType, op_engine<NumType, typename data_engine_traits<Eng1>::cached_engine_t, FUNC_TYPE, typename data_engine_traits<Eng2>::cached_engine_t, take_min<Level1, Level2>::result>, take_min<Level1, Level2>::result> FUNC_NAME (const calc_vector<NumType, Eng1, Level1>& x, const calc_vector<NumType, Eng2, Level2>& y) \
      { \
            typedef calc_cache<NumType, take_min<Level1, Level2>::result> cache_t; \
            typedef cache_chooser<(unsigned int)(cache_t::cache_level) == Level1, calc_cache<NumType, Level1>, calc_cache<NumType,Level2> > cache_chooser_t; \
            cache_t& cache = cache_chooser_t::choose(x.cache(), y.cache()); \
            typedef op_engine<NumType, typename data_engine_traits<Eng1>::cached_engine_t, FUNC_TYPE, typename data_engine_traits<Eng2>::cached_engine_t, take_min<Level1, Level2>::result> engine_t; \
            std::string id = cache.get_hash_key(std::string(#SYMBOL) + x.get_id() + std::string("_") + y.get_id()); \
            TACHY_LOG("Doing delayed cache calculations on " << id); \
            const typename data_engine_traits<Eng1>::cached_engine_t& eng_x = do_cache(x.engine()); \
            const typename data_engine_traits<Eng2>::cached_engine_t& eng_y = do_cache(y.engine()); \
            return calc_vector<NumType, engine_t, take_min<Level1, Level2>::result>(id, x.get_start_date(), eng_x, eng_y, cache); \
      } \
      \
      template <typename NumType, class Eng1, class Eng2, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, Eng1, FUNC_TYPE, Eng2, Level>, Level> FUNC_NAME (const calc_vector<NumType, Eng1, Level>& x, const calc_vector<NumType, Eng2, Level>& y) \
      { \
            typedef calc_cache<NumType, Level> cache_t; \
            cache_t& cache = x.cache();                               \
            typedef op_engine_delayed_cache<NumType, Eng1, FUNC_TYPE, Eng2, Level> engine_t; \
            std::string id = cache.get_hash_key(std::string(#SYMBOL) + x.get_id() + std::string("_") + y.get_id()); \
            return calc_vector<NumType, engine_t, Level>(id, x.get_start_date(), x.engine(), y.engine(), cache); \
      } \
      \
      \
      template <typename NumType, class Eng1, class Eng2> \
      calc_vector<NumType, op_engine<NumType, Eng1, FUNC_TYPE, Eng2, 0>, 0> FUNC_NAME (const calc_vector<NumType, Eng1, 0>& x, const calc_vector<NumType, Eng2, 0>& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache;                                      \
            typedef op_engine<NumType, Eng1, FUNC_TYPE, Eng2, 0> engine_t;  \
            std::string id = cache_t::get_dummy_key(); /* cache.getHashKey(x.get_id() + #OP + y.get_id()); */  \
            return calc_vector<NumType, engine_t, 0>(id, x.get_start_date(), x.engine(), y.engine(), cache); \
      } \
      \
      template <typename NumType, class Eng, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, scalar<NumType>, FUNC_TYPE, Eng, Level>, Level> FUNC_NAME (const NumType& x, const calc_vector<NumType, Eng, Level>& y) \
      { \
            std::string x_id = scalar<NumType>::get_id(x); \
            std::string hashed_id = y.cache().get_hash_key(std::string(#SYMBOL) + x_id + std::string("_") + y.get_id()); \
            typedef op_engine_delayed_cache<NumType, scalar<NumType>, FUNC_TYPE, Eng, Level> engine_t; \
            engine_t eng(hashedId, scalar<NumType>(x), y.engine(), y.cache()); \
            return calc_vector<NumType, engine_t, Level>(hashed_id, y.get_start_date(), eng, y.cache()); \
      } \
      \
      template <typename NumType, class Eng, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, Eng, FUNC_TYPE, scalar<NumType>, Level>, Level> FUNC_NAME (const calc_vector<NumType, Eng, Level>& x, const NumType& y) \
      { \
            std::string y_id = scalar<NumType>::get_id(y); \
            std::string hashed_id = x.cache().get_hash_key(std::string(#SYMBOL) + x.get_id() + std::string("_") + y_id); \
            typedef op_engine_delayed_cache<NumType, Eng, FUNC_TYPE, scalar<NumType>, Level> engine_t; \
            engine_t eng(hashed_id, x.engine(), scalar<NumType>(y), x.cache()); \
            return calc_vector<NumType, engine_t, Level>(hashedId, x.get_start_date(), eng, x.cache()); \
      } \
      \
      template <typename NumType, class Eng>               \
      calc_vector<NumType, op_engine<NumType, scalar<NumType>, FUNC_TYPE, Eng, 0>, 0> FUNC_NAME (const NumType& x, const calc_vector<NumType, Eng, 0>& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache; \
            std::string hashed_id = cache_t::get_dummy_key();                   \
            typedef op_engine<NumType, scalar<NumType>, FUNC_TYPE, Eng, 0> engine_t; \
            engine_t eng(hashed_id, scalar<NumType>(x), y.engine(), cache); \
            return calc_vector<NumType, engine_t, 0>(hashed_id, y.get_start_date(), eng, cache); \
      } \
      \
      template <typename NumType, class Eng> \
      calc_vector<NumType, op_engine<NumType, Eng, FUNC_TYPE, scalar<NumType>, 0>, 0> FUNC_NAME (const calc_vector<NumType, Eng, 0>& x, const NumType& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache; \
            std::string hashed_id = cache_t::get_dummy_key(); \
            typedef op_engine<NumType, Eng, FUNC_TYPE, scalar<NumType>, 0> engine_t; \
            engine_t eng(hashed_id, x.engine(), scalar<NumType>(y), cache); \
            return calc_vector<NumType, engine_t, 0>(hashed_id, x.get_start_date(), eng, cache); \
      }

// end of TACHY_BINARY_STATIC_FUNCTOR_PACK macro

}

#endif // TACHY_STATIC_FUNCTOR_ENGINE_H__INCLUDED

#if !defined(TACHY_EXPR_H__INCLUDED)
#define TACHY_EXPR_H__INCLUDED 1

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include "tachy_vector.h"
#include "tachy_scalar.h"
#include "tachy_exception.h"

namespace tachy
{
      /** Arithmetic operations
       */

      /* TODO: create specialization with vector intrinsics when Op1 & Op2 are the leaf level arrays */
#define TACHY_OP_TYPE_CLASS(OP_NAME, OP, OP_NAME_TRAITS) \
      template <typename NumType> \
      struct OP_NAME \
      { \
            static inline NumType apply(NumType x, NumType y) \
            { \
                  return x OP y; \
            } \
            template <class Res, class Op1, class Op2> \
                  static inline void apply(Res& res, const Op1& x, const Op2& y, int offset_x, int offset_y) \
            { \
                  TACHY_LOG("in scalar version");     \
                  unsigned int sz = res.size(); \
                  unsigned int i; \
                  for (i = 0; i < sz; i += arch_traits_t::stride)    \
                        (*(typename arch_traits_t::packed_t*)(&res[i])) = arch_traits_t::OP_NAME_TRAITS(x.get_packed(i + offset_x), y.get_packed(i + offset_y)); \
                  for ( ; i < sz; ++i ) \
                        res[i] = apply(x[i+offset_x], y[i+offset_y]); \
            } \
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t; \
            typedef typename arch_traits_t::packed_t packed_t; \
            static inline packed_t apply_packed(const packed_t& x, const packed_t& y) \
            { \
                  return arch_traits_t::OP_NAME_TRAITS(x, y); \
            } \
            template <class Res, unsigned int Level1, unsigned int Level2> \
            static inline void apply(Res& res, \
                                     const calc_vector<NumType, vector_engine<NumType>, Level1>& x, \
                                     const calc_vector<NumType, vector_engine<NumType>, Level2>& y) \
            { \
                  TACHY_LOG("in vectorized version"); \
                  unsigned int sz = res.size(); \
                  unsigned int i; \
                  for (i = 0; i < sz; i += arch_traits_t::stride)    \
                        (*(typename arch_traits_t::packed_t*)(&res[i])) = arch_traits_t::OP_NAME_TRAITS(x[i], y[i]); \
                  for ( ; i < sz; ++i ) \
                        res[i] = apply(x[i], y[i]); \
            } \
      };
// end of TACHY_OP_TYPE_CLASS macro

      TACHY_OP_TYPE_CLASS(OpPlus, +, add)
      TACHY_OP_TYPE_CLASS(OpMinus, -, sub)
      TACHY_OP_TYPE_CLASS(OpTimes, *, mul)
      TACHY_OP_TYPE_CLASS(OpDivide, /, div)

      template <typename NumType, typename Op1, class OpType, typename Op2, unsigned int Level>
      class op_engine
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

            op_engine(const std::string& key, const Op1& op1, const Op2& op2, calc_cache<NumType, Level>& cache) :
                  _res(dynamic_cast<vector_engine<NumType>*>(cache[key]))
            {
                  if (0 == _res || 0 == _res->size())
                  {
                        TACHY_LOG("Cache " << cache.get_id() << ": calculating for " << key);
                        tachy_date dt1 = op1.get_start_date();
                        tachy_date dt2 = op2.get_start_date();
                        tachy_date dt = dt1 < dt2 ? dt2 : dt1;
                        int offset1 = std::max<int>(0, dt - dt1);
                        int offset2 = std::max<int>(0, dt - dt2);
                        unsigned int sz1 = op1.size() - offset1;
                        unsigned int sz2 = op2.size() - offset2;
                        unsigned int sz = sz1 && sz2 ? std::min(sz1, sz2) : sz1 + sz2;
                        _res = new vector_engine<NumType>(dt, sz, NumType(0));
                        cache[key] = _res;
                        OpType::apply(*_res, op1, op2, offset1, offset2);
                  }
                  else
                        TACHY_LOG("Cache " << cache.get_id() << ": using cached result for " << key);
            }
           
            op_engine(const op_engine& other) :
                  _res(other._res)
            {}
           
            ~op_engine()
            {}
           
            typename arch_traits_t::packed_t get_packed(unsigned int idx) const
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

      protected:
            vector_engine<NumType>* _res;
           
            op_engine& operator= (const op_engine& other)
            {
                  return *this;
            }
      };
     
      template <typename NumType, typename Op1, class OpType, typename Op2>
      class op_engine<NumType, Op1, OpType, Op2, 0>
      {
      protected:

            void setup()
            {
                  tachy_date dt1 = _op1.get_start_date();
                  tachy_date dt2 = _op2.get_start_date();
                  _dt = dt1 < dt2 ? dt2 : dt1;
                  _offset1 = std::max<int>(0, _dt - dt1);
                  _offset2 = std::max<int>(0, _dt - dt2);
                  unsigned int sz1 = _op1.size() - _offset1;
                  unsigned int sz2 = _op2.size() - _offset2;
                  _sz = sz1 && sz2 ? std::min(sz1, sz2) : sz1 + sz2;
            }
            
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

            op_engine(const Op1& op1, const Op2& op2) :
                  _op1(op1),
                  _op2(op2),
                  _dt(tachy_date::min_date())
            {
                  setup();
            }
           
            op_engine(const std::string&, const Op1& op1, const Op2& op2, const calc_cache<NumType, 0>&) :
                  _op1(op1),
                  _op2(op2),
                  _dt(tachy_date::min_date())
            {
                  setup();
            }
           
            op_engine(const op_engine& other) :
                  _op1(other._op1),
                  _op2(other._op2),
                  _dt(other._dt),
                  _sz(other._sz),
                  _offset1(other._offset1),
                  _offset2(other._offset2)
            {}
           
            typename arch_traits_t::packed_t get_packed(unsigned int idx) const
            {
                  return OpType::apply_packed(_op1.get_packed(idx + _offset1), _op2.get_packed(idx + _offset2));
            }

            NumType operator[] (const unsigned int idx) const
            {
                  return OpType::apply(_op1[idx + _offset1], _op2[idx + _offset2]);
            }
           
            unsigned int size() const
            {
                  return _sz;
            }

            tachy_date get_start_date() const
            {
                  return _dt;
            }

      protected:
            typename data_engine_traits<Op1>::ref_type_t _op1;
            typename data_engine_traits<Op2>::ref_type_t _op2;
            tachy_date   _dt;
            unsigned int _sz;
            unsigned int _offset1;
            unsigned int _offset2;
            
            op_engine& operator= (const op_engine& other )
            {
                  return *this;
            }
      };
     
      template <typename NumType, typename Op1, class OpType, typename Op2, unsigned int Level>
      class op_engine_delayed_cache
      {
      public:
            typedef calc_cache<NumType, Level> cache_t;
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

            op_engine_delayed_cache(const std::string& key, const Op1& op1, const Op2& op2, cache_t& cache) :
                  _key(key),    
                  _op1(op1),    
                  _op2(op2),    
                  _cache(cache),
                  _cached_vector(0)
            {
                  TACHY_LOG("Delayed Cache " << cache.get_id() << ": delayed caching for " << key);
            }
           
            op_engine_delayed_cache(const op_engine_delayed_cache& other) :
                  _key(other._key),    
                  _op1(other._op1),    
                  _op2(other._op2),    
                  _cache(other._cache),
                  _cached_vector(0)
            {}
           
            ~op_engine_delayed_cache()
            {
                  TACHY_LOG("DEBUG: destroying op_engine_delayed_cache " << _key);
                  delete _cached_vector;
            }
           
            NumType operator[] (const unsigned int idx) const
            {
                  return OpType::apply(_op1[idx], _op2[idx]);
            }
           
            typename arch_traits_t::packed_t get_packed(unsigned int idx) const
            {
                  return OpType::apply_packed(_op1.get_packed(idx), _op2.get_packed(idx));
            }

            unsigned int size() const
            {
                  unsigned int s1 = _op1.size();
                  unsigned int s2 = _op2.size();
                  return s1 && s2 ? std::min(s1, s2) : std::max(s1, s2);
            }
           
            tachy_date get_start_date() const
            {
                  tachy_date d1 = _op1.get_start_date();
                  tachy_date d2 = _op2.get_start_date();
                  return d1 < d2 ? d2 : d1;
            }
            
            const op_engine<NumType, Op1, OpType, Op2, Level>& get_cached_engine() const
            {
                  if (0 == _cached_vector)
                        const_cast<op_engine_delayed_cache*>(this)->_cached_vector = new op_engine<NumType, Op1, OpType, Op2, Level>(_key, _op1, _op2, _cache);
                  return *_cached_vector;
            }
           
      protected:
            std::string _key;
            typename data_engine_traits<Op1>::ref_type_t _op1;
            typename data_engine_traits<Op2>::ref_type_t _op2;
            cache_t& _cache;
            op_engine<NumType, Op1, OpType, Op2, Level>* _cached_vector;
           
            op_engine_delayed_cache& operator= (const op_engine_delayed_cache& other )
            {
                  return *this;
            }
      };
     
      template <typename NumType, class Op1, class OpType, class Op2, unsigned int Level>
      const op_engine<NumType, Op1, OpType, Op2, Level>& do_cache(const op_engine_delayed_cache<NumType, Op1, OpType, Op2, Level>& eng)
      {
            return eng.get_cached_engine();
      }
     
     
      template <typename NumType, class Op1, class OpType, class Op2, unsigned int Level>   
      struct data_engine_traits< op_engine_delayed_cache<NumType, Op1, OpType, Op2, Level> >
      {
            typedef op_engine<NumType, Op1, OpType, Op2, Level> cached_engine_t;
            typedef op_engine_delayed_cache<NumType, Op1, OpType, Op2, Level> const& ref_type_t;
      };
      

#define TACHY_EXPR_OPERATOR_PACK(OP_TYPE, OP) \
      /* 1) general case template for binary operation */                                    \
      template <typename NumType, class Eng1, class Eng2, unsigned int Level1, unsigned int Level2> \
      calc_vector<NumType, op_engine<NumType, typename data_engine_traits<Eng1>::cached_engine_t, OP_TYPE, typename data_engine_traits<Eng2>::cached_engine_t, take_min<Level1, Level2>::result>, take_min<Level1, Level2>::result> operator OP (const calc_vector<NumType, Eng1, Level1>& x, const calc_vector<NumType, Eng2, Level2>& y) \
      { \
            typedef calc_cache<NumType, take_min<Level1, Level2>::result> cache_t; \
            typedef cache_chooser<(unsigned int)(cache_t::cache_level) == Level1, calc_cache<NumType, Level1>, calc_cache<NumType,Level2> > cache_chooser_t; \
            cache_t& cache = cache_chooser_t::choose(x.cache(), y.cache()); \
            typedef op_engine<NumType, typename data_engine_traits<Eng1>::cached_engine_t, OP_TYPE, typename data_engine_traits<Eng2>::cached_engine_t, take_min<Level1, Level2>::result> engine_t; \
            std::string id = cache.get_hash_key(x.get_id() + #OP + y.get_id()); \
            TACHY_LOG("Doing delayed cache calculations on " << id); \
            const typename data_engine_traits<Eng1>::cached_engine_t& eng_x = do_cache(x.engine()); \
            const typename data_engine_traits<Eng2>::cached_engine_t& eng_y = do_cache(y.engine()); \
            return calc_vector<NumType, engine_t, take_min<Level1, Level2>::result>(id, eng_x, eng_y, cache); \
      } \
      \
      /* 2) template for cases with Level of both operands being the same - no caching is necessary even if Level > 0 (delay until either = operator or op with a lower Level) */ \
      template <typename NumType, class Eng1, class Eng2, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, Eng1, OP_TYPE, Eng2, Level>, Level> operator OP (const calc_vector<NumType, Eng1, Level>& x, const calc_vector<NumType, Eng2, Level>& y) \
      { \
            typedef calc_cache<NumType, Level> cache_t; \
            cache_t& cache = x.cache();                               \
            typedef op_engine_delayed_cache<NumType, Eng1, OP_TYPE, Eng2, Level> engine_t; \
            std::string id = cache.get_hash_key(x.get_id() + #OP + y.get_id()); \
            return calc_vector<NumType, engine_t, Level>(id, x.engine(), y.engine(), cache); \
      } \
      \
      \
      /* 3) both Level == 0 - no caching here & id is a dummy */  \
      template <typename NumType, class Eng1, class Eng2> \
      calc_vector<NumType, op_engine<NumType, Eng1, OP_TYPE, Eng2, 0>, 0> operator OP (const calc_vector<NumType, Eng1, 0>& x, const calc_vector<NumType, Eng2, 0>& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache;                                      \
            typedef op_engine<NumType, Eng1, OP_TYPE, Eng2, 0> engine_t;  \
            std::string id = cache_t::get_dummy_key(); /* cache.get_hash_key(x.get_id() + #OP + y.get_id()); */  \
            return calc_vector<NumType, engine_t, 0>(id, x.engine(), y.engine(), cache); \
      } \
      \
      /* 4) general template for scalar with a vector */         \
      template <typename NumType, class Eng, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, scalar<NumType>, OP_TYPE, Eng, Level>, Level> operator OP (const NumType& x, const calc_vector<NumType, Eng, Level>& y) \
      { \
            std::string x_id = scalar<NumType>::get_id(x); \
            std::string hashed_id = y.cache().get_hash_key(x_id + #OP + y.get_id()); \
            typedef op_engine_delayed_cache<NumType, scalar<NumType>, OP_TYPE, Eng, Level> engine_t; \
            engine_t eng(hashed_id, scalar<NumType>(x), y.engine(), y.cache()); \
            return calc_vector<NumType, engine_t, Level>(hashed_id, y.get_start_date(), eng, y.cache()); \
      } \
      \
      /* 5) general template for vector with a scalar */         \
      template <typename NumType, class Eng, unsigned int Level> \
      calc_vector<NumType, op_engine_delayed_cache<NumType, Eng, OP_TYPE, scalar<NumType>, Level>, Level> operator OP (const calc_vector<NumType, Eng, Level>& x, const NumType& y) \
      { \
            std::string y_id = scalar<NumType>::get_id(y); \
            std::string hashed_id = x.cache().get_hash_key(x.get_id() + #OP + y_id); \
            typedef op_engine_delayed_cache<NumType, Eng, OP_TYPE, scalar<NumType>, Level> engine_t; \
            engine_t eng(hashed_id, x.engine(), scalar<NumType>(y), x.cache()); \
            return calc_vector<NumType, engine_t, Level>(hashed_id, x.get_start_date(), eng, x.cache()); \
      } \
      \
      /* 6) template for scalar and a non-cacheable (Level == 0) vector */  \
      template <typename NumType, class Eng>               \
      calc_vector<NumType, op_engine<NumType, scalar<NumType>, OP_TYPE, Eng, 0>, 0> operator OP (const NumType& x, const calc_vector<NumType, Eng, 0>& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache; \
            std::string hashed_id = cache_t::get_dummy_key();                   \
            typedef op_engine<NumType, scalar<NumType>, OP_TYPE, Eng, 0> engine_t; \
            return calc_vector<NumType, engine_t, 0>(hashed_id, scalar<NumType>(x), y.engine(), cache); \
      } \
      \
      /* 7) template for a non-cacheable (Level == 0) vector and a scalar */  \
      template <typename NumType, class Eng> \
      calc_vector<NumType, op_engine<NumType, Eng, OP_TYPE, scalar<NumType>, 0>, 0> operator OP (const calc_vector<NumType, Eng, 0>& x, const NumType& y) \
      { \
            typedef calc_cache<NumType, 0> cache_t; \
            cache_t cache; \
            std::string hashed_id = cache_t::get_dummy_key(); \
            typedef op_engine<NumType, Eng, OP_TYPE, scalar<NumType>, 0> engine_t; \
            return calc_vector<NumType, engine_t, 0>(hashed_id, x.engine(), scalar<NumType>(y), cache); \
      }

// end of TACHY_EXPR_OPERATOR_PACK macro

      TACHY_EXPR_OPERATOR_PACK(OpPlus<NumType>,  +)
      TACHY_EXPR_OPERATOR_PACK(OpMinus<NumType>, -)
      TACHY_EXPR_OPERATOR_PACK(OpTimes<NumType>, *)
      TACHY_EXPR_OPERATOR_PACK(OpDivide<NumType>, /)
}

#endif // TACHY_EXPR_H__INCLUDED


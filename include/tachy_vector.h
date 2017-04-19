#if !defined(TACHY_VECTOR_H__INCLUDED)
#define TACHY_VECTOR_H__INCLUDED 1

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <cstring>

#include "tachy_arch_traits.h"
#include "tachy_calc_cache.h"
#include "tachy_vector_engine.h"
#include "tachy_lagged_engine.h"
#include "tachy_time_shift.h"

namespace tachy
{
      // this is the generic template used for Level > 0 and DataEngine distinct from vector
      // for Level == 0 and for DataEngine = std::vector there will be separate specializations later on
      // -- the idea is to enable proxy/caching savings for both speed and memory

      template <typename NumType, class DataEngine, unsigned int Level>
      class calc_vector
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef DataEngine data_engine_t;
            typedef calc_cache<NumType, Level> cache_t;
            typedef calc_vector<NumType, DataEngine, Level> self_t;

            // no default c'tor
            calc_vector(const std::string& id, int date, const DataEngine& eng, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _engine(eng),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-2: Creating copy from same engine: " /* << typeid(eng).name() << " " */ << id);
            }

            // works for a pair of different engines
            template <class Eng1, class Eng2>
            calc_vector(const std::string& id, int date, const Eng1& eng1, const Eng2& eng2, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _engine(id, eng1, eng2, cache),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-3: Creating " << id << " from a pair of engines");
            }

#if 0 // this cannot work when DataEngine is NOT std::vector
            // for proxy vectors
            calc_vector(const std::string& id, int date, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _engine(_cache[id]),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-4: Creating proxy in place: " << id);
            }
#endif

            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector(const calc_vector<NumType, OtherDataEngine, OtherLevel>& other, cache_t& cache) :
                  _engine(other.size()),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-6: Creating from a different engine: " << _id << " from " << other.get_id() << "<" << cache_t::cache_level << ">");
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  for ( int i = 0, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];
            }

            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector& operator= (const calc_vector<NumType, OtherDataEngine, OtherLevel>& other)
            {
                  TACHY_LOG("calc_vector (L>0): assigning: " << _id << " = " << other.get_id());

                  // need a compile time assert here Level <= OtherLevel
                  // at run-time, it should be valid only if the current vector hasn't been cached yet
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  _engine.resize(other.size(), NumType());
                  for ( int i = 0, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];

                  return *this;
            }

            ~calc_vector()
            {}

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return _engine.get_packed(idx);
            }

            typename arch_traits_t::packed_t& get_packed(int idx)
            {
                  return _engine.get_packed(idx);
            }

            NumType operator[] (int idx) const
            {
                  return _engine[idx];
            }

            NumType& operator[] (int idx)
            {
                  return _engine[idx];
            }

            const calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, Level> operator[](const time_shift& shift) const
            {
                  typedef lagged_engine<NumType, data_engine_t, true> engine_t;
                  std::string hashed_id = cache().get_hash_key(std::string("LAGCK_") + _id);
                  engine_t eng(_engine, -shift.get_time_shift()); // because lag already implies a "-"
                  return calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, Level>(hashed_id, _anchor_date, eng, _cache);
            }
            
            const std::string& get_id() const
            {
                  return _id;
            }

            void set_id(const std::string& id)
            {
                  _id = id;
            }

            int get_start_date() const
            {
                  return _anchor_date;
            }

            unsigned int size() const
            {
                  return _engine.size();
            }

            unsigned int get_num_hist() const
            {
                  return 0;
            }

            DataEngine& engine()
            {
                  return _engine;
            }

            const DataEngine& engine() const
            {
                  return _engine;
            }

            const DataEngine& get_cached_engine() const
            {
                  return _engine;
            }

            // cache can be labelled mutable,
            // but for now (Dec 6 2013) this method is the only case where 'const' gets in the way,
            // so const_cast here
            cache_t& cache() const
            {
                  return const_cast<self_t*>(this)->_cache;
            }

            void debug_print(std::ostream& to) const
            {
                  to << "calc_vector<" << cache_t::cache_level << ">[" << _id << "], anchor date = " << _anchor_date << "\n";
                  for (int i = 0; i < _engine.size(); ++i)
                        to << _engine[i] << "\n";
                  to.flush();
            }

      protected:
            std::string _id;
            int         _anchor_date;
            DataEngine  _engine;

            cache_t& _cache;
      };

      template <typename NumType, unsigned int Level>
      class calc_vector<NumType, vector_engine<NumType>, Level>
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef vector_engine<NumType> data_engine_t;
            typedef calc_cache<NumType, Level> cache_t;
            typedef calc_vector<NumType, data_engine_t, Level> self_t;

            // no default c'tor

            calc_vector(const std::string& id, int date, const unsigned int size, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _own_engine(true),
                  _do_cache(true),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-1V: creating from cache & size: " << id);
                  _engine = new data_engine_t(size, NumType(0));
            }

            calc_vector(const std::string& id, int date, const std::vector<NumType>& eng, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _cache(cache)
            {
                  const typename cache_t::cache_engine_t::const_iterator k = _cache.find(_id);
                  if (k == _cache.end())
                  {
                        _engine = new data_engine_t(eng);
                        _do_cache = _own_engine = true;
                        TACHY_LOG("calc_vector (L>0): c-2V: Creating copy from same engine: " /* << typeid(eng).name() << " " */ << id);
                  }
                  else
                  {
                        _engine = dynamic_cast<data_engine_t*>(k->second);
                        _do_cache = _own_engine = false;
                        TACHY_LOG("calc_vector (L>0): c-2V: Creating proxy in place: " << id);
                  }
            }

            // works for proxy vectors
            calc_vector(const std::string& id, int date, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _own_engine(false),
                  _do_cache(false),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-4V: Creating proxy in place: " << id);
                  _engine = dynamic_cast<data_engine_t*>(_cache[_id]);
            }

#if 0
            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector(const calc_vector<NumType, OtherDataEngine, OtherLevel>& other, cache_t& cache) :
                  _cache(cache),
                  _own_engine(true),
                  _do_cache(true)
            {
                  // TODO: add static assert that OtherLevel >= Level -- same logic as for assignment
                  TACHY_LOG("calc_vector (L>0): c-6V: Creating from a different engine: " << _id << " from " << other.get_id() << "<" << cache_t::cache_level << ">");
                  _anchor_date = other.get_start_date();
                  _id = other.get_id();
                  const typename cache_t::cache_engine_t::const_iterator k = _cache.find(_id);
                  if (k == _cache.end())
                  {
                        unsigned int sz = other.size();
                        unsigned int nh = other.get_num_hist();
                        _engine = new data_engine_t(sz + nh, nh);
                        // in a c'tor everything is copied, including history
                        for ( int i = -nh; i < sz; ++i )
                              (*_engine)[i] = other[i];
                  }
                  else
                  {
                        _engine = dynamic_cast<data_engine_t*>(k->second);
                        _do_cache = _own_engine = false;
                  }
            }
#endif

            // implicit cache can only happen from an engine with the same Level,
            // since we're taking cache from it
            template <class OtherDataEngine>
            calc_vector(const calc_vector<NumType, OtherDataEngine, Level>& other) :
                  _own_engine(true),
                  _do_cache(true),
                  _cache(other.cache())
            {
                  TACHY_LOG("calc_vector (L>0): c-7o: Creating from a different engine, implicit cache: " << _id << " from " << other.get_id() << "<" << cache_t::cache_level << ">");
                  _anchor_date = other.get_start_date();
                  _id = other.get_id();
                  const typename cache_t::cache_engine_t::const_iterator k = _cache.find(_id);
                  if (k == _cache.end())
                  {
                        unsigned int sz = other.size();
                        unsigned int nh = other.get_num_hist();
                        _engine = new data_engine_t(sz + nh, nh);
                        // in a c'tor everything is copied, including history
                        for ( int i = -nh; i < sz; ++i )
                              (*_engine)[i] = other[i];
                  }
                  else
                  {
                        _engine = dynamic_cast<data_engine_t*>(k->second);
                        _do_cache = _own_engine = false;
                  }
            }

            // straight copy c'tor - same engine, same level, same cache
            // useful for e.g. storing objects in std::vector
            calc_vector(const calc_vector& other) :
                  _own_engine(false), // RHS will take care of it - one is enough
                  _do_cache(false),
                  _cache(other.cache())
            {
                  TACHY_LOG("calc_vector (L>0): c-7s: Creating from the same engine, implicit cache: " << _id << " from " << other.get_id() << "<" << cache_t::cache_level << ">");
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  _engine = other._engine;
            }

            template<typename T, class Generator>
            calc_vector(const std::string& id, int date, cache_t& cache, const Generator& g) :
                  _id(id),
                  _anchor_date(date),
                  _own_engine(true),
                  _do_cache(true),
                  _cache(cache)
            {
                  TACHY_LOG("calc_vector (L>0): c-7V: Creating from a generator: " << id);
                  _engine = new data_engine_t(g.size(), NumType(0));
                  for (int i = 0, iMax = g.size(); i < iMax; ++i)
                        (*_engine)[i] = g(i);
            }

#if 1
            // for Level == 0 OR if the vector is NOT cached - just go ahead & assign as usual
            // if Level > 0 AND it is cached - then might as well throw an exception
            // Indeed: we've cached the vector to avoid recalculation - so why are we changing it again?
            //
            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector& operator= (const calc_vector<NumType, OtherDataEngine, OtherLevel>& other)
            {
                  TACHY_LOG("calc_vector (L>0): V assigning: " << _id << " = " << other.get_id());

                  auto k = _cache.find(_id);
                  if (k != _cache.end())
                  {
                        TACHY_THROW("calc_vector: trying to assign to a pre-cached object (" << _id << ")");
                  }

                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  // should the copy be with or without history?
                  // if the engine is cached, it will be _with_ history
                  // -- does it mean it has to be with history as well?
                  //
                  // assuming no, as of 3/14/14 - to be consistent with the branch above
                  unsigned int sz = min(_engine->size(), other.size());
                  for (int i = 0, i_last = sz; i < i_last; ++i)
                        (*_engine)[i] = other[i];
                  for (int i = sz, i_last = _engine->size(); i < i_last; ++i)
                        (*_engine)[i] = (*_engine)[sz-1];

                  return *this;
            }

            calc_vector& operator= (const calc_vector& other)
            {
                  if (this != &other)
                  {
                        if (&_cache != &other._cache)
                              TACHY_THROW("calc_vector: trying to assign from a vector from a different cache (" << _id << " from " << other._id << ")");

                        typename cache_t::cache_engine_t::const_iterator k = _cache.find(_id);
                        if (k != _cache.end())
                              TACHY_THROW("calc_vector: trying to assign to a pre-cached object (" << _id << " from " << other._id << ")");

                        // this does not copy history (consistent with the generic assignment operator above)
                        // on the other hand, it will override cached values...
                        // maybe post-creation changes for cached vectors should be disabled by making this private?
                        _id = other._id;
                        _anchor_date = other._anchor_date;
                        *_engine = *other._engine;
                  }

                  return *this;
            }
#endif

            ~calc_vector()
            {
                  if (_own_engine)
                  {
                        bool to_be_cached = false;
                        if (_do_cache)
                        {
                              typename cache_t::cache_engine_t::const_iterator k = _cache.find(_id);
                              to_be_cached = (k == _cache.end());
                        }
                        if (to_be_cached)
                              _cache[_id] = _engine;
                        else
                              delete _engine;
                  }
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return *(typename arch_traits_t::packed_t*)(&(*_engine)[idx]);
            }

            typename arch_traits_t::packed_t& get_packed(int idx)
            {
                  return *(typename arch_traits_t::packed_t*)(&(*_engine)[idx]);
            }

            NumType operator[] (int idx) const
            {
                  return (*_engine)[idx];
            }

            NumType& operator[] (int idx)
            {
                  return (*_engine)[idx];
            }

            const calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, Level> operator[](const time_shift& shift) const
            {
                  typedef lagged_engine<NumType, data_engine_t, true> engine_t;
                  std::string hashed_id = cache().get_hash_key(std::string("LAGCK_") + _id);
                  engine_t eng(_engine, -shift.get_time_shift()); // because lag already implies a "-"
                  return calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, Level>(hashed_id, _anchor_date, eng, _cache);
            }
            
            void drop()
            {
                  _do_cache = false;
            }
            
            const std::string& get_id() const
            {
                  return _id;
            }

            void set_id(const std::string& id)
            {
                  _id = id;
            }

            int get_start_date() const
            {
                  return _anchor_date;
            }

            unsigned int size() const
            {
                  return _engine->size();
            }

            unsigned int get_num_hist() const
            {
                  return _engine->get_num_hist();
            }

            data_engine_t& engine()
            {
                  return *_engine;
            }

            const data_engine_t& engine() const
            {
                  return *_engine;
            }

            const data_engine_t& get_cached_engine() const
            {
                  return *_engine;
            }

            // cache can be labelled mutable,
            // but for now (Dec 6 2013) this method is the only case where 'const' gets in the way,
            // so const_cast here
            cache_t& cache() const
            {
                  return const_cast<self_t*>(this)->_cache;
            }

            void debug_print(std::ostream& to) const
            {
                  to << "calc_vector<" << cache_t::cache_level << ">[" << _id << "], start date = " << _anchor_date << ", num hist = " << _engine->getFirst() << "\n";
                  for (int i = _engine->get_first(), i_last = _engine->size(); i < i_last; ++i)
                        to << (*_engine)[i] << "\n";
                  to.flush();
            }

      protected:
            std::string    _id;
            unsigned int   _anchor_date;
            data_engine_t* _engine; // since it can be a proxy, too
            bool           _own_engine;
            bool           _do_cache;
            
            cache_t& _cache;
      };

      template <typename NumType, class DataEngine>
      class calc_vector<NumType, DataEngine, 0>
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef DataEngine data_engine_t;
            typedef calc_cache<NumType, 0> cache_t;
            typedef calc_vector<NumType, DataEngine, 0> self_t;

            // no default c'tor
            calc_vector(const std::string& id, int date, const DataEngine& eng) :
                  _id(id),
                  _anchor_date(date),
                  _engine(eng)
            {
                  TACHY_LOG("calc_vector (L=0): c-1: Creating from engine: " << id);
            }

            calc_vector(const std::string& id, int date, const DataEngine& eng, const cache_t&) :
                  _id(id),
                  _anchor_date(date),
                  _engine(eng)
            {
                  TACHY_LOG("calc_vector (L=0): c-2: Creating from engine & dummy cache: " << id);
            }

            // works for pair of different engines
            template <class Eng1, class Eng2>
            calc_vector(const std::string& id, int date, const Eng1& eng1, const Eng2& eng2, cache_t& cache) :
                  _id(id),
                  _anchor_date(date),
                  _engine(id, eng1, eng2, cache)
            {
                  TACHY_LOG("calc_vector (L=0): c-3: Creating " << id << " from a pair of engines");
            }

            calc_vector(const calc_vector& other) :
                  _id(other._id),
                  _anchor_date(other._anchor_date),
                  _engine(other._engine)
            {
                  TACHY_LOG("calc_vector (L=0): c-4: Creating (same engine): " << _id << " from " << other.get_id());
            }

            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector(const calc_vector<NumType, OtherDataEngine, OtherLevel>& other) :
                  _engine(other.size())
            {
                  TACHY_LOG("calc_vector (L=0): c-5: Creating (different engine): " << _id << " from " << other.get_id() << "<" << OtherLevel << ">");
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();

                  for ( int i = 0, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];
            }

            calc_vector(const std::string& id, int date, const unsigned int size) :
                  _id(id),
                  _anchor_date(date),
                  _engine(size, NumType())
            {
                  TACHY_LOG("calc_vector (L=0): c-7: Creating from size: " << id);
            }

            template<typename T, class Functor>
            calc_vector(const std::string& id, int date, const Functor& f) :
                  _id(id),
                  _anchor_date(date)
            {
                  TACHY_LOG("calc_vector (L=0): c-8: Creating from functor: " << id);

                  _engine.reserve(f.size());
                  for (int i = 0, i_last = f.size(); i < i_last; ++i)
                        _engine.push_back(f(i));
            }

            template <class OtherDataEngine, unsigned int OtherLevel>
            self_t& operator= (const calc_vector<NumType, OtherDataEngine, OtherLevel>& other)
            {
                  TACHY_LOG("calc_vector (L=0): assigning: " << _id << " = " << other.get_id());
                  _anchor_date = other.get_start_date();
                  _engine.resize(other.size(), NumType());
                  for ( int i = 0, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];
                  return *this;
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return _engine.get_packed(idx);
            }

            typename arch_traits_t::packed_t& get_packed(int idx)
            {
                  return _engine.get_packed(idx);
            }

            const calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, 0> operator[](const time_shift& shift) const
            {
                  std::string hashed_id = cache().get_hash_key(std::string("LAGCK ") + _id);
                  return calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, 0>(hashed_id, _anchor_date, lagged_engine<NumType, data_engine_t, true>(_engine, -shift.get_time_shift()));
            }
      
            NumType operator[] (int idx) const
            {
                  return _engine[idx];
            }

            NumType& operator[] (int idx)
            {
                  return _engine[idx];
            }

            const std::string& get_id() const
            {
                  return _id;
            }

            void set_id(const std::string& id)
            {
                  _id = id;
            }

            int get_start_date() const
            {
                  return _anchor_date;
            }

            unsigned int size() const
            {
                  return _engine.size();
            }

            unsigned int get_num_hist() const
            {
                  return 0;
            }

            DataEngine& engine()
            {
                  return _engine;
            }

            const DataEngine& engine() const
            {
                  return _engine;
            }

            const DataEngine& get_cached_engine() const
            {
                  return _engine;
            }

            cache_t cache() const
            {
                  return cache_t();
            }

            void debug_print(std::ostream& to) const
            {
                  to << "calc_vector<" << cache_t::cache_level << ">[" << _id << "], start date = " << _anchor_date << "\n";
                  for (int i = 0; i < _engine.size(); ++i)
                        to << _engine[i] << "\n";
                  to.flush();
            }

      protected:
            std::string  _id;
            int          _anchor_date;
            DataEngine   _engine;
      };

      template <typename NumType>
      class calc_vector<NumType, vector_engine<NumType>, 0>
      {
      public:
            typedef arch_traits<NumType, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef vector_engine<NumType> data_engine_t;
            typedef calc_cache<NumType, 0> cache_t;
            typedef calc_vector<NumType, data_engine_t, 0> self_t;

            // no default c'tor
            calc_vector(const std::string& id, int date, const std::vector<NumType>& data) :
                  _id(id),
                  _anchor_date(date),
                  _engine(data, 0)
            {
                  TACHY_LOG("calc_vector (L=0): c-1V: Creating from engine: " << id);
            }

            calc_vector(const std::string& id, int date, const std::vector<NumType>& data, const cache_t&) :
                  _id(id),
                  _anchor_date(date),
                  _engine(data, 0)
            {
                  TACHY_LOG("calc_vector (L=0): c-2V: Creating from engine & dummy cache: " << id);
            }

            calc_vector(const calc_vector& other) :
                  _id(other._id),
                  _anchor_date(other._anchor_date),
                  _engine(other._engine)
            {
                  TACHY_LOG("calc_vector (L=0): c-4V: Creating (same engine): " << _id << " from " << other.get_id());
            }

            template <class OtherDataEngine, unsigned int OtherLevel>
            calc_vector(const calc_vector<NumType, OtherDataEngine, OtherLevel>& other) :
                  _engine(other.size() + other.get_num_hist(), other.get_num_hist())
            {
                  TACHY_LOG("calc_vector (L=0): c-5V: Creating (different engine): " << _id << " from " << other.get_id() << "<" << OtherLevel << ">");
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  unsigned int nh = other.get_num_hist();
                  for ( int i = -nh, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];
            }

            template <class OtherDataEngine>
            calc_vector(const calc_vector<NumType, OtherDataEngine, 0>& other, cache_t& /* cache */) :
                  _engine(other.size() + other.get_num_hist(), other.get_num_hist())
            {
                  TACHY_LOG("calc_vector (L=0): c-6V: Creating (different engine): " << _id << " from " << other.get_id() << "<" << cache_t::cache_level << ">");
                  _id = other.get_id();
                  _anchor_date = other.get_start_date();
                  unsigned int nh = other.get_num_hist();
                  for ( int i = -nh, i_last = _engine.size(); i < i_last; ++i )
                        _engine[i] = other[i];
            }

            calc_vector(const std::string& id, int date, const unsigned int size) :
                  _id(id),
                  _anchor_date(date),
                  _engine(size, NumType(0))
            {
                  TACHY_LOG("calc_vector (L=0): c-7V: Creating from size: " << id);
            }

            template<typename T, class Functor>
            calc_vector(const std::string& id, int date, const Functor& f) :
                  _id(id),
                  _anchor_date(date),
                  _engine(f.size())
            {
                  TACHY_LOG("calc_vector (L=0): c-8V: Creating from functor: " << id);

                  for (int i = 0, i_last = f.size(); i < i_last; ++i)
                        _engine[i] = f(i);
            }

            template <class OtherDataEngine, unsigned int OtherLevel>
            self_t& operator= (const calc_vector<NumType, OtherDataEngine, OtherLevel>& other)
            {
                  TACHY_LOG("calc_vector (L=0): V assigning: " << _id << " = " << other.get_id());

                  _anchor_date = other.get_start_date();
                  unsigned int sz = std::min(other.size(), _engine.size());
                  int i = 0;
                  for (int i_last = sz; i < i_last; i += arch_traits_t::stride)
                        get_packed(i) = other.get_packed(i);
                  for (int i_last = sz; i < i_last; ++i)
                        _engine[i] = other[i];
                  for (int i_last = _engine.size(); i < i_last; ++i)
                        _engine[i] = _engine[sz-1];
                  return *this;
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  return *(typename arch_traits_t::packed_t*)(&_engine[idx]);
            }

            typename arch_traits_t::packed_t& get_packed(int idx)
            {
                  return *(typename arch_traits_t::packed_t*)(&_engine[idx]);
            }

            NumType operator[] (int idx) const
            {
                  return _engine[idx];
            }

            NumType& operator[] (int idx)
            {
                  return _engine[idx];
            }

            const calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, 0> operator[](const time_shift& shift) const
            {
                  std::string hashed_id = cache().get_hash_key(std::string("LAGCK ") + _id);
                  return calc_vector<NumType, lagged_engine<NumType, data_engine_t, true>, 0>(hashed_id, _anchor_date, lagged_engine<NumType, data_engine_t, true>(_engine, -shift.get_time_shift()));
            }
      
            const std::string& get_id() const
            {
                  return _id;
            }

            void set_id(const std::string& id)
            {
                  _id = id;
            }

            int get_start_date() const
            {
                  return _anchor_date;
            }

            unsigned int size() const
            {
                  return _engine.size();
            }

            unsigned int get_num_hist() const
            {
                  return _engine.get_num_hist();
            }

            data_engine_t& engine()
            {
                  return _engine;
            }

            const data_engine_t& engine() const
            {
                  return _engine;
            }

            const data_engine_t get_cached_engine() const
            {
                  return _engine;
            }

            cache_t cache() const
            {
                  return cache_t();
            }

            void debug_print(std::ostream& to) const
            {
                  to << "calc_vector<" << cache_t::cache_level << ">[" << _id << "], start date = " << _anchor_date << ", num hist = " << _engine.get_num_hist() << "\n{ ";
                  for (int i = -_engine.get_num_hist(), i_last = _engine.size()-1; i < i_last; ++i)
                        to << _engine[i] << ", ";
                  if (_engine.size() > 0)
                        to << _engine.back();
                  to << " }\n";
                  to.flush();
            }

            void set_history(const std::vector<NumType>& from)
            {
                  _engine.set_hist(from);
            }

      protected:
            std::string   _id;
            int           _anchor_date;
            data_engine_t _engine;
      };
}

#endif // TACHY_VECTOR_H__INCLUDED

#if !defined(TACHY_CALC_CACHE_H__INCLUDED)
#define TACHY_CALC_CACHE_H__INCLUDED

#include <iostream>
#include <vector>
#include <map>
#include <cstring>

#include "tachy_util.h"
#include "tachy_exception.h"
#include "tachy_cacheable.h"

namespace tachy
{
      template <typename NumType, unsigned int Level>
      class calc_cache
      {
      protected:
            typedef std::map<std::string, unsigned int> hash_t;

      public:
            typedef calc_cache<NumType, Level> self_t;
            typedef cacheable* cached_t;
            typedef std::map<std::string, cached_t> cache_engine_t;
            typedef typename cache_engine_t::value_type cached_value_t;

            enum { cache_level = Level };

            explicit calc_cache(const std::string& id)
                  : _id(id)
            {}

            calc_cache(const self_t& other)
                  : _id(other._id)
            {
                  TACHY_LOG("Copying cache " << _id);
                  for (typename cache_engine_t::const_iterator i = other._cache.begin(); i != other._cache.end(); ++i)
                  {
                        _cache.insert(cached_value_t(i->first, i->second->clone()));
                  }
            }

            calc_cache& operator= (const self_t& other)
            {
                  if (this != &other)
                  {
                        clear();
                        _id = other._id;
                        for (typename cache_engine_t::const_iterator i = other._cache.begin(); i != other._cache.end(); ++i)
                        {
                              _cache.insert(cached_value_t(i->first, i->second->clone()));
                        }
                  }
                  return *this;
            }

            ~calc_cache()
            {
                  clear();
            }

            void clear()
            {
                  TACHY_LOG("Clearing calc_cache: " << _id << ": " << _cache.size() << " items");
                  for (typename cache_engine_t::iterator i = _cache.begin(); i != _cache.end(); ++i)
                  {
                        TACHY_LOG("cached: " << i->first << ": " << _use_count[i->first]);
                        delete i->second;
                  }
                  _cache.clear();
            }

            void insert(const typename cache_engine_t::value_type& kv)
            {
                  typename cache_engine_t::iterator i = _cache.find(kv.first);
                  if (i == _cache.end())
                  {
                        TACHY_LOG("calc_cache " << _id << ": adding key " << kv.first);
                        _cache.insert(kv);
                  }
                  else
                        i->second = kv.second;
            }

            cached_t& operator[](const std::string& key)
            {
                  typename cache_engine_t::iterator i = _cache.find(key);
                  if (i == _cache.end())
                  {
                        TACHY_LOG("calc_cache " << _id << ": adding key " << key);
                        i = _cache.insert(cached_value_t(key, 0)).first;
#if defined(TACHY_VERBOSE)
                        _use_count[key] = 0;
#endif
                  }
#if defined(TACHY_VERBOSE)
                  ++_use_count[key];
#endif
                  return i->second;
            }

            const cached_t& operator[](const std::string& key) const
            {
                  typename cache_engine_t::iterator i = _cache.find(key);
                  if (i == _cache.end())
                        TACHY_THROW("calc_cache::get: No such key");
#if defined(TACHY_VERBOSE)
                  ++_use_count[key];
#endif
                  return i->second;
            }

            typename cache_engine_t::const_iterator find(const std::string& key) const
            {
                  typename cache_engine_t::const_iterator k = _cache.find(key);
#if defined(TACHY_VERBOSE)
                  if (k != _cache.end())
                        ++_use_count[key];
#endif
                  return k;
            }

            const typename cache_engine_t::const_iterator begin() const
            {
                  return _cache.begin();
            }

            const typename cache_engine_t::const_iterator end() const
            {
                  return _cache.end();
            }

            bool has_key(const std::string& key) const
            {
                  return _cache.find(key) != _cache.end();
            }

            std::string get_hash_key(const std::string& key)
            {
                  hash_t::iterator h = _hashed.find(key);
                  if (h == _hashed.end())
                  {
                        h = _hashed.insert(hash_t::value_type(key, 1 + _hashed.size())).first;
                        TACHY_LOG("calc_cache " << _id << ": hash key " << key << " -> " << h->second);
                  }
                  unsigned int k = h->second;
                  const char* const s = "0123456789abcdef";
                  char res[2*sizeof(unsigned int)/sizeof(char) + 2];
                  res[0] = 'X';
                  char* ptr = &res[1];
                  for (; k > 0; ++ptr)
                  {
                        *ptr = s[k&0xf];
                        k >>= 4;
                  }
                  *ptr = '\0';
                  //sprintf(res, "X%d", h->second);
                  return res;
            }

            const std::string& get_id() const
            {
                  return _id;
            }

            void set_id(const std::string& id)
            {
                  _id = id;
            }

      private:
            std::string  _id;
            cache_engine_t _cache;
            hash_t  _hashed;
#if defined(TACHY_VERBOSE)
            mutable hash_t _use_count;
#endif
            calc_cache() {}
      };

      // partial specialization
      // 12/26: back to Level only. General case cache Level>0 should behave as a dummy cache when necessary
      //
      // "dummy" cache used to propagate the Level through a subtree of the same Level without caching intermediate steps
      // once it hits assignment or copy c'tor or binary operation with a lesser Level, general cache will be chosen
      //
      // Note that the Level == 0 cache is separate since
      // (a) it does not need to pass through a reference to the real non-dummy cache
      // and
      // (b) hashing need not preserve the true name of the vector

      template <typename NumType>
      class calc_cache<NumType, 0>
      {
      public:
            enum { cache_level = 0 };

            calc_cache()
            {}

            explicit calc_cache(const std::string&)
            {}

            calc_cache(const calc_cache<NumType, 0>&)
            {
                  TACHY_LOG("Copying Dummy calc_cache");
            }

            calc_cache& operator= (const calc_cache<NumType, 0>&)
            {
                  return *this;
            }

            ~calc_cache()
            {
                  clear();
            }

            void clear()
            {
                  TACHY_LOG("Clearing Dummy calc_cache");
            }

            bool has_key(const std::string&) const
            {
                  return false;
            }

            std::string get_hash_key(const std::string&)
            {
                  return get_dummy_key(); // not cached - so why bother?
            }

            static std::string get_dummy_key()
            {
                  return "V0";
            }
      };

      template <class EngineTo, class EngineFrom> const EngineTo& do_cache(const EngineFrom& eng);

      template <class Engine>
      const Engine& do_cache(const Engine& eng)
      {
            return eng;
      }

      template <bool Cond, class Cache1, class Cache2>
      struct cache_chooser;

      template <class Cache1, class Cache2>
      struct cache_chooser<true, Cache1, Cache2>
      {
            typedef Cache1 chosen_t;
            static chosen_t& choose(const Cache1& x, const Cache2&)
            {
                  return const_cast<chosen_t&>(x);
            }
      };

      template <class Cache1, class Cache2>
      struct cache_chooser<false, Cache1, Cache2>
      {
            typedef Cache2 chosen_t;
            static chosen_t& choose(const Cache1&, const Cache2& y)
            {
                  return const_cast<chosen_t&>(y);
            }
      };

      template <unsigned int K, unsigned int N>
      struct take_min
      {
            enum { result = K < N ? K : N };
      };

      template <unsigned int K, unsigned int N>
      struct take_max
      {
            enum { result = K > N ? K : N };
      };

      template <unsigned int K, unsigned int N>
      struct is_eq
      {
            enum { result = K == N ? 1 : 0 };
      };

      template <typename T>
      struct data_engine_traits
      {
            typedef T cached_engine_t;
            typedef T const& ref_type_t;
      };
}

#endif // TACHY_CALC_CACHE_H__INCLUDED

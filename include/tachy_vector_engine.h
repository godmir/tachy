#if !defined(TACHY_VECTOR_ENGINE_H__INCLUDED)
#define TACHY_VECTOR_ENGINE_H__INCLUDED

#include <vector>

#include "tachy_arch_traits.h"
#include "tachy_aligned_allocator.h"
#include "tachy_cacheable.h"
#include "tachy_date.h"

namespace tachy
{
      template <typename NumType>
      class vector_engine : public cacheable
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef aligned_allocator<NumType, arch_traits_t::align> allocator_t;
            typedef std::vector<NumType, allocator_t> storage_t;

            vector_engine(const tachy_date& start_date, const std::vector<NumType>& data) :
                  _data(data.begin(), data.end()),
                  _start_date(start_date)
            {}

            vector_engine(const vector_engine& other) : cacheable(other),
                  _data(other._data),
                  _start_date(other._start_date)
            {}

            vector_engine(const tachy_date& start_date, unsigned int size, NumType value) :
                  _data(size, value),
                  _start_date(start_date)
            {}

            vector_engine(const tachy_date& start_date, unsigned int size) :
                  _data(size, NumType(0)),
                  _start_date(start_date)
            {}

            // virtual because it inherits from cacheable
            virtual ~vector_engine()
            {}

            virtual vector_engine* clone() const
            {
                  return new vector_engine(*this);
            }

            vector_engine& operator= (const vector_engine& other)
            {
                  if (&other != this)
                  {
                        // doing the right thing trumps doing it fast:
                        // hence we're copying everything here
                        // assignment that only copies projection - not history - can be done with a view/proxy object
                        _data = other._data;
                        _start_date = other._start_date;
                  }
                  return *this;
            }

            vector_engine& operator= (const std::vector<NumType>& src)
            {
                  int delta = src.size() - size();
                  if (delta > 0)
                        _data.resize(_data.size() + delta, NumType(0));
                  copy(src.begin(), src.end(), begin());
            }

            typename arch_traits_t::packed_t get_packed(int idx) const
            {
                  // can this be improved? or is it faster to go with unaligned load than to branch?
                  return arch_traits_t::loadu(&_data[idx]);
            }

            NumType operator[] (int idx) const
            {
                  return _data[idx];
            }

            NumType& operator[] (int idx)
            {
                  return _data[idx];
            }

            NumType operator[] (const tachy_date& dt) const
            {
                  return _data[dt - _start_date];
            }

            NumType at(const tachy_date& dt) const
            {
                  return _data[std::max(0, dt - _start_date)];
            }
            
            unsigned int size() const
            {
                  return _data.size();
            }

            typename storage_t::const_iterator begin() const
            {
                  return _data.begin();
            }

            typename storage_t::iterator begin()
            {
                  return _data.begin();
            }

            typename storage_t::const_iterator end() const
            {
                  return _data.end();
            }

            typename storage_t::iterator end()
            {
                  return _data.end();
            }

            NumType front() const
            {
                  return _data[0];
            }

            NumType& front()
            {
                  return _data[0];
            }

            NumType back() const
            {
                  return _data.back();
            }

            NumType& back()
            {
                  return _data.back();
            }

            tachy_date get_start_date() const
            {
                  return _start_date;
            }

            void set_start_date(const tachy_date& start_date)
            {
                  int diff = start_date - _start_date;
                  if (diff > 0) // new date is later, chop off some history
                  {
                        for (int i = diff, i_max = _data.size(); i < i_max; ++i)
                              _data[i-diff] = _data[i];
                        _data.resize(_data.size() - diff, NumType(0));
                  }
                  else if (diff < 0) // new date is earlier - add 0's
                  {
                        diff = -diff; // for clarity
                        _data.resize(_data.size() + diff, NumType(0));
                        for (int i = _data.size(); i > diff; --i)
                              _data[i] = _data[i-diff];
                  }
            }
            
      private:
            storage_t  _data;
            tachy_date _start_date;
      };
}

#endif // TACHY_VECTOR_ENGINE_H__INCLUDED

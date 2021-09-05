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
                  _start_date(start_date),
                  _guard(0)
            {}

            vector_engine(const vector_engine& other) : cacheable(other),
                  _data(other._data),
                  _start_date(other._start_date),
                  _guard(0) // no need to copy because storage is not shared between this and other
            {}

            vector_engine(const tachy_date& start_date, unsigned int size, NumType value) :
                  _data(size, value),
                  _start_date(start_date),
                  _guard(0)
            {}

            vector_engine(const tachy_date& start_date, unsigned int size) :
                  _data(size, NumType(0)),
                  _start_date(start_date),
                  _guard(0)
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
                        _data = other._data;
                        _start_date = other._start_date;
                        // _guard is unchanged (not sure it's right: but generally _guard is about storage, not actual values)
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

            void set_packed(int idx, typename arch_traits_t::packed_t value)
            {
                  *(typename arch_traits_t::packed_t*)(&_data[idx]) = value;
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

            void reset(const tachy_date& new_start_date, unsigned int new_size)
            {
                  int diff = new_start_date - _start_date;
                  if (diff > 0) // new date is later, chop off some history
                  {
                        // ... by moving values to the lower indices
                        for (int i = diff, i_max = std::min(size(), diff + new_size); i < i_max; ++i)
                              _data[i-diff] = _data[i];
                        // ... and adjusting the tail as necessary
                        unsigned int old_size = size();
                        _data.resize(new_size, NumType(0));
                        // ... zero out the tail
                        for (int i = std::max<int>(0, old_size - diff); i < new_size; ++i)
                              _data[i] = NumType(0);
                  }
                  else if (diff < 0) // new date is earlier - add 0's
                  {
                        diff = -diff; // for clarity
                        _data.resize(new_size, NumType(0));
                        for (int i = new_size-1; i >= diff; --i)
                              _data[i] = _data[i-diff];
                        for (int i = 0; i < diff; ++i)
                              _data[i] = NumType(0);
                  }
                  else if (new_size != size()) // same start date, simply resize
                        _data.resize(new_size, NumType(0));
                  _start_date = new_start_date;
            }

            void incr_assign_guard() const
            {
                  ++_guard;
            }

            void decr_assign_guard() const
            {
                  if (_guard > 0)
                        --_guard;
            }

            bool is_guarded() const
            {
                  return _guard > 0;
            }
            
      private:
            storage_t  _data;
            tachy_date _start_date;

            mutable int _guard;
      };
}

#endif // TACHY_VECTOR_ENGINE_H__INCLUDED

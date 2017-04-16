#if !defined(TACHY_VECTOR_ENGINE_H)
#define TACHY_VECTOR_ENGINE_H

#include <vector>

#include "tachy_arch_traits.h"
#include "tachy_aligned_allocator.h"
#include "tachy_cacheable.h"

namespace tachy
{
      template <typename NumType>
      class vector_engine : public cacheable
      {
      public:
            typedef arch_traits<NumType, ACTIVE_ARCH_TYPE> arch_traits_t;
            typedef aligned_allocator<NumType, arch_traits_t::align> allocator_t;
            typedef std::vector<NumType, allocator_t> storage_t;

            vector_engine(const std::vector<NumType>& data, unsigned int start) :
                  _data(data.begin(), data.end()),
                  _num_hist(start)
            {}

            vector_engine(const vector_engine& other) : cacheable(other),
                  _data(other._data),
                  _num_hist(other._num_hist)
            {}

            explicit vector_engine(const std::vector<NumType>& data) :
                  _data(data.begin(), data.end()),
                  _num_hist(0)
            {}

            vector_engine(unsigned int size, NumType value) :
                  _data(size, value),
                  _num_hist(0)
            {}

            vector_engine(unsigned int size, unsigned int start) :
                  _data(size, NumType(0)),
                  _num_hist(start)
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
                        _num_hist = other._num_hist;
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
                  return arch_traits_t::loadu(&_data[idx + _num_hist]);
            }

            NumType operator[] (int idx) const
            {
                  return _data[idx + _num_hist];
            }

            NumType& operator[] (int idx)
            {
                  return _data[idx + _num_hist];
            }

            unsigned int size() const
            {
                  return _data.size() - _num_hist;
            }

            typename storage_t::const_iterator begin() const
            {
                  return _data.begin() + _num_hist;
            }

            typename storage_t::iterator begin()
            {
                  return _data.begin() + _num_hist;
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
                  return _data[_num_hist];
            }

            NumType& front()
            {
                  return _data[_num_hist];
            }

            NumType back() const
            {
                  return _data.back();
            }

            NumType& back()
            {
                  return _data.back();
            }

            typename storage_t::const_iterator begin_hist() const
            {
                  return _data.begin();
            }

            typename storage_t::const_iterator end_hist() const
            {
                  return begin();
            }

            NumType front_hist() const
            {
                  return _data[0];
            }

            unsigned int get_num_hist() const
            {
                  return _num_hist;
            }

            // TODO: need a less crude way to set history size
            void set_num_hist(unsigned int num_hist)
            {
                  _num_hist = num_hist;
            }
            
            // note that while the "projection" section remains intact
            // "history" section is resized as necessary
            void set_hist(const std::vector<NumType>& from)
            {
                  if (false == from.empty())
                  {
                        if (_num_hist > 0)
                              _data.erase(_data.begin(), _data.begin()+_num_hist);
                        _data.insert(_data.begin(), from.begin(), from.end());
                        _num_hist = from.size();
                  }
            }

      private:
            storage_t    _data;
            unsigned int _num_hist;
      };
}

#endif // TACHY_VECTOR_ENGINE_H

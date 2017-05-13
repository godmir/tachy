#if !defined(TACHY_CACHEABLE_H__INCLUDED)
#define TACHY_CACHEABLE_H__INCLUDED

namespace tachy
{
      class cacheable
      {
      public:
            cacheable()
            {}

            cacheable(const cacheable& /* other */)
            {}
            
            virtual ~cacheable()
            {}

            virtual cacheable* clone() const = 0;

      private:
            cacheable& operator= (const cacheable& /* other */)
            {
                  return *this;
            }
      };
}

#endif // TACHY_CACHEABLE_H__INCLUDED

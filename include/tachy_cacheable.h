#ifndef TACHY_CACHEABLE_H
#define TACHY_CACHEABLE_H

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

#endif // TACHY_CACHEABLE_H

#if !defined(TACHY_TIME_SHIFT_H__INCLUDED)
#define TACHY_TIME_SHIFT_H__INCLUDED

namespace tachy
{
      class time_shift
      {
      private:
            int _shift;

      public:
            time_shift() : _shift(0) {}
            time_shift(int shift) : _shift(shift) {}
            time_shift(const time_shift& other) : _shift(other._shift) {}

            time_shift& operator= (const time_shift& other)
            {
                  _shift = other._shift;
                  return *this;
            }

            int get_time_shift() const { return _shift; }

            time_shift& operator+=(int shift)
            {
                  _shift += shift;
                  return *this;
            }

            time_shift& operator-=(int shift)
            {
                  _shift -= shift;
                  return *this;
            }
      };

      inline time_shift operator+(const time_shift& a_time_shift, int delta)
      {
            return time_shift(a_time_shift.get_time_shift() + delta);
      }

      inline time_shift operator+(int delta, const time_shift& a_time_shift)
      {
            return time_shift(a_time_shift.get_time_shift() + delta);
      }

      inline time_shift operator-(const time_shift& a_time_shift, int delta)
      {
            return time_shift(a_time_shift.get_time_shift() - delta);
      }

      // note that there is no "int - time_shift" version: it's unclear what it would mean
}

#endif // TACHY_TIME_SHIFT_H__INCLUDED

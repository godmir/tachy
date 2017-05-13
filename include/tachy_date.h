#if !defined(TACHY_DATE_H__INCLUDED)
#define TACHY_DATE_H__INCLUDED

#include "tachy_util.h"

namespace tachy
{
      class tachy_date
      {
      public:
            static tachy_date min_date()
            {
                  return tachy_date(1001);
            }
            
            explicit tachy_date(unsigned int dt) throw(tachy::exception)
                  : _y(dt/100),
                    _m(dt%100)
            {
                  if (not is_valid())
                        TACHY_THROW("Invalid date: " << dt);
            }

            tachy_date(const tachy_date& other) :
                  _y(other._y),
                  _m(other._m)
            {}

            bool is_valid() const
            {
                  return 0 < _m && _m < 13 && 0 < _y && _y < 10000;
            }

            tachy_date& operator= (const tachy_date& other)
            {
                  _y = other._y;
                  _m = other._m;
                  return *this;
            }

            tachy_date& operator+= (int months) throw(tachy::exception)
            {
                  bool valid = false;
                  int y = _y + months/12;
                  int m = _m + months%12;
                  if (y > 0)
                  {
                        if (m > 12)
                        {
                              _y = y + 1;
                              _m = m - 12;
                        }
                        else if (m < 1)
                        {
                              _y = y - 1;
                              _m = m + 12;
                        }
                        else
                        {
                              _y = y;
                              _m = m;
                        }
                        valid = is_valid();
                  }
                  if (not valid)
                        TACHY_THROW("Got invalid date after adding " << months << " months");
                  return *this;
            }

            tachy_date& operator-= (int months)
            {
                  return *this += -months;
            }

            tachy_date& operator++ ()
            {
                  return *this += 1;
            }

            tachy_date operator++ (int)
            {
                  tachy_date dt(*this);
                  *this += 1;
                  return dt;
            }
            
            int year() const
            {
                  return _y;
            }

            int month() const
            {
                  return _m;
            }

            unsigned int as_uint() const
            {
                  return 100*_y + _m;
            }

      private:
            unsigned int _y;
            unsigned int _m;
      };

      tachy_date operator+ (const tachy_date& dt, int months)
      {
            tachy_date dt_new(dt);
            return dt_new += months;
      }
      
      tachy_date operator- (const tachy_date& dt, int months)
      {
            tachy_date dt_new(dt);
            return dt_new += -months;
      }

      int operator- (const tachy_date& dt2, const tachy_date& dt1)
      {
            return 12*(dt2.year() - dt1.year()) + dt2.month() - dt1.month();
      }

      bool operator== (const tachy_date& dt2, const tachy_date& dt1)
      {
            return dt2.year() == dt1.year() and dt2.month() == dt1.month();
      }

      bool operator!= (const tachy_date& dt2, const tachy_date& dt1)
      {
            return not (dt2 == dt1);
      }

      bool operator< (const tachy_date& dt2, const tachy_date& dt1)
      {
            return dt2.year() < dt1.year() or (dt2.year() == dt1.year() and dt2.month() < dt1.month());
      }

      bool operator<= (const tachy_date& dt2, const tachy_date& dt1)
      {
            return dt2.year() < dt1.year() or (dt2.year() == dt1.year() and dt2.month() <= dt1.month());
      }

      bool operator> (const tachy_date& dt2, const tachy_date& dt1)
      {
            return not (dt2 <= dt1);
      }

      bool operator>= (const tachy_date& dt2, const tachy_date& dt1)
      {
            return not (dt2 < dt1);
      }
}

#endif // TACHY_DATE_H__INCLUDED

#if !defined(TACHY_EXCEPTION_H__INCLUDED)
#define TACHY_EXCEPTION_H__INCLUDED

#include <exception>
#include <string>
#include <sstream>

namespace tachy
{
      class exception : public std::exception
      {
      public:
            explicit exception(const std::string& what)
                  : _what(what)
            {}

            exception(const std::string& what, unsigned int line, const std::string& file)
            {
                  std::ostringstream s;
                  s << "Exception: " << file << "[" << line << "]: " << what;
                  _what = s.str();
            }

            virtual ~exception() _GLIBCXX_USE_NOEXCEPT
            {}

            virtual const char* what() const _GLIBCXX_USE_NOEXCEPT
            {
                  return _what.c_str();
            }

      protected:
            std::string _what;
      };
}
#endif // TACHY_EXCEPTION_H__INCLUDED

#if !defined(TACHY_UTIL_H__INCLUDED)
#define TACHY_UTIL_H__INCLUDED

#include <sstream>
#include <exception>
#include <iostream>

#include "tachy_exception.h"

#define TACHY_CT_DEBUG 0 // set to 1 to enable compile time debug msgs

#if defined(TACHY_VERBOSE)
#define TACHY_LOG(x) std::cout << x << std::endl
#else
#define TACHY_LOG(x) {}
#endif

#define TACHY_THROW(x) { std::ostringstream s; s << x; throw tachy::exception(s.str(), __LINE__, __FILE__); }

#endif // TACHY_UTIL_H__INCLUDED

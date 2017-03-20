#if !defined(TACHY_LAGGED_VECTOR_H__INCLUDED)
#define TACHY_LAGGED_VECTOR_H__INCLUDED

#include "tachy_lagged_engine.h"
#include "tachy_vector.h"

namespace tachy
{
      template <typename NumType, class Engine, unsigned int Level>
      calc_vector<NumType, lagged_engine<NumType, Engine, true>, Level> lag_checked(unsigned int lag, const calc_vector<NumType, Engine, Level>& x)
      {
            std::string hashed_id = x.cache().get_hash_key(std::string("LAGCK ") + x.get_id());
            return calc_vector<NumType, lagged_engine<NumType, Engine, true>, Level>(hashed_id, x.get_start_date(), lagged_engine<NumType, Engine, true>(x.engine(), lag));
      }
      
      template <typename NumType, class Engine, unsigned int Level>
      calc_vector<NumType, lagged_engine<NumType, Engine, false>, Level> lag_unchecked(unsigned int lag, const calc_vector<NumType, Engine, Level>& x)
      {
#if 0
           static vector<char> id;
            size_t sz = x.get_id().size() + 8;
            if (id.size() < sz)
                  id.resize(sz, '\0');
            sprintf(&id[0], "ULAG_%d(%s)", lag, x.get_id().c_str());
            std::string hashed_id = x.cache().getHashKey(std::string("LAG ") + &id[0]);
#else
            std::string hashed_id = x.cache().getHashKey(std::string("LAG ") + x.get_id());
#endif
            return calc_vector<NumType, lagged_engine<NumType, Engine, false>, Level>(hashed_id, x.get_start_date(), lagged_engine<NumType, Engine, false>(x.engine(), lag));
      }
}

#endif // TACHY_LAGGED_VECTOR_H__INCLUDED

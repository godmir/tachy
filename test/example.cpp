#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <deque>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <sstream>
#include <memory>
using namespace std;

#include <sys/time.h>

#include "tachy.h"

#if defined(TACHY_EXAMPLE_MODEL_REAL_IS_FLOAT)
typedef float real_t;
#else
typedef double real_t;
#endif

/*****---------------------------------------- Sun Mar  2 2014 ----------*****/

static void random(real_t x0, real_t x1, tachy::calc_vector<real_t, tachy::vector_engine<real_t>, 0>& vec)
{
      unsigned int iMax = vec.size();
      real_t xc = 0.5*(x0 + x1);
      real_t dx = 0.5*(x1 - x0);
      for (unsigned int i = 0; i < iMax; ++i)
            vec[i] = real_t(xc + dx*double(random())/RAND_MAX);
}

/*****---------------------------------------- Sun Mar  2 2014 ----------*****/

struct Model
{
      const unsigned int nProj;
      const real_t eiOffset;

      typedef tachy::linear_spline_uniform<real_t> Spline_t;

      Spline_t baseRefi;
      
      Model(int date, unsigned int numProj) :
            nProj(numProj),
            eiOffset(0.97)
      {
            typedef tachy::spline_util<real_t>::xy_pair_t XYPair_t;
            vector<XYPair_t> lsPoints(8, XYPair_t());
            lsPoints[0] = XYPair_t(0.0, 0.01);
            lsPoints[1] = XYPair_t(0.03, 0.03);
            lsPoints[2] = XYPair_t(0.06, 0.05);
            lsPoints[3] = XYPair_t(0.09, 0.01);
            lsPoints[4] = XYPair_t(0.12, -0.04);
            lsPoints[5] = XYPair_t(0.15, -0.03);
            lsPoints[6] = XYPair_t(0.20, -0.02);
            lsPoints[7] = XYPair_t(0.25, -0.01);
            baseRefi = Spline_t("refi", lsPoints);
      }
};

/*****---------------------------------------- Sun May 19 2013 ----------*****/

class PmtCalc
{
private:
      int _minTerm;
      typedef tachy::arch_traits<real_t, tachy::ACTIVE_ARCH_TYPE> ArchTraits_t;
      typedef tachy::aligned_allocator<real_t, ArchTraits_t::align> Allocator_t;
      typedef vector<real_t, Allocator_t> Vector_t;
      mutable Vector_t _cachedTail;
      
      void calcPmtsConstRate(real_t rate,
                             typename Vector_t::iterator p,
                             const typename Vector_t::const_iterator& pLast,
                             bool invert) const
      {
            int term = pLast - p;
            real_t c = rate/1200.0;
            int iMax = std::max(0, term - _minTerm);
            if (invert)
            {
                  for (int i = 0; i < iMax; ++i, ++p)
                        *p = (1.0 - 1.0/pow(1.0 + c, term-i))/c;
            }
            else
            {
                  for (int i = 0; i < iMax; ++i, ++p)
                        *p = c/(1.0 - 1.0/pow(1.0 + c, term-i));
            }
            if (_minTerm > 0)
            {
                  real_t x = c/(1.0 - 1.0/pow(1.0 + c, _minTerm));
                  fill_n(p, std::min(term, _minTerm), invert ? 1.0/x : x);
            }
      }

public:
      explicit PmtCalc(int minTerm)
            : _minTerm(minTerm)
      {}

      PmtCalc(const PmtCalc& other)
            : _minTerm(other._minTerm),
              _cachedTail(other._cachedTail)
      {}

      PmtCalc& operator= (const PmtCalc& other)
      {
            if (this != &other)
            {
                  _minTerm = other._minTerm;
                  _cachedTail = other._cachedTail;
            }
            return *this;
      }

      ~PmtCalc()
      {}

      template <unsigned int Level>
      void calcPmts(tachy::calc_vector<real_t, tachy::vector_engine<real_t>, Level>& pmts, real_t rate) const
      {
            calcPmtsConstRate(rate, pmts.engine().begin(), pmts.engine().end(), false);
      }

      template <class RatesEngine>
      void calcInvPmts(tachy::calc_vector<real_t, tachy::vector_engine<real_t>, 0>& invPmts,
                       const tachy::calc_vector<real_t, RatesEngine, 0>& rates) const
      {
            int term = invPmts.size();
            int iMaxRates = rates.size();
            typename tachy::vector_engine<real_t>::storage_t::iterator ip = invPmts.engine().begin();
            int iMax0 = std::min<int>(iMaxRates, std::max<int>(0, term - _minTerm));
            for (int i = 0; i < iMax0; ++i, ++ip)
            {
                  real_t ri = rates[i]/1200.0;
                  *ip = (1.0 - 1.0/pow(1.0 + ri, term-i))/ri;
            }
            if (iMax0 == iMaxRates)
                  calcPmtsConstRate(rates[iMaxRates-1], ip, invPmts.engine().end(), true);
            else if (iMax0 > 0)
            {
                  int iMax1 = std::min<int>(rates.size(), term);
                  for (int i = iMax0; i < iMax1; ++i, ++ip)
                  {
                        real_t ri = rates[i]/1200.0;
                        *ip = (1.0 - 1.0/pow(1.0 + ri, _minTerm))/ri;
                  }
                  if (iMax1 == iMaxRates)
                        calcPmtsConstRate(rates[iMaxRates-1], ip, invPmts.engine().end(), true);
            }
      }
};

/*****---------------------------------------- Sun May 19 2013 ----------*****/

class Pool : public tachy::calc_cache<real_t, 2>
{
public:
      typedef tachy::calc_cache<real_t, 2> Cache_t;
      typedef tachy::calc_vector<real_t, tachy::vector_engine<real_t>, 2> CachedVector_t;
            
      string id;
      real_t wac;
      real_t als;
      real_t dFee;
      real_t elbow;
      
      int    wam;
      int    wala;
      int    origTerm;

      Pool(const string& id,
           real_t wac,
           real_t als,
           real_t dFee,
           real_t elbowShift,
           int wam,
           int wala,
           int origTerm) : Cache_t(id)
      {
            this->wac = wac;
            this->als = als;
            this->dFee = dFee;
            this->elbow = elbowShift;
            this->wam = wam;
            this->wala = wala;
            this->origTerm = origTerm;
      }

      Pool(const Pool& other) : Cache_t(other),
                                wac(other.wac),
                                als(other.als),
                                dFee(other.dFee),
                                elbow(other.elbow),
                                wam(other.wam),
                                wala(other.wala),
                                origTerm(other.origTerm)
      {}

      Pool& operator= (const Pool& other)
      {
            if (this != &other)
            {
                  static_cast<Cache_t&>(*this) = static_cast<const Cache_t&>(other);
                  wac = other.wac;
                  als = other.als;
                  dFee = other.dFee;
                  elbow = other.elbow;
                  wam = other.wam;
                  wala = other.wala;
                  origTerm = other.origTerm;
            }

            return *this;
      }

      ~Pool()
      {}

      const CachedVector_t getPmts(const PmtCalc& pmtCalc, int projDate)
      {
            const char* key = "pmts";
            if (false == this->has_key(key))
            {
                  CachedVector_t pmts(key, projDate, this->wam, *this);
                  pmtCalc.calcPmts(pmts, this->wac);
            }
            return CachedVector_t(key, projDate, *this);
      }

      const CachedVector_t getAmort(const PmtCalc& pmtCalc, int projDate)
      {
            const char* key = "amort";
            if (false == this->has_key(key))
            {
                  CachedVector_t amort(key, projDate, this->wam, *this);
                  const CachedVector_t pmts = getPmts(pmtCalc, projDate);
                  amort[0] = this->als;
                  real_t cpn1 = 1.0 + this->wac/1200.0;
                  for (int i = 1; i < this->wam; ++i)
                        amort[i] = amort[i-1]*(cpn1 - pmts[i-1]);
            }
            return CachedVector_t(key, projDate, *this);
      }
};

/*****---------------------------------------- Wed Mar  5 2014 ----------*****/

typedef tachy::calc_vector<real_t, tachy::vector_engine<real_t>, 0> CVec0_t;
typedef tachy::calc_vector<real_t, tachy::vector_engine<real_t>, 2> CVec2_t;
typedef tachy::calc_vector<real_t, tachy::iota_engine<real_t>, 2> AgeVec2_t;

void runAll(const Model& model, const vector<Pool*>& collateral, int projDate, int numPaths)
{
      const unsigned int nProj = model.nProj;

      PmtCalc pmtCalc(120);
      
      CVec0_t mtg("mtgRate", projDate, nProj);

      char pathKey[8];

      vector<real_t> mtgHist(360, 4.51);
      mtg.set_history(mtgHist);

      long unsigned int ut = 0;
      struct timeval tv;
      
      for (int nthPath = 0; nthPath < numPaths; ++nthPath)
      {
            random(0.01, 5.0, mtg);

            memset(pathKey, '\0', sizeof(pathKey));
            sprintf(pathKey, "%d", nthPath + 1);

            cout << "Running path " << pathKey << endl;

            gettimeofday(&tv, 0);
            long unsigned int t0 = 1000000*tv.tv_sec + tv.tv_usec;
            
            for (unsigned int ithPool = 0; ithPool < collateral.size(); ++ithPool)
            {
                  Pool* p = collateral[ithPool];

                  CVec2_t actPmts = p->getPmts(pmtCalc, projDate);
                  CVec2_t actAmort = p->getAmort(pmtCalc, projDate);

                  // these vectors can also be declared outside of the loop to save on ctor/dtor invocations
                  CVec0_t wouldBeInvPmts1(string("pmts1") + pathKey, projDate, actPmts.size());
                  CVec0_t wouldBeInvPmts2(string("pmts2") + pathKey, projDate, actPmts.size());
                  CVec0_t wouldBeInvPmts3(string("pmts3") + pathKey, projDate, actPmts.size());
                  CVec0_t pmtRatio1("pmtRatio1", projDate, nProj);
                  CVec0_t pmtRatio2("pmtRatio2", projDate, nProj);
                  CVec0_t pmtRatio3("pmtRatio3", projDate, nProj);
                  CVec0_t burnout("burnout", projDate, nProj);
                  CVec0_t smrRefi("smrRefi", projDate, nProj);

                  tachy::time_shift t;
                  
                  pmtCalc.calcInvPmts(wouldBeInvPmts1, mtg[t-2]);
                  pmtCalc.calcInvPmts(wouldBeInvPmts2, mtg[t-2] + p->dFee);
                  pmtCalc.calcInvPmts(wouldBeInvPmts3, mtg[t-2] + p->dFee + p->elbow);

                  pmtRatio1 = actPmts*wouldBeInvPmts1 - model.eiOffset;
                  pmtRatio2 = actPmts*wouldBeInvPmts2 - model.eiOffset;
                  pmtRatio3 = actPmts*wouldBeInvPmts3 - model.eiOffset;

                  burnout[0] = 0.0;
                  for (unsigned int i = 1; i < nProj; ++i)
                        burnout[i] = 0.98*burnout[i-1] + max(0.0, min(pmtRatio1[i], 0.2));

                  AgeVec2_t wala("wala", projDate, AgeVec2_t::data_engine_t(p->wala, p->wam), *p);

                  smrRefi = (1.0 - 0.25*exp(1.0 - actAmort/200000.0))*(1.0 + 0.2*exp(-wala/36.0))*(0.6*model.baseRefi(pmtRatio3) + 0.4*model.baseRefi(pmtRatio3[t+1]) - 1.0)*exp(0.25*burnout);

                  // other pieces of total smm can be done similarly
            }

            gettimeofday(&tv, 0);
            ut += 1000000*tv.tv_sec + tv.tv_usec - t0;
      }

      cout << "Active model time = " << 1e-6*ut << " sec" << endl;
}

/*****---------------------------------------- Thu May 16 2013 ----------*****/

int main(int argc, char** argv)
{
      int numPools = 100;
      int numPaths = 200;
      
      if (argc == 3)
      {
            numPools = atoi(argv[1]);
            numPaths = atoi(argv[2]);
      }
      
      cout << "Running with " << numPools << " pools for " << numPaths << " paths" << endl;

      int projDate = 20130510;

      vector<Pool*> collateral;
      collateral.reserve(numPools);
      for (int i = 0; i < numPools; ++i)
      {
            real_t wac = real_t(4.0 + 0.75*double(random())/RAND_MAX);
            real_t als = real_t(175000.0 + 75000.0*double(random())/RAND_MAX);
            real_t dfee = real_t(0.45 + 0.25*double(random()/RAND_MAX));
            real_t elbow = real_t(-0.5 + double(random()/RAND_MAX));
            int wam    = 350 + (8*random())/RAND_MAX;
            int term   = 360;
            stringstream id;
            id << i+1;
            Pool* p = new Pool(id.str(), wac, als, dfee, elbow, wam, term - wam, term);
            collateral.push_back(p);
            
            cout << "Created pool " << p->get_id() << endl;
      }

      const unsigned int nProj = 360;

      Model model(projDate, nProj);
      
      runAll(model, collateral, projDate, numPaths);
      
      for (vector<Pool*>::iterator i = collateral.begin(); i != collateral.end(); ++i)
            delete *i;

      return 0;
}

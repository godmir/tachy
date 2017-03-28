#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <limits>

#include "tachy_arch_traits.h"
#include "tachy_aligned_allocator.h"
#include "tachy_vector_engine.h"
#include "tachy_iota_engine.h"
#include "tachy_lagged_engine.h"
#include "tachy_vector.h"
#include "tachy_expression.h"
#include "tachy_static_functor_engine.h"
#include "tachy_spline_util.h"
#include "tachy_linear_spline_incr_slope.h"
#include "tachy_linear_spline_uniform.h"
#include "tachy_linear_spline_uniform_index.h"
#include "tachy_mod_linear_spline_uniform.h"

class tachy_arch_traits_test_double : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::arch_traits<real_t, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

      real_t x[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));
      real_t y[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));

      arch_traits_t::packed_t u;
      arch_traits_t::packed_t v;
      
      int ix[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));
      int iy[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));

      arch_traits_t::index_t iu;
      arch_traits_t::index_t iv;
      
public:

      void setUp()
      {
            for (int i = 0; i < arch_traits_t::stride; ++i)
            {
                  x[i] = real_t(random())/RAND_MAX;
                  y[i] = real_t(random())/RAND_MAX;
                  ix[i] = int(100.0*x[i]);
                  iy[i] = int(100.0*y[i]);
                  if (iy[i] == 0)
                        iy[i] = 1;
            }
            u = arch_traits_t::loada(x);
            v = arch_traits_t::loada(y);
            iu = arch_traits_t::iload(ix);
            iv = arch_traits_t::iload(iy);
      }
      
      void test_zero()
      {
            TS_TRACE("test_zero");
            arch_traits_t::packed_t z = arch_traits_t::zero();
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], 0.0);
      }

      void test_set1()
      {
            TS_TRACE("test_set1");
            arch_traits_t::packed_t z = arch_traits_t::set1(x[0]);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[0]);
      }

      void test_iset1()
      {
            TS_TRACE("test_iset1");
            arch_traits_t::index_t iz = arch_traits_t::iset1(ix[0]);
            for (int i = 0; i < arch_traits_t::stride; ++i)
            {
#if TACHY_SIMD_VERSION==6 // when compiled with "-mfma -O3" TS_ASSERT_EQUALS shows garbage for the first element of iz - unless iz is used other statements
                  std::ostringstream msg;
                  msg << i << ", " << ix[0] << ", " << ((int*)&iz)[i];
                  TSM_ASSERT_EQUALS(msg.str(), ((int*)&iz)[i], ix[0]);
#else
                  TS_ASSERT_EQUALS(((int*)&iz)[i], ix[0]);
#endif
            }
      }
      
      void test_cvti()
      {
            TS_TRACE("test_cvti");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::index_t idx = arch_traits_t::cvti(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
            {
                  int k = ((int*)(&idx))[i];
                  int j = int(x[i] + 0.5);
                  if (k != j)
                  {
                        std::cout << i << ", " << x[i] << std::endl;
#if 0
                        std::ostringstream s;
                        s << i << ", " << x[i];
                        const char* msg = s.str().c_str();
                        TSM_ASSERT_EQUALS(msg, k, j);
#endif
                  }
                  TS_ASSERT_EQUALS(k, j);
            }
      }

      void test_floor()
      {
            TS_TRACE("test_floor");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::packed_t z = arch_traits_t::floor(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::floor(x[i]));
      }

      void test_ceil()
      {
            TS_TRACE("test_ceil");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::packed_t z = arch_traits_t::ceil(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::ceil(x[i]));
      }

      void test_add()
      {
            TS_TRACE("test_add");
            arch_traits_t::packed_t z = arch_traits_t::add(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i] + y[i]);
      }

      void test_sub()
      {
            TS_TRACE("test_sub");
            arch_traits_t::packed_t z = arch_traits_t::sub(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i] - y[i]);
      }
      
      void test_mul()
      {
            TS_TRACE("test_mul");
            arch_traits_t::packed_t z = arch_traits_t::mul(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i]*y[i]);
      }
      
      void test_div()
      {
            TS_TRACE("test_div");
            arch_traits_t::packed_t z = arch_traits_t::div(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i]/y[i]);
      }

      void test_max()
      {
            TS_TRACE("test_max");
            arch_traits_t::packed_t z = arch_traits_t::max(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::max(x[i], y[i]));
      }

      void test_min()
      {
            TS_TRACE("test_min");
            arch_traits_t::packed_t z = arch_traits_t::min(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::min(x[i], y[i]));
      }

      void test_iadd()
      {
            TS_TRACE("test_iadd");
            arch_traits_t::index_t iz = arch_traits_t::iadd(iu, iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], ix[i] + iy[i]);
      }

      void test_isub()
      {
            TS_TRACE("test_isub");
            arch_traits_t::index_t iz = arch_traits_t::isub(iu, iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], ix[i] - iy[i]);
      }
      
      void test_imul()
      {
            TS_TRACE("test_imul");
            arch_traits_t::index_t iz = arch_traits_t::imul(iu, iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], ix[i]*iy[i]);
      }
      
      void test_idiv()
      {
            TS_TRACE("test_idiv");
            arch_traits_t::index_t iz = arch_traits_t::idiv(iu, iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], ix[i]/iy[i]);
      }

      void test_imax()
      {
            TS_TRACE("test_imax");
            arch_traits_t::index_t iz = arch_traits_t::imax(ix[0], iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], std::max(ix[0], iy[i]));
      }

      void test_imin()
      {
            TS_TRACE("test_imin");
            arch_traits_t::index_t iz = arch_traits_t::imin(ix[0], iv);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], std::min(ix[0], iy[i]));
      }

      void test_exp()
      {
            TS_TRACE("test_exp");
            arch_traits_t::packed_t z = arch_traits_t::exp(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::exp(x[i]));
      }

      void test_sqrt()
      {
            TS_TRACE("test_sqrt");
            arch_traits_t::packed_t z = arch_traits_t::sqrt(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::sqrt(x[i]));
      }

      void test_neg()
      {
            TS_TRACE("test_neg");
            arch_traits_t::packed_t z = arch_traits_t::neg(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], -x[i]);
      }

      void test_gather()
      {
            TS_TRACE("test_gather");
            std::vector<real_t> src(500, 0.0);
            for (int i = 0; i < src.size(); ++i)
                  src[i] = real_t(random())/RAND_MAX;
            arch_traits_t::index_t idx;
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  ((int*)&idx)[i] = int((arch_traits_t::stride + 1)*double(random())/RAND_MAX)%arch_traits_t::stride;
            arch_traits_t::packed_t z = arch_traits_t::gather(&src[0], idx);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], src[((int*)(&idx))[i]]);
      }

      void test_igather()
      {
            TS_TRACE("test_igather");
            std::vector<int> src(500, 0);
            for (int i = 0; i < src.size(); ++i)
                  src[i] = random();
            arch_traits_t::index_t idx;
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  ((int*)&idx)[i] = int((arch_traits_t::stride + 1)*double(random())/RAND_MAX)%arch_traits_t::stride;
            arch_traits_t::index_t iz = arch_traits_t::igather(&src[0], idx);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((int*)&iz)[i], src[((int*)(&idx))[i]]);
      }
};

class tachy_arch_traits_test_float : public CxxTest::TestSuite
{
private:
      typedef float real_t;
      typedef tachy::arch_traits<real_t, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;

      real_t x[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));
      real_t y[arch_traits_t::stride] __attribute__ ((aligned(sizeof(arch_traits_t::packed_t))));

      arch_traits_t::packed_t u;
      arch_traits_t::packed_t v;
      
public:

      void setUp()
      {
            for (int i = 0; i < arch_traits_t::stride; ++i)
            {
                  x[i] = real_t(random())/RAND_MAX;
                  y[i] = real_t(random())/RAND_MAX;
            }
            u = arch_traits_t::loada(x);
            v = arch_traits_t::loada(y);
      }
      
      void test_zero()
      {
            TS_TRACE("test_zero");
            arch_traits_t::packed_t z = arch_traits_t::zero();
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], 0.0);
      }

      void test_set1()
      {
            TS_TRACE("test_set1");
            arch_traits_t::packed_t z = arch_traits_t::set1(x[0]);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[0]);
      }

      void test_cvti()
      {
            TS_TRACE("test_cvti");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::index_t idx = arch_traits_t::cvti(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
            {
                  int k = ((int*)(&idx))[i];
                  real_t frac = x[i] - int(x[i]);
                  int j = frac < 0.5f ? int(x[i]) : int(x[i]) + 1;
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << k;
                  TSM_ASSERT_EQUALS(msg.str().c_str(), (((int*)(&idx))[i]), int(x[i] + 0.5f));
            }
      }

      void test_floor()
      {
            TS_TRACE("test_floor");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::packed_t z = arch_traits_t::floor(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::floor(x[i]));
      }

      void test_ceil()
      {
            TS_TRACE("test_ceil");
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  x[i] *= 10.0;
            u = arch_traits_t::loada(x);
            arch_traits_t::packed_t z = arch_traits_t::ceil(u);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::ceil(x[i]));
      }

      void test_add()
      {
            TS_TRACE("test_add");
            arch_traits_t::packed_t z = arch_traits_t::add(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i] + y[i]);
      }

      void test_sub()
      {
            TS_TRACE("test_sub");
            arch_traits_t::packed_t z = arch_traits_t::sub(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i] - y[i]);
      }
      
      void test_mul()
      {
            TS_TRACE("test_mul");
            arch_traits_t::packed_t z = arch_traits_t::mul(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i]*y[i]);
      }
      
      void test_div()
      {
            TS_TRACE("test_div");
            arch_traits_t::packed_t z = arch_traits_t::div(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], x[i]/y[i]);
      }

      void test_max()
      {
            TS_TRACE("test_max");
            arch_traits_t::packed_t z = arch_traits_t::max(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::max(x[i], y[i]));
      }

      void test_min()
      {
            TS_TRACE("test_min");
            arch_traits_t::packed_t z = arch_traits_t::min(u, v);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::min(x[i], y[i]));
      }

      void test_exp()
      {
            TS_TRACE("test_exp");
            arch_traits_t::packed_t z = arch_traits_t::exp(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::exp(x[i]));
      }

      void test_sqrt()
      {
            TS_TRACE("test_sqrt");
            arch_traits_t::packed_t z = arch_traits_t::sqrt(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], std::sqrt(x[i]));
      }

      void test_neg()
      {
            TS_TRACE("test_neg");
            arch_traits_t::packed_t z = arch_traits_t::neg(arch_traits_t::loada(x));
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], -x[i]);
      }
};

class tachy_aligned_allocator_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::arch_traits<real_t, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
      typedef tachy::aligned_allocator<real_t, arch_traits_t::align> allocator_t;
      
public:
      void test_allocator()
      {
            TS_TRACE("test_allocator");

            std::vector<double, allocator_t> vec(500, 0.0);
            const double* const addr = &vec[0];
            TS_ASSERT_EQUALS(size_t(addr)%arch_traits_t::align, 0);
      }
};

class tachy_vector_engine_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::vector_engine<real_t> engine_t;
      typedef std::vector<real_t>          vector_t;

      unsigned int offset;
      vector_t src;
            
public:

      void setUp()
      {
            offset = 12;
            src.resize(500, 0.0);
            for (vector_t::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            engine_t eng0(src);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng0[i]);

            engine_t eng1(src, offset);
            for (int i = offset; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng1[i-offset]);

            engine_t eng2(eng1);
            for (int i = offset; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng2[i-offset]);

            const real_t value = real_t(random())/RAND_MAX;
            engine_t eng3(src.size(), value);
            for (int i = 0; i < eng3.size(); ++i)
                  TS_ASSERT_EQUALS(eng3[i], value);

            engine_t eng4(src.size(), offset);
            for (int i = 0; i < eng4.size(); ++i)
                  TS_ASSERT_EQUALS(eng4[i], 0.0);
      }

      void test_clone()
      {
            TS_TRACE("test_clone");

            engine_t eng0(src);

            engine_t* eng1 = eng0.clone();
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS((*eng1)[i], src[i]);
      }

      void test_iterators()
      {
            TS_TRACE("test_iterators");

            engine_t eng(src, offset);

            TS_ASSERT_EQUALS(*eng.begin(), src[offset]);
            TS_ASSERT_EQUALS(*(eng.end()-1), src.back());
            TS_ASSERT_EQUALS(*eng.begin_hist(), src[0]);
            TS_ASSERT_EQUALS(*eng.end_hist(), *eng.begin());
      }

      void test_accessors()
      {
            TS_TRACE("test_accessors");

            engine_t eng(src, offset);

            TS_ASSERT_EQUALS(eng.front(), src[offset]);
            TS_ASSERT_EQUALS(eng.back(), src.back());
            TS_ASSERT_EQUALS(eng.front_hist(), src[0]);
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");

            engine_t eng(src, offset);

            TS_ASSERT_EQUALS(eng.size(), src.size() - offset);
            TS_ASSERT_EQUALS(eng.get_num_hist(), offset);
      }

      void test_set_history()
      {
            TS_TRACE("test_set_history");

            engine_t eng(src, offset);
            for (int i = offset; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng[i-offset]);
            TS_ASSERT_EQUALS(eng.get_num_hist(), offset);
            
            std::vector<real_t> hist(100, 20.0);
            eng.set_hist(hist);
            TS_ASSERT_EQUALS(eng.get_num_hist(), hist.size());
            for (int i = offset; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng[i-offset]);
            for (int i = 0; i < hist.size(); ++i)
                  TS_ASSERT_EQUALS(hist[i], *(eng.begin_hist() + i));
            TS_ASSERT_EQUALS(eng.get_num_hist(), hist.size());
      }
};

class tachy_iota_engine_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::arch_traits<real_t, tachy::ACTIVE_ARCH_TYPE> arch_traits_t;
      typedef tachy::iota_engine<real_t> engine_t;
      
public:

      void test_iota()
      {
            engine_t eng0;
            TS_ASSERT_EQUALS(eng0.size(), 0);

            unsigned int size = 500;
            int start = 0;
            engine_t eng1(start, size);
            TS_ASSERT_EQUALS(eng1.size(), size);
            for (int i = 0; i < eng1.size(); ++i)
                  TS_ASSERT_EQUALS(eng1[i], i + start);

            engine_t eng2 = eng1;
            TS_ASSERT_EQUALS(eng2.size(), eng1.size());
            for (int i = 0; i < eng2.size(); ++i)
                  TS_ASSERT_EQUALS(eng2[i], i + start);

            start = 12;
            size = 400;
            engine_t eng3(start, size);
            eng2 = eng3;
            TS_ASSERT_EQUALS(eng2.size(), size);
            for (int i = 0; i < eng2.size(); ++i)
                  TS_ASSERT_EQUALS(eng2[i], i + start);

            int idx = 9;
            arch_traits_t::packed_t z = eng2.get_packed(idx);
            for (int i = 0; i < arch_traits_t::stride; ++i)
                  TS_ASSERT_EQUALS(((real_t*)&z)[i], idx + eng2[0] + i);
      }
};

class tachy_lagged_engine_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef std::vector<real_t> vector_t;
      typedef tachy::vector_engine<real_t> raw_engine_t;
      typedef tachy::lagged_engine<real_t, raw_engine_t, true> c_engine_t;
      typedef tachy::lagged_engine<real_t, raw_engine_t, false> u_engine_t;      
      
      std::vector<real_t> src;
            
public:

      void setUp()
      {
            src.resize(500, 0.0);
            for (vector_t::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_lagged_engine()
      {
            raw_engine_t eng0(src);

            unsigned int lag = 2;
            c_engine_t leng1(eng0, lag);
            TS_ASSERT_EQUALS(src.size(), leng1.size());
            for (int i = 0; i < lag; ++i)
                  TS_ASSERT_EQUALS(src[0], leng1[i]);
            for (int i = lag; i < leng1.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-lag], leng1[i]);

#if 1 // ideally, I'd like no copying of lagged engines
            c_engine_t leng2(leng1);
            for (int i = 0; i < lag; ++i)
                  TS_ASSERT_EQUALS(src[0], leng2[i]);
            for (int i = lag; i < leng1.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-lag], leng2[i]);
#endif
          
            u_engine_t uleng(eng0, lag);
            TS_ASSERT_EQUALS(src.size(), uleng.size());
            for (int i = lag; i < uleng.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-lag], uleng[i]);
      }
};

class tachy_vector_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::vector_engine<real_t> engine_t;
      typedef tachy::calc_vector<real_t, engine_t, 0U> vector_t;

      int date;
      std::vector<real_t> src;
            
public:

      void setUp()
      {
            date = 20170310;
            src.resize(500, 0.0);
            for (std::vector<real_t>::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            vector_t v0("v0", date, src);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v0[i]);

            vector_t v1 = v0;
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v1[i]);

            vector_t v2("v2", date, src.size());
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(real_t(0), v2[i]);
      }

      void test_clone()
      {
            TS_TRACE("test_clone");
      }

      void test_iterators()
      {
            TS_TRACE("test_iterators");
      }

      void test_accessors()
      {
            TS_TRACE("test_accessors");
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");
      }

      void test_set_history()
      {
            TS_TRACE("test_set_history");
      }
};

class tachy_lagged_vector_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::vector_engine<real_t> engine_t;
      typedef tachy::calc_vector<real_t, engine_t, 0U> vector_t;

      int date;
      std::vector<real_t> src;
            
public:

      void setUp()
      {
            date = 20170310;
            src.resize(500, 0.0);
            for (std::vector<real_t>::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            vector_t v0("v0", date, src);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v0[i]);

            tachy::time_shift t(2);
            for (int i = 0; i < t.get_time_shift(); ++i)
                  TS_ASSERT_EQUALS(src[0], v0[t][i]);
            for (int i = t.get_time_shift(); i < v0.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-t.get_time_shift()], v0[t][i]);

            vector_t v1 = v0[t];
            TS_ASSERT_EQUALS(src.size(), v1.size());
            for (int i = 0; i < t.get_time_shift(); ++i)
                  TS_ASSERT_EQUALS(src[0], v1[i]);
            for (int i = t.get_time_shift(); i < v1.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-t.get_time_shift()], v1[i]);
      }

      void test_clone()
      {
            TS_TRACE("test_clone");
      }

      void test_iterators()
      {
            TS_TRACE("test_iterators");
      }

      void test_accessors()
      {
            TS_TRACE("test_accessors");
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");
      }

      void test_set_history()
      {
            TS_TRACE("test_set_history");
      }
};

class tachy_expression_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;
      typedef tachy::vector_engine<real_t> engine_t;
      typedef tachy::calc_vector<real_t, engine_t, 0U> vector_t;
      typedef tachy::calc_cache<real_t, 1U> cache_t;
      typedef tachy::calc_vector<real_t, engine_t, cache_t::cache_level> cached_vector_t;
      
      int date;
      std::vector<real_t> src[6];

      void set_vector(unsigned int sz, std::vector<real_t>& vec)
      {
            vec.resize(sz, 0.0);
            for (std::vector<real_t>::iterator i = vec.begin(); i != vec.end(); ++i)
            {
                  *i = 2.0*(real_t(random())/RAND_MAX - 1.0);
                  if (std::abs(*i) < 1e-6)
                        *i = 1e-6;
            }
      }
      
public:

      void setUp()
      {
            unsigned int sz = 500;
            date = 20170310;
            for (int i = 1; i < sizeof(src)/sizeof(src[0]); ++i)
                  set_vector(sz, src[i]);
            src[0].resize(sz, 0.0);
      }
      
      void test_2op()
      {
            TS_TRACE("test_2op");

            vector_t x("x", date, src[2]);
            vector_t y("y", date, src[1]);
            vector_t r("r", date, src[0]);

            const real_t delta =  2.0*std::numeric_limits<real_t>::epsilon();

            r = x + y;
            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(src[2][i] + src[1][i], r[i], std::max(1.0, std::abs(r[i]))*delta);

            r = x - y;
            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(src[2][i] - src[1][i], r[i], std::max(1.0, std::abs(r[i]))*delta);

            r = x * y;
            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(src[2][i] * src[1][i], r[i], std::max(1.0, std::abs(r[i]))*delta);

            r = x / y;
            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(src[2][i] / src[1][i], r[i], std::max(1.0, std::abs(r[i]))*delta);
      }

      void test_3op()
      {
            TS_TRACE("test_3op");

            vector_t x("x", date, src[3]);
            vector_t y("y", date, src[2]);
            vector_t z("z", date, src[1]);
            vector_t r("r", date, src[0]);
            
            r = x + y + z;
            for (int i = 0; i < r.size(); ++i)
            {
                  const real_t delta = 2.0*std::abs(r[i])*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(src[3][i] + src[2][i] + src[1][i], r[i], delta);
            }

            r = x - y * z;
            for (int i = 0; i < r.size(); ++i)
            {
                  const real_t delta = 2.0*std::abs(r[i])*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(src[3][i] - src[2][i] * src[1][i], r[i], delta);
            }

            r = x * y / z;
            for (int i = 0; i < r.size(); ++i)
            {
                  const real_t delta = 2.0*std::abs(r[i])*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(src[3][i] * src[2][i] / src[1][i], r[i], delta);
            }

            r = (x + y) / z;
            for (int i = 0; i < r.size(); ++i)
            {
                  const real_t delta = 2.0*std::abs(r[i])*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA((src[3][i] + src[2][i])/src[1][i], r[i], delta);
            }
      }

      void test_caching_1()
      {
            TS_TRACE("test_caching_1");

            cache_t cache("c0");
            
            cached_vector_t u("u", date, src[1], cache);
            cached_vector_t v("v", date, src[2], cache);
            cached_vector_t w("w", date, src[3], cache);
            
            for (int k = 0; k < 10; ++k)
            {
                  set_vector(src[4].size(), src[4]);

                  vector_t x("x", date, src[4]);
                  vector_t r("r", date, src[0]);

                  r = u*(1.0 + v)/(2.0 - w) + (1.0 - v)*x;
                  
                  for (int i = 0; i < r.size(); ++i)
                  {
                        const real_t delta = 5.0*std::numeric_limits<real_t>::epsilon();
                        real_t chk = src[1][i]*(1.0 + src[2][i])/(2.0 - src[3][i]) + (1.0 - src[2][i])*src[4][i];
                        std::ostringstream msg;
                        msg << i << ", " << chk << ", " << r[i] << ", " << chk - r[i] << ", " << delta;
                        TSM_ASSERT_DELTA(msg.str().c_str(), chk, r[i], delta);
                  }
            }

            unsigned int num_cached = 0;
            for (cache_t::cache_engine_t::const_iterator i = cache.begin(); i != cache.end(); ++i)
            {
                  ++num_cached;
                  //std::cout << "\n" << num_cached << " cached item: " << i->first << std::endl;
            }
            TS_ASSERT_EQUALS(2, num_cached);
      }

      void test_static_functors()
      {
            TS_TRACE("test_static_functors");

            vector_t x("x", date, src[1]);
            vector_t r("r", date, src[0]);

            r = exp(-x);

            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_EQUALS(exp(-src[1][i]), r[i]);
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");
      }

      void test_set_history()
      {
            TS_TRACE("test_set_history");
      }
};

class tachy_gcd_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;

      static int gcd(unsigned int u, unsigned int v)
      {
            if (u == v || u == 0 || v == 0)
                  return u|v;
            if ((u&1) == 0)
            { // if u is even
                  if ((v&1) == 0) // if u and v are even
                        return 2*gcd(u/2, v/2);
                  else // u is even and v is odd
                        return  gcd(u/2, v);
            }
            else if ((v&1) == 0) // if u is odd and v is even
                  return gcd(u, v/2);
            else
            { // both are odd
                  if (u > v)
                        return gcd((u-v)/2, v);
                  else
                        return gcd((v-u)/2, u);
            }
      }
      
public:

      void test_gcd()
      {
            TS_TRACE("test_gcd");

            for (int i = 0; i < 1000; ++i)
            {
                  int u = int(100.0*double(random())/RAND_MAX);
                  int v = int(100.0*double(random())/RAND_MAX);

                  int gcd0 = gcd(u, v);
                  int gcd1 = tachy::spline_util<real_t>::gcd(u, v);

                  TS_ASSERT_EQUALS(gcd0, gcd1);
            }
      }
};

class tachy_linear_spline_test : public CxxTest::TestSuite
{
private:
      typedef double real_t;

      typedef tachy::spline_util<real_t>::xy_pair_t xy_pair_t;
      typedef std::vector<xy_pair_t>                xy_vector_t;
      typedef std::vector<real_t>                   num_vector_t;
      typedef tachy::vector_engine<real_t> engine_t;
      typedef tachy::calc_vector<real_t, engine_t, 0U> vector_t;
      typedef tachy::calc_cache<real_t, 2U> cache_t;
      typedef tachy::calc_vector<real_t, engine_t, cache_t::cache_level> cached_vector_t;
      
      xy_vector_t pts;
      num_vector_t src;
      num_vector_t tgt;
      unsigned int date;
      
public:

      void setUp()
      {
            date = 20170310;
            
            pts.resize(8, xy_pair_t(0.0, 0.0));
            pts[0] = xy_pair_t(0.0, 0.02);
            pts[1] = xy_pair_t(0.1, 0.05);
            pts[2] = xy_pair_t(0.3, 0.08);
            pts[3] = xy_pair_t(0.4, 0.02);
            pts[4] = xy_pair_t(0.5, -0.02);
            pts[5] = xy_pair_t(0.6, -0.05);
            pts[6] = xy_pair_t(0.75, -0.08);
            pts[7] = xy_pair_t(0.85, -0.02);

            tgt.resize(500, 0.0);
            src = tgt;
            for (int i = 0; i < src.size(); ++i)
                  src[i] = -0.1 + 1.2*real_t(random())/RAND_MAX;
      }
      
      void test_incr_spline()
      {
            TS_TRACE("test_incr_spline");

            tachy::linear_spline_incr_slope<real_t> s("test", pts);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  TS_ASSERT_EQUALS(s(src[i]), y);
            }
      }

      void test_incr_spline_vector()
      {
            TS_TRACE("test_incr_spline_vector");
            
            tachy::linear_spline_incr_slope<real_t> s("test", pts);

            vector_t x("x", date, src);
            vector_t r("r", date, tgt);

            r = s(x);

            for (int i = 0; i < r.size(); ++i)
            {
                  const real_t delta = 5.0*std::numeric_limits<real_t>::epsilon();
                  real_t chk = s(src[i]);
                  std::ostringstream msg;
                  msg << i << ", " << chk << ", " << r[i] << ", " << chk - r[i] << ", " << delta;
                  TSM_ASSERT_DELTA(msg.str().c_str(), chk, r[i], delta);
            }
      }

      void test_uniform_spline()
      {
            TS_TRACE("test_uniform_spline");

            tachy::linear_spline_uniform<real_t> s("test", pts);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_uniform_spline_vector()
      {
            TS_TRACE("test_uniform_spline_vector");
            
            tachy::linear_spline_uniform<real_t> s("test", pts);
            
            vector_t x("x", date, src);
            vector_t r("r", date, tgt);

            r = s(x);

            for (int i = 0; i < r.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << src[i] << ", " << y << ", " << r[i];
                  TSM_ASSERT_DELTA(msg.str().c_str(), y, r[i], delta);
            }
      }

      void test_uniform_index_spline()
      {
            TS_TRACE("test_uniform_index_spline");

            tachy::linear_spline_uniform_index<real_t, false> s("test", pts);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_unifrom_index_spline_vector()
      {
            TS_TRACE("test_uniform_index_spline_vector");
            
            tachy::linear_spline_uniform_index<real_t, false> s("test", pts);
            
            vector_t x("x", date, src);
            vector_t r("r", date, tgt);

            r = s(x);

            for (int i = 0; i < r.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << src[i] << ", " << y << ", " << r[i];
                  TSM_ASSERT_DELTA(msg.str().c_str(), y, r[i], delta);
            }
      }

      void test_mod_uniform_index_spline()
      {
            TS_TRACE("test_mod_uniform_index_spline");

            tachy::linear_spline_uniform_index<real_t, false> s0("base", pts);

            cache_t cache("the_cache");

            unsigned int n_mod = 100;
            std::vector<cached_vector_t> modulation;
            modulation.reserve(pts.size());
            for (int i = 0; i < pts.size(); ++i)
            {
                  std::ostringstream id;
                  id << "mod " << i + 1;
                  modulation.push_back(cached_vector_t(id.str(), 20170320, n_mod, cache));
                  real_t amp = real_t(random())/RAND_MAX;
                  for (int t = 0; t < n_mod; ++t)
                        modulation[i][t] = amp*exp(-real_t(t)/n_mod);
            }

            try
            {
                  tachy::mod_linear_spline_uniform_index<real_t, 2U> s(s0, modulation);

                  for (int t = 0; t < n_mod; ++t)
                  {
                        for (int i = 0; i < src.size(); ++i)
                        {
                              real_t y = 0.0;
                              for (int k = 0; k < pts.size(); ++k)
                                    y += modulation[k][t]*pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                              const real_t delta = 20.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                              TS_ASSERT_DELTA(s(t, src[i]), y, delta);
                        }
                  }
            }
            catch (std::exception& ex)
            {
                  TSM_ASSERT(ex.what(), false);
            }
      }
};

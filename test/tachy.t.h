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
#include "tachy_date.h"

class tachy_date_test : public CxxTest::TestSuite
{
public:

      void test_dates_valid()
      {
            TS_TRACE("test_dates_valid");

            int dt[] = { 1001, 197810, 201705, 210001, 0, 15, 25000001 };
            TS_ASSERT(tachy::tachy_date(dt[0]).is_valid());
            TS_ASSERT(tachy::tachy_date(dt[1]).is_valid());
            TS_ASSERT(tachy::tachy_date(dt[2]).is_valid());
            TS_ASSERT(tachy::tachy_date(dt[3]).is_valid());
            TS_ASSERT_THROWS(not tachy::tachy_date(dt[4]).is_valid(), tachy::exception);
            TS_ASSERT_THROWS(not tachy::tachy_date(dt[5]).is_valid(), tachy::exception);
            TS_ASSERT_THROWS(not tachy::tachy_date(dt[6]).is_valid(), tachy::exception);
      }

      void test_dates_diff()
      {
            TS_TRACE("test_dates_diff");

            tachy::tachy_date dt1[] = { tachy::tachy_date(197810),
                                        tachy::tachy_date(199905),
                                        tachy::tachy_date(200501),
                                        tachy::tachy_date(201705) };
            tachy::tachy_date dt2[] = { tachy::tachy_date(196312),
                                        tachy::tachy_date(200101),
                                        tachy::tachy_date(200502),
                                        tachy::tachy_date(202004) };
            TS_ASSERT_EQUALS(-178, dt2[0] - dt1[0]);
            TS_ASSERT_EQUALS(  20, dt2[1] - dt1[1]);
            TS_ASSERT_EQUALS(   1, dt2[2] - dt1[2]);
            TS_ASSERT_EQUALS(  35, dt2[3] - dt1[3]);
      }

      void test_dates_add()
      {
            TS_TRACE("test_dates_add");

            tachy::tachy_date dt[] = { tachy::tachy_date(196312),
                                       tachy::tachy_date(200001),
                                       tachy::tachy_date(200501),
                                       tachy::tachy_date(202004) };
            int m[] = { 200, -1, 23, -35 };
            TS_ASSERT_EQUALS(tachy::tachy_date(198008), dt[0] + m[0]);
            TS_ASSERT_EQUALS(tachy::tachy_date(199912), dt[1] + m[1]);
            TS_ASSERT_EQUALS(tachy::tachy_date(200612), dt[2] + m[2]);
            TS_ASSERT_EQUALS(tachy::tachy_date(201705), dt[3] + m[3]);

            TS_ASSERT_THROWS(dt[1] + 100000, tachy::exception);
      }

};

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
#if TACHY_SIMD_VERSION==6 || TACHY_SIMD_VERSION==4 // when compiled with "-mfma -O3" or "-mavx -O3" with gcc v9 TS_ASSERT_EQUALS shows garbage for the first element of iz - unless iz is used other statements
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
                  TS_ASSERT_DELTA(((real_t*)&z)[i], std::exp(x[i]), 5.0*std::numeric_limits<real_t>::epsilon());
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
            {
#if TACHY_SIMD_VERSION==4 // when compiled with "-mavx -O3" with gcc v9 TS_ASSERT_EQUALS shows garbage for the first element of iz - unless iz is used other statements
                  std::ostringstream msg;
                  msg << i << ", " << ix[0] << ", " << ((int*)&iz)[i];
                  TSM_ASSERT_EQUALS(msg.str(), ((int*)&iz)[i], src[((int*)(&idx))[i]]);
#else
                  TS_ASSERT_EQUALS(((int*)&iz)[i], src[((int*)(&idx))[i]]);
#endif
            }
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

      unsigned int dt;
      unsigned int offset;
      vector_t src;
            
public:

      void setUp()
      {
            dt = 201705;
            offset = 12;
            src.resize(500, 0.0);
            for (vector_t::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            engine_t eng0(tachy::tachy_date(dt), src);
            TS_ASSERT_EQUALS(dt, eng0.get_start_date().as_uint());
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng0[i]);

            engine_t eng1(tachy::tachy_date(dt), src);
            tachy::tachy_date di(dt);
            for (int i = 0; i < src.size(); ++i, ++di)
                  TS_ASSERT_EQUALS(src[i], eng1[di]);

            engine_t eng2(eng1);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], eng2[i]);

            const real_t value = real_t(random())/RAND_MAX;
            engine_t eng3(tachy::tachy_date(dt), src.size(), value);
            for (int i = 0; i < eng3.size(); ++i)
                  TS_ASSERT_EQUALS(eng3[i], value);

            engine_t eng4(tachy::tachy_date(dt), src.size());
            for (int i = 0; i < eng4.size(); ++i)
                  TS_ASSERT_EQUALS(eng4[i], 0.0);
      }

      void test_clone()
      {
            TS_TRACE("test_clone");

            engine_t eng0(tachy::tachy_date(dt), src);

            engine_t* eng1 = eng0.clone();
            TS_ASSERT_EQUALS(eng1->get_start_date(), eng0.get_start_date());
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS((*eng1)[i], src[i]);
      }

      void test_iterators()
      {
            TS_TRACE("test_iterators");

            engine_t eng(tachy::tachy_date(dt), src);

            TS_ASSERT_EQUALS(*eng.begin(), src[0]);
            TS_ASSERT_EQUALS(*(eng.end()-1), src.back());
      }

      void test_accessors()
      {
            TS_TRACE("test_accessors");

            engine_t eng(tachy::tachy_date(dt), src);

            TS_ASSERT_EQUALS(eng.front(), src.front());
            TS_ASSERT_EQUALS(eng.back(), src.back());
            TS_ASSERT_EQUALS(eng[tachy::tachy_date(dt)], src.front());
            TS_ASSERT_EQUALS(eng[tachy::tachy_date(dt) + src.size() - 1], src.back());
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");

            engine_t eng(tachy::tachy_date(dt), src);

            TS_ASSERT_EQUALS(eng.size(), src.size());
      }

      void test_reset()
      {
            TS_TRACE("test_reset");

            tachy::tachy_date base_date(dt);
            engine_t base_eng(base_date, src);

            tachy::tachy_date start_dates[] = { base_date - src.size()/2,
                                                base_date,
                                                base_date + 12,
                                                base_date + src.size(),
                                                base_date + 2*src.size() };
            size_t sizes[] = { src.size()/4,
                               src.size() - 12,
                               src.size(),
                               src.size() + 12,
                               2*src.size() };
                                              
            for (int k = 0; k < sizeof(start_dates)/sizeof(start_dates[0]); ++k)
            {
                  for (int j = 0; j < sizeof(sizes)/sizeof(sizes[0]); ++j)
                  {
                        // std::cerr << k << ' ' << j << std::endl;
                        engine_t eng = base_eng;
                        eng.reset(start_dates[k], sizes[j]);
                        TS_ASSERT_EQUALS(start_dates[k], eng.get_start_date());
                        TS_ASSERT_EQUALS(sizes[j], eng.size());
                        int i0 = base_date - start_dates[k];
                        int i1 = std::min<int>(sizes[j], src.size() + i0);
                        int i = 0;
                        for ( ; i < i0; ++i)
                              TS_ASSERT_EQUALS(0.0, eng[i]);
                        for ( ; i < i1; ++i)
                              TS_ASSERT_EQUALS(base_eng[i-i0], eng[i]);
                        for ( ; i < sizes[j]; ++i)
                              TS_ASSERT_EQUALS(0.0, eng[i]);
                  }
            }
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
            tachy::tachy_date dt(201703);
            
            engine_t eng0(dt);
            TS_ASSERT_EQUALS(eng0.size(), 0);

            unsigned int size = 500;
            int start = 0;
            engine_t eng1(dt, start, size);
            TS_ASSERT_EQUALS(eng1.size(), size);
            for (int i = 0; i < eng1.size(); ++i)
                  TS_ASSERT_EQUALS(eng1[i], i + start);

            start = 12;
            size = 400;
            engine_t eng2(dt, start, size);
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
            raw_engine_t eng0(tachy::tachy_date(201705), src);

            tachy::tachy_date dt = eng0.get_start_date();
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
            date = 201703;
            src.resize(500, 0.0);
            for (std::vector<real_t>::iterator i = src.begin(); i != src.end(); ++i)
                  *i = real_t(random())/RAND_MAX;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            vector_t v0("v0", tachy::tachy_date(date), src);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v0[i]);

            vector_t v1 = v0;
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v1[i]);

            vector_t v2("v2", tachy::tachy_date(date), src.size());
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
            date = 201703;
            src.resize(500, 0.0);
            for (std::vector<real_t>::iterator i = src.begin(); i != src.end(); ++i)
                  *i = 2.0*real_t(random())/RAND_MAX - 1.0;
      }
      
      void test_ctor()
      {
            TS_TRACE("test_ctor");

            vector_t v0("v0", tachy::tachy_date(date), src);
            for (int i = 0; i < src.size(); ++i)
                  TS_ASSERT_EQUALS(src[i], v0[i]);

            int dt = 2;
            tachy::time_shift t;
            for (int i = 0; i < t.get_time_shift(); ++i)
                  TS_ASSERT_EQUALS(src[0], v0[t-dt][i]);
            for (int i = dt; i < v0.size(); ++i)
                  TS_ASSERT_EQUALS(src[i-dt], v0[t-dt][i]);

            t -= dt;
            vector_t v1 = v0[t];
            TS_ASSERT_EQUALS(src.size(), v1.size());
            for (int i = 0; i < abs(t.get_time_shift()); ++i)
                  TS_ASSERT_EQUALS(src[0], v1[i]);
            for (int i = abs(t.get_time_shift()); i < v1.size(); ++i)
                  TS_ASSERT_EQUALS(src[i+t.get_time_shift()], v1[i]);
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

      void test_assign_guard()
      {
            vector_t v0("v0", tachy::tachy_date(date), src);
            
            std::vector<real_t> expected = src;
            expected[0] = 0.75;
            for (int i = 1; i < expected.size(); ++i)
                  expected[i] = 0.75*exp(-expected[i-1]) + std::max(0.0, std::min(v0[i], 0.2));

            int dt = 1;
            tachy::time_shift t;
            v0[0] = 0.0;
            v0 = 0.75*tachy::exp(-v0[t-dt]) + tachy::max(0.0, tachy::min(v0, 0.2));

            for (int i = 0; i < src.size(); ++i)
            {
                  const real_t delta = 2.0*std::abs(expected[i])*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(expected[i], v0[i], delta);
            }
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
            date = 201703;
            for (int i = 1; i < sizeof(src)/sizeof(src[0]); ++i)
                  set_vector(sz, src[i]);
            src[0].resize(sz, 0.0);
      }
      
      void test_2op()
      {
            TS_TRACE("test_2op");

            vector_t x("x", tachy::tachy_date(date), src[2]);
            vector_t y("y", tachy::tachy_date(date), src[1]);
            vector_t r("r", tachy::tachy_date(date), src[0]);

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

            vector_t x("x", tachy::tachy_date(date), src[3]);
            vector_t y("y", tachy::tachy_date(date), src[2]);
            vector_t z("z", tachy::tachy_date(date), src[1]);
            vector_t r("r", tachy::tachy_date(date), src[0]);
            
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

      cached_vector_t make_cached_vector(cache_t& cache, const char* name, const std::vector<real_t>& src) const
      {
            cached_vector_t vec(name, tachy::tachy_date(date), src.size(), cache, false);
            for (int i = 0; i < src.size(); ++i)
                  vec[i] = src[i];
            return vec;
      }
      
      void test_caching_1()
      {
            TS_TRACE("test_caching_1");

            cache_t cache("c0");

            cached_vector_t u("u", tachy::tachy_date(date), src[1], cache, false);
            cached_vector_t v("v", tachy::tachy_date(date), src[2], cache, false);
            cached_vector_t w = make_cached_vector(cache, "w", src[3]);

            cached_vector_t* t = new cached_vector_t("t", tachy::tachy_date(date), src[3], cache, false);
            
            for (int k = 0; k < 10; ++k)
            {
                  set_vector(src[4].size(), src[4]);

                  vector_t x("x", tachy::tachy_date(date), src[4]);
                  vector_t r("r", tachy::tachy_date(date), src[0]);

                  r = u*(1.0 + v)/(2.0 - w) + (1.0 - v)*x;
                  
                  for (int i = 0; i < r.size(); ++i)
                  {
                        const real_t delta = 5.0*std::numeric_limits<real_t>::epsilon();
                        real_t chk = src[1][i]*(1.0 + src[2][i])/(2.0 - src[3][i]) + (1.0 - src[2][i])*src[4][i];
                        std::ostringstream msg;
                        msg << i << ", " << chk << ", " << r[i] << ", " << chk - r[i] << ", " << delta;
                        TSM_ASSERT_DELTA(msg.str().c_str(), chk, r[i], delta);
                  }

                  if (k == 2)
                  {
                        cached_vector_t z = u*v + 1.0;
                        cached_vector_t y = z*(*t) + 2.0;

                        z.drop();

                        delete t;
                  }
            }

            unsigned int num_cached = 0;
            for (cache_t::cache_engine_t::const_iterator i = cache.begin(); i != cache.end(); ++i)
            {
                  ++num_cached;
                  // std::cout << "\n" << num_cached << " cached item: " << i->first << std::endl;
            }
            TS_ASSERT_EQUALS(3, num_cached);
      }

      void test_static_functors()
      {
            TS_TRACE("test_static_functors");

            const real_t delta = 5.0*std::numeric_limits<real_t>::epsilon();
            
            vector_t x("x", tachy::tachy_date(date), src[1]);
            vector_t r("r", tachy::tachy_date(date), src[0]);

            r = exp(-x);

            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(std::exp(-src[1][i]), r[i], delta);

            r = log(1.1 + 0.5*x);

            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_DELTA(std::log(1.1 + 0.5*src[1][i]), r[i], delta);

            r = abs(x);

            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_EQUALS(std::abs(src[1][i]), r[i]);
      }

      void test_static_posneg_functors()
      {
            TS_TRACE("test_static_posneg_functors");

            vector_t x("x", tachy::tachy_date(date), src[1]);
            vector_t r("r", tachy::tachy_date(date), src[0]);

            r = exp(max(0.0, x));

            for (int i = 0; i < r.size(); ++i)
                  TS_ASSERT_EQUALS(std::exp(std::max(0.0, src[1][i])), r[i]);
      }

      void test_size_queries()
      {
            TS_TRACE("test_size_queries");
      }

      void test_shifted_ops()
      {
            TS_TRACE("test_shifted_ops");

            int shift = 12;
            vector_t x("x", tachy::tachy_date(date), src[1]);
            vector_t y("y", tachy::tachy_date(date) + shift, src[2]);

            vector_t r = x;
            r = y;
            TS_ASSERT_EQUALS(tachy::tachy_date(date), r.get_start_date());
            for (int i = 0; i < shift; ++i)
                  TS_ASSERT_EQUALS(x[i], r[i]);
            for (int i = shift; i < r.size(); ++i)
                  TS_ASSERT_EQUALS(y[i-shift], r[i]);

            vector_t u = x + y;
            TS_ASSERT_EQUALS(y.get_start_date(), u.get_start_date());
            const real_t delta = 2.0*std::numeric_limits<real_t>::epsilon();
            for (int i = 0; i < u.size(); ++i)
                  TS_ASSERT_DELTA(x[i+shift] + y[i], u[i], delta);
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
      xy_vector_t pts_uniform;
      num_vector_t src;
      num_vector_t tgt;
      unsigned int date;
      
public:

      void setUp()
      {
            date = 201703;
            
            pts.resize(8, xy_pair_t(0.0, 0.0));
            pts[0] = xy_pair_t(0.0, 0.02);
            pts[1] = xy_pair_t(0.1, 0.05);
            pts[2] = xy_pair_t(0.3, 0.08);
            pts[3] = xy_pair_t(0.4, 0.02);
            pts[4] = xy_pair_t(0.5, -0.02);
            pts[5] = xy_pair_t(0.6, -0.05);
            pts[6] = xy_pair_t(0.75, -0.08);
            pts[7] = xy_pair_t(0.85, -0.02);

            pts_uniform.resize(4, xy_pair_t(0.0, 0.0));
            pts_uniform[0] = xy_pair_t(0.0, 0.02);
            pts_uniform[1] = xy_pair_t(0.2, 0.05);
            pts_uniform[2] = xy_pair_t(0.4, 0.08);
            pts_uniform[3] = xy_pair_t(0.6, 0.02);

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

            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r("r", tachy::tachy_date(date), tgt);

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

      void test_uniform_spline_2()
      {
            TS_TRACE("test_uniform_spline_2");
      
            tachy::linear_spline_uniform<real_t> s("testu", pts_uniform);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts_uniform.size(); ++k)
                        y += pts_uniform[k].second*std::max<real_t>(0.0, src[i] - pts_uniform[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_uniform_spline_vector()
      {
            TS_TRACE("test_uniform_spline_vector");
            
            tachy::linear_spline_uniform<real_t> s("test", pts);
            
            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r("r", tachy::tachy_date(date), tgt);

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

            tachy::linear_spline_uniform_index<real_t, false> s("test", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size(); ++k)
                        y += pts[k].second*std::max<real_t>(0.0, src[i] - pts[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_uniform_index_spline_2()
      {
            TS_TRACE("test_uniform_index_spline_2");

            tachy::linear_spline_uniform_index<real_t, false> s("testu", pts_uniform, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts_uniform.size(); ++k)
                        y += pts_uniform[k].second*std::max<real_t>(0.0, src[i] - pts_uniform[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_uniform_index_spline_3()
      {
            TS_TRACE("test_uniform_index_spline_3");

            tachy::linear_spline_uniform_index<real_t, false> s0("test", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);

            xy_vector_t pts_xy;
            pts_xy.reserve(pts.size()+2);
            double x = 2.0*pts.front().first - pts[1].first;
            pts_xy.push_back(xy_vector_t::value_type(x, s0(x)));
            for (int i = 0; i < pts.size(); ++i)
                  pts_xy.push_back(xy_vector_t::value_type(pts[i].first, s0(pts[i].first)));
            x = 2.0*pts.back().first - pts[pts.size()-2].first;
            pts_xy.push_back(xy_vector_t::value_type(x, s0(x)));
                             
            tachy::linear_spline_uniform_index<real_t, false> s1("testxy", pts_xy, tachy::spline_util<real_t>::SPLINE_INIT_FROM_XY_POINTS);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  const real_t delta = 10.0*std::abs(s0(src[i]))*std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s1(src[i]), s0(src[i]), delta);
            }
      }

      void test_uniform_index_spline_4()
      {
            TS_TRACE("test_uniform_index_spline_4");

            tachy::linear_spline_uniform_index<real_t, false> s("test", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_LOCAL_SLOPES);
            
            for (int i = 0; i < src.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size()-1; ++k)
                        y += pts[k].second*std::max<real_t>(0.0, std::min<real_t>(src[i], pts[k+1].first) - pts[k].first);
                  y += pts.back().second*std::max<real_t>(0, src[i] - pts.back().first);
                  const real_t delta = std::abs(y)*1e-8; //std::numeric_limits<real_t>::epsilon();
                  TS_ASSERT_DELTA(s(src[i]), y, delta);
            }
      }

      void test_unifrom_index_spline_vector()
      {
            TS_TRACE("test_uniform_index_spline_vector");
            
            tachy::linear_spline_uniform_index<real_t, false> s("test", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);
            
            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r("r", tachy::tachy_date(date), tgt);

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

      void test_unifrom_index_spline_vector_2()
      {
            TS_TRACE("test_uniform_index_spline_vector_2");
            
            tachy::linear_spline_uniform_index<real_t, false> s("test_uniform", pts_uniform, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);
            
            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r("r", tachy::tachy_date(date), tgt);

            r = s(x);

            for (int i = 0; i < r.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts_uniform.size(); ++k)
                        y += pts_uniform[k].second*std::max<real_t>(0.0, src[i] - pts_uniform[k].first);
                  const real_t delta = 10.0*std::abs(y)*std::numeric_limits<real_t>::epsilon();
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << src[i] << ", " << y << ", " << r[i];
                  TSM_ASSERT_DELTA(msg.str().c_str(), y, r[i], delta);
            }
      }

      void test_unifrom_index_spline_vector_3()
      {
            TS_TRACE("test_uniform_index_spline_vector_3");
            
            tachy::linear_spline_uniform_index<real_t, false> s0("test_xs", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);
            
            xy_vector_t pts_xy;
            pts_xy.reserve(pts.size()+2);
            double x_first = 2.0*pts.front().first - pts[1].first;
            pts_xy.push_back(xy_vector_t::value_type(x_first, s0(x_first)));
            for (int i = 0; i < pts.size(); ++i)
                  pts_xy.push_back(xy_vector_t::value_type(pts[i].first, s0(pts[i].first)));
            double x_last = 2.0*pts.back().first - pts[pts.size()-2].first;
            pts_xy.push_back(xy_vector_t::value_type(x_last, s0(x_last)));
                             
            tachy::linear_spline_uniform_index<real_t, false> s1("test_xy", pts_xy, tachy::spline_util<real_t>::SPLINE_INIT_FROM_XY_POINTS);

            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r1 = s1(x);
            vector_t r0 = s0(x);
            
            for (int i = 0; i < x.size(); ++i)
            {
                  const real_t delta = 10.0*std::abs(r0[i])*std::numeric_limits<real_t>::epsilon();
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << src[i] << ", " << r0[i] << ", " << r1[i];
                  TSM_ASSERT_DELTA(msg.str().c_str(), r0[i], r1[i], delta);
            }
      }

      void test_unifrom_index_spline_vector_4()
      {
            TS_TRACE("test_uniform_index_spline_vector_4");
            
            tachy::linear_spline_uniform_index<real_t, false> s("test_uniform", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_LOCAL_SLOPES);
            
            vector_t x("x", tachy::tachy_date(date), src);
            vector_t r("r", tachy::tachy_date(date), tgt);

            r = s(x);

            for (int i = 0; i < r.size(); ++i)
            {
                  real_t y = 0.0;
                  for (int k = 0; k < pts.size()-1; ++k)
                        y += pts[k].second*std::max<real_t>(0.0, std::min<real_t>(src[i], pts[k+1].first) - pts[k].first);
                  y += pts.back().second*std::max<real_t>(0.0, src[i] - pts.back().first);
                  const real_t delta = std::abs(y)*1e-8; //std::numeric_limits<real_t>::epsilon();
                  std::ostringstream msg;
                  msg << i << ", " << x[i] << ", " << src[i] << ", " << y << ", " << r[i] << ", " << std::abs<real_t>(y - r[i]) << ", " << delta;
                  TSM_ASSERT_DELTA(msg.str().c_str(), y, r[i], delta);
            }
      }

      void test_mod_uniform_index_spline()
      {
            TS_TRACE("test_mod_uniform_index_spline");

            tachy::linear_spline_uniform_index<real_t, false> s0("base", pts, tachy::spline_util<real_t>::SPLINE_INIT_FROM_INCR_SLOPES);

            cache_t cache("the_cache");

            unsigned int n_mod = 100;
            std::vector<cached_vector_t> modulation;
            modulation.reserve(pts.size());
            for (int i = 0; i < pts.size(); ++i)
            {
                  std::ostringstream id;
                  id << "mod " << i + 1;
                  modulation.push_back(cached_vector_t(id.str(), tachy::tachy_date(201703), n_mod, cache, true));
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
                              const real_t delta = 1e-8*std::abs(y); //*std::numeric_limits<real_t>::epsilon();
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

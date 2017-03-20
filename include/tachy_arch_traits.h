#ifndef TACHY_ARCH_TRAITS_H
#define TACHY_ARCH_TRAITS_H

#if defined(__MMX__)
#include <mmintrin.h>  /* MMX  __m64 int */
#endif
#if defined(__SSE__)
#include <xmmintrin.h> /* SSE  __m128  float */
#endif
#if defined(__SSE2__)
#include <emmintrin.h> /* SSE2 __m128i: long long,  __m128d double  */
#include <mmintrin.h>  /* for indices */
#endif
#if defined(__AVX__)
#include <immintrin.h> /* AVX  __m256  float */
#include <smmintrin.h> /* for indices */
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/time.h>
#include <stdlib.h>

#include "tachy_util.h"

namespace tachy
{
      enum ARCH_TYPE
      {
            ARCH_SCALAR = 1,
            ARCH_IA_SSE,
            ARCH_IA_SSE2,
            ARCH_IA_AVX,
            ARCH_IA_AVX2
      };

      enum
      {
            ACTIVE_ARCH_TYPE
#if defined(__AVX2__) || defined(__AVX512__)
#define TACHY_SIMD_VERSION 5
            = ARCH_IA_AVX2
#if TACHY_CT_DEBUG
#warning "ARCH type is AVX2"
#endif
#elif defined(__AVX__)
#define TACHY_SIMD_VERSION 4
            = ARCH_IA_AVX
#if TACHY_CT_DEBUG
#warning "ARCH type is AVX"
#endif
#elif defined(__SSE2__)
#define TACHY_SIMD_VERSION 3
            = ARCH_IA_SSE2
#if TACHY_CT_DEBUG
#warning "ARCH type is SSE2"
#endif
#elif defined(__SSE__)
#define TACHY_SIMD_VERSION 2
            = ARCH_IA_SSE
#if TACHY_CT_DEBUG
#warning "ARCH type is SSE"
#endif
#else
#if defined(__MMX__)
#define TACHY_SIMD_VERSION 1
#else
#define TACHY_SIMD_VERSION 0
#endif
            = ARCH_SCALAR
#if TACHY_CT_DEBUG
#warning "ARCH type is SCALAR"
#endif
#endif
      };

      template <typename NumType, unsigned int ArchType> struct arch_traits
      {
            typedef NumType scalar_t;
            typedef scalar_t packed_t;
            typedef int      index_t;
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x) // aligned load
            {
                  return *x;
            }
            static inline packed_t loadu(const scalar_t* x) // unaligned load
            {
                  return *x;
            }
            static inline index_t iload(const int i)
            {
                  return i;
            }
            static inline packed_t zero()
            {
                  return packed_t(0);
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return x;
            }
            static inline index_t cvti(const packed_t x)
            {
                  return int(x + 0.5);
            }
            static inline packed_t add(const packed_t x, const packed_t y)
            {
                  return x + y;
            }
            static inline packed_t sub(const packed_t x, const packed_t y)
            {
                  return x - y;
            }
            static inline packed_t mul(const packed_t x, const packed_t y)
            {
                  return x * y;
            }
            static inline packed_t div(const packed_t x, const packed_t y)
            {
                  return x / y;
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return std::floor(x);
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return std::ceil(x);
            }
            static inline packed_t max(const packed_t a, const packed_t x)
            {
                  return std::max(a, x);
            }
            static inline packed_t min(const packed_t a, const packed_t x)
            {
                  return std::min(a, x);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  return std::max(a, i);
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  return std::min(a, i);
            }
            static inline packed_t sqrt(const packed_t x)
            {
                  return std::sqrt(x);
            }
            static inline packed_t exp(const packed_t x)
            {
                  return std::exp(x);
            }
            static inline packed_t neg(const packed_t x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return s[i];
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return x*y + c;
            }
      };

      template <> struct arch_traits<float, ARCH_IA_SSE>
      {
#if defined(__SSE__)
            typedef float scalar_t;
            typedef __m128 packed_t;
            typedef int index_t __attribute__ ((__vector_size__ (16), __may_alias__));
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x)
            {
                  return _mm_load_ps(x);
            }
            static inline packed_t loadu(const scalar_t* x)
            {
                  return _mm_setr_ps(x[0], x[1], x[2], x[3]);
            }
            static inline index_t iload(const int i)
            {
                  index_t idx = { i, i, i, i };
                  return idx;
            }
            static inline packed_t zero()
            {
                  return _mm_set1_ps(scalar_t(0));
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return _mm_set1_ps(x);
            }
            static inline index_t cvti(const packed_t& x)
            {
                  index_t idx = { int(((scalar_t*)(&x))[0] + 0.5f),
                                  int(((scalar_t*)(&x))[1] + 0.5f),
                                  int(((scalar_t*)(&x))[2] + 0.5f),
                                  int(((scalar_t*)(&x))[3] + 0.5f) };
                  return idx;
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return _mm_setr_ps(std::floor(((scalar_t*)(&x))[0]),
                                     std::floor(((scalar_t*)(&x))[1]),
                                     std::floor(((scalar_t*)(&x))[2]),
                                     std::floor(((scalar_t*)(&x))[3]));
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return _mm_setr_ps(std::ceil(((scalar_t*)(&x))[0]),
                                     std::ceil(((scalar_t*)(&x))[1]),
                                     std::ceil(((scalar_t*)(&x))[2]),
                                     std::ceil(((scalar_t*)(&x))[3]));
            }
            static inline packed_t add(const packed_t& x, const packed_t& y)
            {
                  return _mm_add_ps(x, y);
            }
            static inline packed_t sub(const packed_t& x, const packed_t& y)
            {
                  return _mm_sub_ps(x, y);
            }
            static inline packed_t mul(const packed_t& x, const packed_t& y)
            {
                  return _mm_mul_ps(x, y);
            }
            static inline packed_t div(const packed_t& x, const packed_t& y)
            {
                  return _mm_div_ps(x, y);
            }
            static inline packed_t max(const packed_t& x, const packed_t& y)
            {
                  return _mm_max_ps(x, y);
            }
            static inline packed_t min(const packed_t& x, const packed_t& y)
            {
                  return _mm_min_ps(x, y);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  index_t idx = { std::max(a, ((int*)(&i))[0]),
                                  std::max(a, ((int*)(&i))[1]),
                                  std::max(a, ((int*)(&i))[2]),
                                  std::max(a, ((int*)(&i))[3]) };
                  return idx;
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  index_t idx = { std::min(a, ((int*)(&i))[0]),
                                  std::min(a, ((int*)(&i))[1]),
                                  std::min(a, ((int*)(&i))[2]),
                                  std::min(a, ((int*)(&i))[3]) };
                  return idx;
            }
            static inline packed_t sqrt(const packed_t& x)
            {
                  return _mm_sqrt_ps(x);
            }
            static inline packed_t exp(const packed_t& x)
            {
                  return _mm_setr_ps(std::exp(((scalar_t*)(&x))[0]),
                                     std::exp(((scalar_t*)(&x))[1]),
                                     std::exp(((scalar_t*)(&x))[2]),
                                     std::exp(((scalar_t*)(&x))[3]));
            }
            static inline packed_t neg(const packed_t& x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm_setr_ps(s[((int*)(&i))[0]], s[((int*)(&i))[1]],
                                     s[((int*)(&i))[2]], s[((int*)(&i))[3]]);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return add(mul(x, y), c);
            }
#endif
      };

      template <> struct arch_traits<double, ARCH_IA_SSE2>
      {
#if defined(__SSE2__)
            typedef double scalar_t;
            typedef __m128d packed_t;
            typedef __m64 index_t;
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x)
            {
                  return _mm_load_pd(x);
            }
            static inline packed_t loadu(const scalar_t* x)
            {
                  return _mm_loadu_pd(x);
            }
            static inline index_t iload(const int i)
            {
                  return _mm_set_pi32(i, i);
            }
            static inline packed_t zero()
            {
                  return _mm_set1_pd(scalar_t(0));
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return _mm_set1_pd(x);
            }
            static inline index_t cvti(const packed_t& x)
            {
                  return _mm_cvtpd_pi32(x);
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return _mm_setr_pd(std::floor(((scalar_t*)(&x))[0]),
                                     std::floor(((scalar_t*)(&x))[1]));
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return _mm_setr_pd(std::ceil(((scalar_t*)(&x))[0]),
                                     std::ceil(((scalar_t*)(&x))[1]));
            }
            static inline packed_t add(const packed_t& x, const packed_t& y)
            {
                  return _mm_add_pd(x, y);
            }
            static inline packed_t sub(const packed_t& x, const packed_t& y)
            {
                  return _mm_sub_pd(x, y);
            }
            static inline packed_t mul(const packed_t& x, const packed_t& y)
            {
                  return _mm_mul_pd(x, y);
            }
            static inline packed_t div(const packed_t& x, const packed_t& y)
            {
                  return _mm_div_pd(x, y);
            }
            static inline packed_t max(const packed_t& x, const packed_t& y)
            {
                  return _mm_max_pd(x, y);
            }
            static inline packed_t min(const packed_t& x, const packed_t& y)
            {
                  return _mm_min_pd(x, y);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  return _mm_setr_pi32(std::max(a, ((int*)(&i))[0]),
                                       std::max(a, ((int*)(&i))[1]));
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  return _mm_setr_pi32(std::min(a, ((int*)(&i))[0]),
                                       std::min(a, ((int*)(&i))[1]));
            }
            static inline packed_t sqrt(const packed_t& x)
            {
                  return _mm_sqrt_pd(x);
            }
            static inline packed_t exp(const packed_t& x)
            {
                  return _mm_setr_pd(std::exp(((scalar_t*)(&x))[0]),
                                     std::exp(((scalar_t*)(&x))[1]));
            }
            static inline packed_t neg(const packed_t& x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm_setr_pd(s[((int*)(&i))[0]], s[((int*)(&i))[1]]);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return add(mul(x, y), c);
            }
#endif
      };

      template <> struct arch_traits<float, ARCH_IA_SSE2>
      {
#if defined(__SSE2__)
            typedef float scalar_t;
            typedef __m128 packed_t;
            typedef __m128i index_t;
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x)
            {
                  return _mm_load_ps(x);
            }
            static inline packed_t loadu(const scalar_t* x)
            {
                  return _mm_loadu_ps(x);
            }
            static inline index_t iload(const int i)
            {
                  return _mm_set_epi32(i, i, i, i);
            }
            static inline packed_t zero()
            {
                  return _mm_set1_ps(scalar_t(0));
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return _mm_set1_ps(x);
            }
            static inline index_t cvti(const packed_t& x)
            {
                  return _mm_cvtps_epi32(x);
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return _mm_setr_ps(std::floor(((scalar_t*)(&x))[0]),
                                     std::floor(((scalar_t*)(&x))[1]),
                                     std::floor(((scalar_t*)(&x))[2]),
                                     std::floor(((scalar_t*)(&x))[3]));
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return _mm_setr_ps(std::ceil(((scalar_t*)(&x))[0]),
                                     std::ceil(((scalar_t*)(&x))[1]),
                                     std::ceil(((scalar_t*)(&x))[2]),
                                     std::ceil(((scalar_t*)(&x))[3]));
            }
            static inline packed_t add(const packed_t& x, const packed_t& y)
            {
                  return _mm_add_ps(x, y);
            }
            static inline packed_t sub(const packed_t& x, const packed_t& y)
            {
                  return _mm_sub_ps(x, y);
            }
            static inline packed_t mul(const packed_t& x, const packed_t& y)
            {
                  return _mm_mul_ps(x, y);
            }
            static inline packed_t div(const packed_t& x, const packed_t& y)
            {
                  return _mm_div_ps(x, y);
            }
            static inline packed_t max(const packed_t& x, const packed_t& y)
            {
                  return _mm_max_ps(x, y);
            }
            static inline packed_t min(const packed_t& x, const packed_t& y)
            {
                  return _mm_min_ps(x, y);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  return _mm_setr_epi32(std::max(a, ((int*)(&i))[0]),
                                        std::max(a, ((int*)(&i))[1]),
                                        std::max(a, ((int*)(&i))[2]),
                                        std::max(a, ((int*)(&i))[3]));
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  return _mm_setr_epi32(std::min(a, ((int*)(&i))[0]),
                                        std::min(a, ((int*)(&i))[1]),
                                        std::min(a, ((int*)(&i))[2]),
                                        std::min(a, ((int*)(&i))[3]));
            }
            static inline packed_t sqrt(const packed_t& x)
            {
                  return _mm_sqrt_ps(x);
            }
            static inline packed_t exp(const packed_t& x)
            {
                  return _mm_setr_ps(std::exp(((scalar_t*)(&x))[0]),
                                     std::exp(((scalar_t*)(&x))[1]),
                                     std::exp(((scalar_t*)(&x))[2]),
                                     std::exp(((scalar_t*)(&x))[3]));
            }
            static inline packed_t neg(const packed_t& x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm_setr_ps(s[((int*)(&i))[0]], s[((int*)(&i))[1]],
                                     s[((int*)(&i))[2]], s[((int*)(&i))[3]]);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return add(mul(x, y), c);
            }
#endif
      };

      template <> struct arch_traits<double, ARCH_IA_AVX>
      {
#if defined(__AVX__)
            typedef double scalar_t;
            typedef __m256d packed_t;
            typedef __m128i index_t;
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x)
            {
                  return _mm256_load_pd(x);
            }
            static inline packed_t loadu(const scalar_t* x)
            {
                  return _mm256_loadu_pd(x);
            }
            static inline index_t iload(const int i)
            {
                  return _mm_set_epi32(i, i, i, i);
            }
            static inline packed_t zero()
            {
                  return _mm256_set1_pd(scalar_t(0));
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return _mm256_set1_pd(x);
            }
            static inline index_t cvti(const packed_t& x)
            {
                  return _mm256_cvtpd_epi32(x);
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return _mm256_floor_pd(x);
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return _mm256_ceil_pd(x);
            }
            static inline packed_t add(const packed_t& x, const packed_t& y)
            {
                  return _mm256_add_pd(x, y);
            }
            static inline packed_t sub(const packed_t& x, const packed_t& y)
            {
                  return _mm256_sub_pd(x, y);
            }
            static inline packed_t mul(const packed_t& x, const packed_t& y)
            {
                  return _mm256_mul_pd(x, y);
            }
            static inline packed_t div(const packed_t& x, const packed_t& y)
            {
                  return _mm256_div_pd(x, y);
            }
            static inline packed_t max(const packed_t& x, const packed_t& y)
            {
                  return _mm256_max_pd(x, y);
            }
            static inline packed_t min(const packed_t& x, const packed_t& y)
            {
                  return _mm256_min_pd(x, y);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  return _mm_max_epi32(iload(a), i);
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  return _mm_min_epi32(iload(a), i);
            }
            static inline packed_t sqrt(const packed_t& x)
            {
                  return _mm256_sqrt_pd(x);
            }
            static inline packed_t exp(const packed_t& x)
            {
                  return _mm256_setr_pd(std::exp(((scalar_t*)(&x))[0]),
                                        std::exp(((scalar_t*)(&x))[1]),
                                        std::exp(((scalar_t*)(&x))[2]),
                                        std::exp(((scalar_t*)(&x))[3]));
            }
            static inline packed_t neg(const packed_t& x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm256_setr_pd(s[((int*)(&i))[0]],
                                        s[((int*)(&i))[1]],
                                        s[((int*)(&i))[2]],
                                        s[((int*)(&i))[3]]);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return add(mul(x, y), c);
            }
#endif
      };

      template <> struct arch_traits<double, ARCH_IA_AVX2> : public arch_traits<double, ARCH_IA_AVX>
      {
#if defined(__AVX2__)
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm256_i32gather_pd(s, i, 1);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return _mm256_fmadd_pd(x, y, c);
            }
#endif
      };

      template <> struct arch_traits<float, ARCH_IA_AVX>
      {
#if defined(__AVX__)
            typedef float scalar_t;
            typedef __m256 packed_t;
            typedef int index_t __attribute__ ((__vector_size__ (32), __may_alias__));
            enum { stride = sizeof(packed_t)/sizeof(scalar_t),
                   align = sizeof(packed_t) /* in bytes */ };
            static inline packed_t loada(const scalar_t* x)
            {
                  return _mm256_load_ps(x);
            }
            static inline packed_t loadu(const scalar_t* x)
            {
                  return _mm256_loadu_ps(x);
            }
            static inline index_t iload(const int i)
            {
                  index_t idx = { i, i, i, i, i, i, i, i };
                  return idx;
            }
            static inline packed_t zero()
            {
                  return _mm256_set1_ps(scalar_t(0));
            }
            static inline packed_t set1(const scalar_t x)
            {
                  return _mm256_set1_ps(x);
            }
            static inline index_t cvti(const packed_t& x)
            {
                  index_t idx = { int(((scalar_t*)(&x))[0] + 0.5f),
                                  int(((scalar_t*)(&x))[1] + 0.5f),
                                  int(((scalar_t*)(&x))[2] + 0.5f),
                                  int(((scalar_t*)(&x))[3] + 0.5f),
                                  int(((scalar_t*)(&x))[4] + 0.5f),
                                  int(((scalar_t*)(&x))[5] + 0.5f),
                                  int(((scalar_t*)(&x))[6] + 0.5f),
                                  int(((scalar_t*)(&x))[7] + 0.5f) };
                  return idx;
            }
            static inline packed_t floor(const packed_t& x)
            {
                  return _mm256_floor_ps(x);
            }
            static inline packed_t ceil(const packed_t& x)
            {
                  return _mm256_ceil_ps(x);
            }
            static inline packed_t add(const packed_t& x, const packed_t& y)
            {
                  return _mm256_add_ps(x, y);
            }
            static inline packed_t sub(const packed_t& x, const packed_t& y)
            {
                  return _mm256_sub_ps(x, y);
            }
            static inline packed_t mul(const packed_t& x, const packed_t& y)
            {
                  return _mm256_mul_ps(x, y);
            }
            static inline packed_t div(const packed_t& x, const packed_t& y)
            {
                  return _mm256_div_ps(x, y);
            }
            static inline packed_t max(const packed_t& x, const packed_t& y)
            {
                  return _mm256_max_ps(x, y);
            }
            static inline packed_t min(const packed_t& x, const packed_t& y)
            {
                  return _mm256_min_ps(x, y);
            }
            static inline index_t imax(const int a, const index_t& i)
            {
                  index_t idx = { std::max(a, ((int*)(&i))[0]),
                                  std::max(a, ((int*)(&i))[1]),
                                  std::max(a, ((int*)(&i))[2]),
                                  std::max(a, ((int*)(&i))[3]),
                                  std::max(a, ((int*)(&i))[4]),
                                  std::max(a, ((int*)(&i))[5]),
                                  std::max(a, ((int*)(&i))[6]),
                                  std::max(a, ((int*)(&i))[7]) };
                  return idx;
            }
            static inline index_t imin(const int a, const index_t& i)
            {
                  index_t idx = { std::min(a, ((int*)(&i))[0]),
                                  std::min(a, ((int*)(&i))[1]),
                                  std::min(a, ((int*)(&i))[2]),
                                  std::min(a, ((int*)(&i))[3]),
                                  std::min(a, ((int*)(&i))[4]),
                                  std::min(a, ((int*)(&i))[5]),
                                  std::min(a, ((int*)(&i))[6]),
                                  std::min(a, ((int*)(&i))[7]) };
                  return idx;
            }
            static inline packed_t sqrt(const packed_t& x)
            {
                  return _mm256_sqrt_ps(x);
            }
            static inline packed_t exp(const packed_t& x)
            {
                  return _mm256_setr_ps(std::exp(((scalar_t*)(&x))[0]),
                                        std::exp(((scalar_t*)(&x))[1]),
                                        std::exp(((scalar_t*)(&x))[2]),
                                        std::exp(((scalar_t*)(&x))[3]),
                                        std::exp(((scalar_t*)(&x))[4]),
                                        std::exp(((scalar_t*)(&x))[5]),
                                        std::exp(((scalar_t*)(&x))[6]),
                                        std::exp(((scalar_t*)(&x))[7]));
            }
            static inline packed_t neg(const packed_t& x)
            {
                  return -x;
            }
            static inline packed_t gather(const scalar_t* s, const index_t& i)
            {
                  return _mm256_setr_ps(s[((int*)(&i))[0]],
                                        s[((int*)(&i))[1]],
                                        s[((int*)(&i))[2]],
                                        s[((int*)(&i))[3]],
                                        s[((int*)(&i))[4]],
                                        s[((int*)(&i))[5]],
                                        s[((int*)(&i))[6]],
                                        s[((int*)(&i))[7]]);
            }
            static inline packed_t fmadd(const packed_t& x, const packed_t& y, const packed_t& c)
            {
                  return add(mul(x, y), c);
            }
#endif
      };
}

#endif // TACHY_ARCH_TRAITS_H

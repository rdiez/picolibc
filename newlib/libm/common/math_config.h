/* Configuration for math routines.
   Copyright (c) 2017-2018 Arm Ltd.  All rights reserved.

   SPDX-License-Identifier: BSD-3-Clause

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. The name of the company may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

   THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef _MATH_CONFIG_H
#define _MATH_CONFIG_H

#include <math.h>
#include <stdint.h>
#include <errno.h>
#include <fenv.h>

#ifndef WANT_ROUNDING
/* Correct special case results in non-nearest rounding modes.  */
# define WANT_ROUNDING 1
#endif
#ifdef _IEEE_LIBM
# define WANT_ERRNO 0
# define _LIB_VERSION _IEEE_
#else
/* Set errno according to ISO C with (math_errhandling & MATH_ERRNO) != 0.  */
# define WANT_ERRNO 1
# define _LIB_VERSION _POSIX_
#endif
#ifndef WANT_ERRNO_UFLOW
/* Set errno to ERANGE if result underflows to 0 (in all rounding modes).  */
# define WANT_ERRNO_UFLOW (WANT_ROUNDING && WANT_ERRNO)
#endif

#define _IEEE_  -1
#define _POSIX_ 0

#ifdef _HAVE_ATTRIBUTE_ALWAYS_INLINE
#define ALWAYS_INLINE __inline__ __attribute__((__always_inline__))
#else
#define ALWAYS_INLINE __inline__
#endif

#ifdef _HAVE_ATTRIBUTE_NOINLINE
# define NOINLINE __attribute__ ((__noinline__))
#else
# define NOINLINE
#endif

#ifdef HAVE_BUILTIN_EXPECT
# define likely(x) __builtin_expect (!!(x), 1)
# define unlikely(x) __builtin_expect (x, 0)
#else
# define likely(x) (x)
# define unlikely(x) (x)
#endif

/* Compiler can inline round as a single instruction.  */
#ifndef HAVE_FAST_ROUND
# if __aarch64__
#   define HAVE_FAST_ROUND 1
# else
#   define HAVE_FAST_ROUND 0
# endif
#endif

/* Compiler can inline lround, but not (long)round(x).  */
#ifndef HAVE_FAST_LROUND
# if __aarch64__ && (100*__GNUC__ + __GNUC_MINOR__) >= 408 && __NO_MATH_ERRNO__
#   define HAVE_FAST_LROUND 1
# else
#   define HAVE_FAST_LROUND 0
# endif
#endif

/* Compiler can inline fma as a single instruction.  */
#ifndef HAVE_FAST_FMA
# if __aarch64__ || (__ARM_FEATURE_FMA && (__ARM_FP & 8)) || __riscv_flen >= 64
#   define HAVE_FAST_FMA 1
# else
#   define HAVE_FAST_FMA 0
# endif
#endif

#ifndef HAVE_FAST_FMAF
# if HAVE_FAST_FMA || (__ARM_FEATURE_FMA && (__ARM_FP & 4)) || __riscv_flen >= 32
#  define HAVE_FAST_FMAF 1
# else
#  define HAVE_FAST_FMAF 0
# endif
#endif

#if HAVE_FAST_ROUND
/* When set, the roundtoint and converttoint functions are provided with
   the semantics documented below.  */
# define TOINT_INTRINSICS 1

/* Round x to nearest int in all rounding modes, ties have to be rounded
   consistently with converttoint so the results match.  If the result
   would be outside of [-2^31, 2^31-1] then the semantics is unspecified.  */
static ALWAYS_INLINE double_t
roundtoint (double_t x)
{
  return round (x);
}

/* Convert x to nearest int in all rounding modes, ties have to be rounded
   consistently with roundtoint.  If the result is not representible in an
   int32_t then the semantics is unspecified.  */
static ALWAYS_INLINE int32_t
converttoint (double_t x)
{
# if HAVE_FAST_LROUND
  return lround (x);
# else
  return (long) round (x);
# endif
}
#endif

#ifndef TOINT_INTRINSICS
# define TOINT_INTRINSICS 0
#endif

static ALWAYS_INLINE uint32_t
asuint (float f)
{
#if defined(__riscv_flen) && __riscv_flen >= 32
  uint32_t result;
  __asm__("fmv.x.w\t%0, %1" : "=r" (result) : "f" (f));
  return result;
#else
  union
  {
    float f;
    uint32_t i;
  } u = {f};
  return u.i;
#endif
}

static ALWAYS_INLINE float
asfloat (uint32_t i)
{
#if defined(__riscv_flen) && __riscv_flen >= 32
  float result;
  __asm__("fmv.w.x\t%0, %1" : "=f" (result) : "r" (i));
  return result;
#else
  union
  {
    uint32_t i;
    float f;
  } u = {i};
  return u.f;
#endif
}

static ALWAYS_INLINE int32_t
_asint32 (float f)
{
    return (int32_t) asuint(f);
}

static ALWAYS_INLINE int
_sign32(int32_t ix)
{
    return ((uint32_t) ix) >> 31;
}

static ALWAYS_INLINE int
_exponent32(int32_t ix)
{
    return (ix >> 23) & 0xff;
}

static ALWAYS_INLINE int32_t
_significand32(int32_t ix)
{
    return ix & 0x7fffff;
}

static ALWAYS_INLINE float
_asfloat(int32_t i)
{
    return asfloat((uint32_t) i);
}

static ALWAYS_INLINE uint64_t
asuint64 (double f)
{
#if defined(__riscv_flen) && __riscv_flen >= 64 && __riscv_xlen >= 64
  uint64_t result;
  __asm__("fmv.x.d\t%0, %1" : "=r" (result) : "f" (f));
  return result;
#else
  union
  {
    double f;
    uint64_t i;
  } u = {f};
  return u.i;
#endif
}

static ALWAYS_INLINE double
asdouble (uint64_t i)
{
#if defined(__riscv_flen) && __riscv_flen >= 64 && __riscv_xlen >= 64
  double result;
  __asm__("fmv.d.x\t%0, %1" : "=f" (result) : "r" (i));
  return result;
#else
  union
  {
    uint64_t i;
    double f;
  } u = {i};
  return u.f;
#endif
}

static ALWAYS_INLINE int64_t
_asint64(double f)
{
    return (int64_t) asuint64(f);
}

static ALWAYS_INLINE int
_sign64(int64_t ix)
{
    return ((uint64_t) ix) >> 63;
}

static ALWAYS_INLINE int
_exponent64(int64_t ix)
{
    return (ix >> 52) & 0x7ff;
}

static ALWAYS_INLINE int64_t
_significand64(int64_t ix)
{
    return ix & 0xfffffffffffffLL;
}

static ALWAYS_INLINE double
_asdouble(int64_t i)
{
    return asdouble((uint64_t) i);
}

#ifndef IEEE_754_2008_SNAN
# define IEEE_754_2008_SNAN 1
#endif
static ALWAYS_INLINE int
issignalingf_inline (float x)
{
  uint32_t ix = asuint (x);
  if (!IEEE_754_2008_SNAN)
    return (ix & 0x7fc00000) == 0x7fc00000;
  return 2 * (ix ^ 0x00400000) > 0xFF800000u;
}

static ALWAYS_INLINE int
issignaling_inline (double x)
{
  uint64_t ix = asuint64 (x);
  if (!IEEE_754_2008_SNAN)
    return (ix & 0x7ff8000000000000) == 0x7ff8000000000000;
  return 2 * (ix ^ 0x0008000000000000) > 2 * 0x7ff8000000000000ULL;
}

#ifdef PICOLIBC_FLOAT_NOEXCEPT
#define FORCE_FLOAT     float
#define pick_float_except(expr,val)    (val)
#else
#define FORCE_FLOAT     volatile float
#define pick_float_except(expr,val)    (expr)
#endif

#ifdef PICOLIBC_DOUBLE_NOEXCEPT
#define FORCE_DOUBLE    double
#define pick_double_except(expr,val)    (val)
#else
#define FORCE_DOUBLE    volatile double
#define pick_double_except(expr,val)    (expr)
#endif

static ALWAYS_INLINE float
opt_barrier_float (float x)
{
  FORCE_FLOAT y = x;
  return y;
}

static ALWAYS_INLINE double
opt_barrier_double (double x)
{
  FORCE_DOUBLE y = x;
  return y;
}

static ALWAYS_INLINE void
force_eval_float (float x)
{
  FORCE_FLOAT y = x;
  (void) y;
}

static ALWAYS_INLINE void
force_eval_double (double x)
{
  FORCE_DOUBLE y = x;
  (void) y;
}

/* Evaluate an expression as the specified type, normally a type
   cast should be enough, but compilers implement non-standard
   excess-precision handling, so when FLT_EVAL_METHOD != 0 then
   these functions may need to be customized.  */
static ALWAYS_INLINE float
eval_as_float (float x)
{
  return x;
}
static ALWAYS_INLINE double
eval_as_double (double x)
{
  return x;
}

/* gcc emitting PE/COFF doesn't support visibility */
#if defined (__GNUC__) && !defined (__CYGWIN__)
# define HIDDEN __attribute__ ((__visibility__ ("hidden")))
#else
# define HIDDEN
#endif

/* Error handling tail calls for special cases, with a sign argument.
   The sign of the return value is set if the argument is non-zero.  */

/* The result overflows.  */
HIDDEN float __math_oflowf (uint32_t);
/* The result underflows to 0 in nearest rounding mode.  */
HIDDEN float __math_uflowf (uint32_t);
/* The result underflows to 0 in some directed rounding mode only.  */
HIDDEN float __math_may_uflowf (uint32_t);
/* Division by zero.  */
HIDDEN float __math_divzerof (uint32_t);
/* The result overflows.  */
HIDDEN double __math_oflow (uint32_t);
/* The result underflows to 0 in nearest rounding mode.  */
HIDDEN double __math_uflow (uint32_t);
/* The result underflows to 0 in some directed rounding mode only.  */
HIDDEN double __math_may_uflow (uint32_t);
/* Division by zero.  */
HIDDEN double __math_divzero (uint32_t);

/* Error handling using input checking.  */

/* Invalid input unless it is a quiet NaN.  */
HIDDEN float __math_invalidf (float);
/* Invalid input unless it is a quiet NaN.  */
HIDDEN double __math_invalid (double);

/* Error handling using output checking, only for errno setting.  */

/* Check if the result overflowed to infinity.  */
HIDDEN float __math_check_oflowf (float);
/* Check if the result overflowed to infinity.  */
HIDDEN double __math_check_oflow (double);
/* Check if the result underflowed to 0.  */
HIDDEN double __math_check_uflow (double);

/* Check if the result overflowed to infinity.  */
static inline double
check_oflow (double x)
{
  return WANT_ERRNO ? __math_check_oflow (x) : x;
}

/* Check if the result overflowed to infinity.  */
static inline float
check_oflowf (float x)
{
  return WANT_ERRNO ? __math_check_oflowf (x) : x;
}

/* Check if the result underflowed to 0.  */
static inline double
check_uflow (double x)
{
  return WANT_ERRNO ? __math_check_uflow (x) : x;
}

/* Set inexact exception */
#if defined(FE_INEXACT) && !defined(PICOLIBC_DOUBLE_NOEXECPT)
double __math_inexact(double);
void __math_set_inexact(void);
#else
#define __math_inexact(val) (val)
#define __math_set_inexact()
#endif

#if defined(FE_INEXACT) && !defined(PICOLIBC_FLOAT_NOEXECPT)
float __math_inexactf(float val);
void __math_set_inexactf(void);
#else
#define __math_inexactf(val) (val)
#define __math_set_inexactf()
#endif

/* Shared between expf, exp2f and powf.  */
#define EXP2F_TABLE_BITS 5
#define EXP2F_POLY_ORDER 3
extern const struct exp2f_data
{
  uint64_t tab[1 << EXP2F_TABLE_BITS];
  double shift_scaled;
  double poly[EXP2F_POLY_ORDER];
  double shift;
  double invln2_scaled;
  double poly_scaled[EXP2F_POLY_ORDER];
} __exp2f_data HIDDEN;

#define LOGF_TABLE_BITS 4
#define LOGF_POLY_ORDER 4
extern const struct logf_data
{
  struct
  {
    double invc, logc;
  } tab[1 << LOGF_TABLE_BITS];
  double ln2;
  double poly[LOGF_POLY_ORDER - 1]; /* First order coefficient is 1.  */
} __logf_data HIDDEN;

#define LOG2F_TABLE_BITS 4
#define LOG2F_POLY_ORDER 4
extern const struct log2f_data
{
  struct
  {
    double invc, logc;
  } tab[1 << LOG2F_TABLE_BITS];
  double poly[LOG2F_POLY_ORDER];
} __log2f_data HIDDEN;

#define POWF_LOG2_TABLE_BITS 4
#define POWF_LOG2_POLY_ORDER 5
#if TOINT_INTRINSICS
# define POWF_SCALE_BITS EXP2F_TABLE_BITS
#else
# define POWF_SCALE_BITS 0
#endif
#define POWF_SCALE ((double) (1 << POWF_SCALE_BITS))
extern const struct powf_log2_data
{
  struct
  {
    double invc, logc;
  } tab[1 << POWF_LOG2_TABLE_BITS];
  double poly[POWF_LOG2_POLY_ORDER];
} __powf_log2_data HIDDEN;

#define EXP_TABLE_BITS 7
#define EXP_POLY_ORDER 5
/* Use polynomial that is optimized for a wider input range.  This may be
   needed for good precision in non-nearest rounding and !TOINT_INTRINSICS.  */
#define EXP_POLY_WIDE 0
/* Use close to nearest rounding toint when !TOINT_INTRINSICS.  This may be
   needed for good precision in non-nearest rouning and !EXP_POLY_WIDE.  */
#define EXP_USE_TOINT_NARROW 0
#define EXP2_POLY_ORDER 5
#define EXP2_POLY_WIDE 0
extern const struct exp_data
{
  double invln2N;
  double shift;
  double negln2hiN;
  double negln2loN;
  double poly[4]; /* Last four coefficients.  */
  double exp2_shift;
  double exp2_poly[EXP2_POLY_ORDER];
  uint64_t tab[2*(1 << EXP_TABLE_BITS)];
} __exp_data HIDDEN;

#define LOG_TABLE_BITS 7
#define LOG_POLY_ORDER 6
#define LOG_POLY1_ORDER 12
extern const struct log_data
{
  double ln2hi;
  double ln2lo;
  double poly[LOG_POLY_ORDER - 1]; /* First coefficient is 1.  */
  double poly1[LOG_POLY1_ORDER - 1];
  struct {double invc, logc;} tab[1 << LOG_TABLE_BITS];
#if !HAVE_FAST_FMA
  struct {double chi, clo;} tab2[1 << LOG_TABLE_BITS];
#endif
} __log_data HIDDEN;

#define LOG2_TABLE_BITS 6
#define LOG2_POLY_ORDER 7
#define LOG2_POLY1_ORDER 11
extern const struct log2_data
{
  double invln2hi;
  double invln2lo;
  double poly[LOG2_POLY_ORDER - 1];
  double poly1[LOG2_POLY1_ORDER - 1];
  struct {double invc, logc;} tab[1 << LOG2_TABLE_BITS];
#if !HAVE_FAST_FMA
  struct {double chi, clo;} tab2[1 << LOG2_TABLE_BITS];
#endif
} __log2_data HIDDEN;

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8
extern const struct pow_log_data
{
  double ln2hi;
  double ln2lo;
  double poly[POW_LOG_POLY_ORDER - 1]; /* First coefficient is 1.  */
  /* Note: the pad field is unused, but allows slightly faster indexing.  */
  struct {double invc, pad, logc, logctail;} tab[1 << POW_LOG_TABLE_BITS];
} __pow_log_data HIDDEN;

#if WANT_ERRNO
HIDDEN double
__math_with_errno (double y, int e);

HIDDEN float
__math_with_errnof (float y, int e);
#else
#define __math_with_errno(x, e) (x)
#define __math_with_errnof(x, e) (x)
#endif

HIDDEN double
__math_xflow (uint32_t sign, double y);

HIDDEN float
__math_xflowf (uint32_t sign, float y);

#endif

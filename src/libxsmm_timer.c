/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#include <libxsmm_timer.h>
#include <libxsmm_intrinsics_x86.h>
#include "libxsmm_main.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#if defined(_WIN32)
# include <Windows.h>
#elif defined(__GNUC__) || defined(__PGI) || defined(_CRAYC)
# include <sys/time.h>
# include <time.h>
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#if defined(__powerpc64__)
# include <sys/platform/ppc.h>
#endif

#if defined(__powerpc64__)
# define LIBXSMM_TIMER_RDTSC(CYCLE) { \
    CYCLE = __ppc_get_timebase(); \
  }
#elif ((defined(__GNUC__) || defined(LIBXSMM_INTEL_COMPILER) || defined(__PGI)) && (64 <= (LIBXSMM_BITS)))
# define LIBXSMM_TIMER_RDTSC(CYCLE) { libxsmm_timer_tickint libxsmm_timer_rdtsc_hi_; \
    __asm__ __volatile__ ("rdtsc" : "=a"(CYCLE), "=d"(libxsmm_timer_rdtsc_hi_)); \
    CYCLE |= libxsmm_timer_rdtsc_hi_ << 32; \
  }
#elif (defined(_rdtsc) || defined(_WIN32))
# define LIBXSMM_TIMER_RDTSC(CYCLE) (CYCLE = __rdtsc())
#endif


LIBXSMM_APIVAR(int internal_timer_init_rtc);


LIBXSMM_API_INTERN libxsmm_timer_tickint libxsmm_timer_tick_rtc(void)
{
  libxsmm_timer_tickint result;
#if !(defined(__PGI) && defined(__cplusplus))
  int dummy = 0;
#endif
#if defined(_WIN32)
# if 1
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  result = (libxsmm_timer_tickint)t.QuadPart;
# else /* low resolution */
  result = (libxsmm_timer_tickint)GetTickCount();
# endif
#elif defined(CLOCK_MONOTONIC)
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  result = 1000000000ULL * t.tv_sec + t.tv_nsec;
#else
  struct timeval t;
  gettimeofday(&t, 0);
  result = 1000000ULL * t.tv_sec + t.tv_usec;
#endif
#if !(defined(__PGI) && defined(__cplusplus))
  LIBXSMM_UNUSED(dummy);
  dummy =
#endif
  LIBXSMM_ATOMIC_ADD_FETCH(&internal_timer_init_rtc, 1, LIBXSMM_ATOMIC_RELAXED);
  return result;
}


LIBXSMM_API LIBXSMM_INTRINSICS(LIBXSMM_X86_GENERIC)
libxsmm_timer_tickint libxsmm_timer_tick(void)
{
  libxsmm_timer_tickint result;
#if defined(LIBXSMM_TIMER_RDTSC)
  LIBXSMM_TIMER_RDTSC(result);
#else
  result = libxsmm_timer_tick_rtc();
#endif
  return result;
}


LIBXSMM_API double libxsmm_timer_duration(libxsmm_timer_tickint tick0, libxsmm_timer_tickint tick1)
{
  double result = (double)LIBXSMM_DELTA(tick0, tick1);
#if defined(LIBXSMM_TIMER_RDTSC)
# if defined(LIBXSMM_INIT_COMPLETED)
  LIBXSMM_ASSERT_MSG(0 != internal_timer_init_rtc, "LIBXSMM is not initialized");
# else
  if (0 == internal_timer_init_rtc) libxsmm_init();
# endif
  if (0 < libxsmm_timer_scale) {
    result *= libxsmm_timer_scale;
  }
  else
#endif
  {
#if defined(_WIN32)
# if 1
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    result /= (double)frequency.QuadPart;
# else /* low resolution */
    result *= 1E-3;
# endif
#elif defined(CLOCK_MONOTONIC)
    result *= 1E-9;
#else
    result *= 1E-6;
#endif
  }
  return result;
}


#if defined(LIBXSMM_BUILD) && (!defined(LIBXSMM_NOFORTRAN) || defined(__clang_analyzer__))

/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_timer_ncycles)(libxsmm_timer_tickint* /*ncycles*/, const libxsmm_timer_tickint* /*tick0*/, const libxsmm_timer_tickint* /*tick1*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_timer_ncycles)(libxsmm_timer_tickint* ncycles, const libxsmm_timer_tickint* tick0, const libxsmm_timer_tickint* tick1)
{
#if !defined(NDEBUG)
  static int error_once = 0;
  if (NULL != ncycles && NULL != tick0 && NULL != tick1)
#endif
  {
    *ncycles = libxsmm_timer_ncycles(*tick0, *tick1);
  }
#if !defined(NDEBUG)
  else if (0 != libxsmm_verbosity /* library code is expected to be mute */
    && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
  {
    fprintf(stderr, "LIBXSMM ERROR: invalid arguments for libxsmm_timer_ncycles specified!\n");
  }
#endif
}

#endif /*defined(LIBXSMM_BUILD) && (!defined(LIBXSMM_NOFORTRAN) || defined(__clang_analyzer__))*/


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
#include <libxsmm.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


int main(int argc, char* argv[])
{
  const int insize = (1 < argc ? atoi(argv[1]) : 0);
  const int incrmt = (2 < argc ? atoi(argv[2]) : 0);
  const int nelems = (3 < argc ? atoi(argv[3]) : 0);
  const int niters = (4 < argc ? atoi(argv[4]) : 1);
  const int elsize = (0 >= insize ? LIBXSMM_DESCRIPTOR_SIGSIZE : insize);
  const int stride = (0 >= incrmt ? LIBXSMM_MAX(LIBXSMM_DESCRIPTOR_MAXSIZE, elsize) : LIBXSMM_MAX(incrmt, elsize));
  const size_t n = (0 >= nelems ? (((size_t)2 << 30/*2 GB*/) / stride) : ((size_t)nelems));
  int result = EXIT_SUCCESS;
  size_t nbytes, size, nrpt;
  unsigned char *a, *b;
  double duration;

  LIBXSMM_ASSERT(elsize <= stride);
  if (0 < niters) {
    size = n;
    nrpt = niters;
  }
  else {
    size = LIBXSMM_MAX(LIBXSMM_ABS(niters), 1);
    nrpt = n;
  }
  nbytes = size * stride;
  a = (unsigned char*)(0 != nbytes ? malloc(nbytes) : NULL);
  b = (unsigned char*)(0 != nbytes ? malloc(nbytes) : NULL);

  if (NULL != a && NULL != b) {
    /* initialize the data */
    libxsmm_rng_seq(a, (libxsmm_blasint)nbytes);
    libxsmm_rng_seq(b, (libxsmm_blasint)nbytes);

    { /* benchmark libxsmm_hash/pure */
      size_t diff = 0, i;
      const libxsmm_timer_tickint start = libxsmm_timer_tick();
      for (i = 0; i < nrpt; ++i) {
        const unsigned int hash_a = libxsmm_hash(a, (unsigned int)nbytes, 0/*seed*/);
        const unsigned int hash_b = libxsmm_hash(b, (unsigned int)nbytes, 0/*seed*/);
        diff += (hash_a != hash_b);
      }
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
      if (0 < duration) printf("libxsmm_hash/pure:\t%.8f s (%i MB/s)\n", duration,
        (int)LIBXSMM_ROUND((2.0 * nrpt * nbytes) / ((1024.0 * 1024.0) * duration)));
      result += (int)diff * ((int)stride / ((int)stride + 1)); /* ignore result */
    }

    { /* benchmark libxsmm_hash/cmp */
      size_t diff = 0, i, j;
      const libxsmm_timer_tickint start = libxsmm_timer_tick();
      for (i = 0; i < nrpt; ++i) {
        for (j = 0; j < nbytes; j += stride) {
          const void *const aj = a + j, *const bj = b + j;
          const unsigned int hash_a = libxsmm_hash(aj, elsize, 0/*seed*/);
          const unsigned int hash_b = libxsmm_hash(bj, elsize, 0/*seed*/);
          diff += (hash_a != hash_b || libxsmm_diff(aj, bj, (unsigned char)elsize));
        }
      }
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
      if (0 < duration) printf("libxsmm_hash/cmp:\t%.8f s (%i MB/s)\n", duration,
        (int)LIBXSMM_ROUND((2.0 * nrpt * nbytes) / ((1024.0 * 1024.0) * duration)));
      result += (int)diff * ((int)stride / ((int)stride + 1)); /* ignore result */
    }

    { /* benchmark libxsmm_memcmp */
      size_t diff = 0, i, j;
      const libxsmm_timer_tickint start = libxsmm_timer_tick();
      for (i = 0; i < nrpt; ++i) {
        for (j = 0; j < nbytes; j += stride) {
          const void *const aj = a + j, *const bj = b + j;
          diff += libxsmm_memcmp(aj, bj, elsize);
        }
      }
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
      if (0 < duration) printf("libxsmm_memcmp:\t\t%.8f s (%i MB/s)\n", duration,
        (int)LIBXSMM_ROUND((2.0 * nrpt * nbytes) / ((1024.0 * 1024.0) * duration)));
      result += (int)diff * ((int)stride / ((int)stride + 1)); /* ignore result */
    }

    { /* benchmark stdlib's memcmp */
      size_t diff = 0, i, j;
      const libxsmm_timer_tickint start = libxsmm_timer_tick();
      for (i = 0; i < nrpt; ++i) {
        for (j = 0; j < nbytes; j += stride) {
          const void *const aj = a + j, *const bj = b + j;
#if defined(_MSC_VER)
#         pragma warning(push)
#         pragma warning(disable: 6385)
#endif
          diff += (0 != memcmp(aj, bj, elsize));
#if defined(_MSC_VER)
#         pragma warning(pop)
#endif
        }
        /* memcmp is likely pure and without touching a it is not repeated (nrpt) */
        a[i%nbytes] = 255;
      }
      duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
      if (0 < duration) printf("stdlib memcmp:\t\t%.8f s (%i MB/s)\n", duration,
        (int)LIBXSMM_ROUND((2.0 * nrpt * nbytes) / ((1024.0 * 1024.0) * duration)));
      result += (int)diff * ((int)stride / ((int)stride + 1)); /* ignore result */
    }
  }
  else {
    result = EXIT_FAILURE;
  }

  free(a);
  free(b);

  return result;
}


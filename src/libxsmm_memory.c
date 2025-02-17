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
#include <libxsmm_memory.h>
#include "libxsmm_hash.h"
#include "libxsmm_diff.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <string.h>
#include <stdio.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#if !defined(LIBXSMM_DIFF_MEMCMP) && 0
# define LIBXSMM_DIFF_MEMCMP
#endif


LIBXSMM_API unsigned char libxsmm_diff_16(const void* a, const void* b, ...)
{
  LIBXSMM_DIFF_16_DECL(a16);
  LIBXSMM_DIFF_16_LOAD(a16, a);
  return LIBXSMM_DIFF_16(a16, b, 0/*dummy*/);
}


LIBXSMM_API unsigned char libxsmm_diff_32(const void* a, const void* b, ...)
{
  LIBXSMM_DIFF_32_DECL(a32);
  LIBXSMM_DIFF_32_LOAD(a32, a);
  return LIBXSMM_DIFF_32(a32, b, 0/*dummy*/);
}


LIBXSMM_API unsigned char libxsmm_diff_48(const void* a, const void* b, ...)
{
  LIBXSMM_DIFF_48_DECL(a48);
  LIBXSMM_DIFF_48_LOAD(a48, a);
  return LIBXSMM_DIFF_48(a48, b, 0/*dummy*/);
}


LIBXSMM_API unsigned char libxsmm_diff_64(const void* a, const void* b, ...)
{
  LIBXSMM_DIFF_64_DECL(a64);
  LIBXSMM_DIFF_64_LOAD(a64, a);
  return LIBXSMM_DIFF_64(a64, b, 0/*dummy*/);
}


LIBXSMM_API unsigned char libxsmm_diff(const void* a, const void* b, unsigned char size)
{
  const uint8_t *const a8 = (const uint8_t*)a, *const b8 = (const uint8_t*)b;
  unsigned char i;
  for (i = 0; i < (size & 0xF0); i += 16) {
    LIBXSMM_DIFF_16_DECL(a16);
    LIBXSMM_DIFF_16_LOAD(a16, a8 + i);
    if (LIBXSMM_DIFF_16(a16, b8 + i, 0/*dummy*/)) return 1;
  }
  for (; i < size; ++i) if (a8[i] ^ b8[i]) return 1;
  return 0;
}


LIBXSMM_API unsigned int libxsmm_diff_n(const void* a, const void* bn, unsigned char size,
  unsigned char stride, unsigned int hint, unsigned int n)
{
  unsigned int result;
  LIBXSMM_ASSERT(size <= stride);
#if defined(LIBXSMM_DIFF_MEMCMP)
  LIBXSMM_DIFF_N(unsigned int, result, memcmp, a, bn, size, stride, hint, n);
#else
  switch (size) {
    case 64: {
      LIBXSMM_DIFF_64_DECL(a64);
      LIBXSMM_DIFF_64_LOAD(a64, a);
      LIBXSMM_DIFF_N(unsigned int, result, LIBXSMM_DIFF_64, a64, bn, size, stride, hint, n);
    } break;
    case 48: {
      LIBXSMM_DIFF_48_DECL(a48);
      LIBXSMM_DIFF_48_LOAD(a48, a);
      LIBXSMM_DIFF_N(unsigned int, result, LIBXSMM_DIFF_48, a48, bn, size, stride, hint, n);
    } break;
    case 32: {
      LIBXSMM_DIFF_32_DECL(a32);
      LIBXSMM_DIFF_32_LOAD(a32, a);
      LIBXSMM_DIFF_N(unsigned int, result, LIBXSMM_DIFF_32, a32, bn, size, stride, hint, n);
    } break;
    case 16: {
      LIBXSMM_DIFF_16_DECL(a16);
      LIBXSMM_DIFF_16_LOAD(a16, a);
      LIBXSMM_DIFF_N(unsigned int, result, LIBXSMM_DIFF_16, a16, bn, size, stride, hint, n);
    } break;
    default: {
      LIBXSMM_DIFF_N(unsigned int, result, libxsmm_diff, a, bn, size, stride, hint, n);
    }
  }
#endif
  return result;
}


LIBXSMM_API int libxsmm_memcmp(const void* a, const void* b, size_t size)
{
#if defined(LIBXSMM_DIFF_MEMCMP)
  return memcmp(a, b, size);
#else
  const uint8_t *const a8 = (const uint8_t*)a, *const b8 = (const uint8_t*)b;
  LIBXSMM_DIFF_32_DECL(aa);
  size_t i;
  for (i = 0; i < (size & 0xFFFFFFFFFFFFFFE0); i += 32) {
    LIBXSMM_DIFF_32_LOAD(aa, a8 + i);
    if (LIBXSMM_DIFF_32(aa, b8 + i, 0/*dummy*/)) return 1;
  }
  for (; i < size; ++i) if (a8[i] ^ b8[i]) return 1;
  return 0;
#endif
}


LIBXSMM_API unsigned int libxsmm_hash(const void* data, unsigned int size, unsigned int seed)
{
  LIBXSMM_INIT
  return libxsmm_crc32(seed, data, size);
}


LIBXSMM_API unsigned long long libxsmm_hash_string(const char* string)
{
  unsigned long long result;
  const size_t length = NULL != string ? strlen(string) : 0;
  if (sizeof(result) < length) {
    const size_t length2 = length / 2;
    unsigned int seed32 = 0; /* seed=0: match else-optimization */
    LIBXSMM_INIT
    seed32 = libxsmm_crc32(seed32, string, length2);
    result = libxsmm_crc32(seed32, string + length2, length - length2);
    result = (result << 32) | seed32;
  }
  else { /* reinterpret directly as hash value */
    char *const s = (char*)&result; signed char i;
    for (i = 0; i < (signed char)length; ++i) s[i] = string[i];
    for (; i < (signed char)sizeof(result); ++i) s[i] = 0;
  }
  return result;
}


#if defined(LIBXSMM_BUILD) && (!defined(LIBXSMM_NOFORTRAN) || defined(__clang_analyzer__))

/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash)(void* /*hash_seed*/, const void* /*data*/, const int* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash)(void* hash_seed, const void* data, const int* size)
{
#if !defined(NDEBUG)
  static int error_once = 0;
  if (NULL != hash_seed && NULL != data && NULL != size && 0 <= *size)
#endif
  {
    unsigned int *const hash_seed_ui32 = (unsigned int*)hash_seed;
    *hash_seed_ui32 = (libxsmm_hash(data, (unsigned int)*size, *hash_seed_ui32) & 0x7FFFFFFF/*sign-bit*/);
  }
#if !defined(NDEBUG)
  else if (0 != libxsmm_verbosity /* library code is expected to be mute */
    && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
  {
    fprintf(stderr, "LIBXSMM ERROR: invalid arguments for libxsmm_hash specified!\n");
  }
#endif
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_char)(void* /*hash_seed*/, const void* /*data*/, const int* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_char)(void* hash_seed, const void* data, const int* size)
{
  LIBXSMM_FSYMBOL(libxsmm_hash)(hash_seed, data, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i8)(void* /*hash_seed*/, const void* /*data*/, const int* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i8)(void* hash_seed, const void* data, const int* size)
{
  LIBXSMM_FSYMBOL(libxsmm_hash)(hash_seed, data, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i32)(void* /*hash_seed*/, const void* /*data*/, const int* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i32)(void* hash_seed, const void* data, const int* size)
{
  LIBXSMM_FSYMBOL(libxsmm_hash)(hash_seed, data, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i64)(void* /*hash_seed*/, const void* /*data*/, const int* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_hash_i64)(void* hash_seed, const void* data, const int* size)
{
  LIBXSMM_FSYMBOL(libxsmm_hash)(hash_seed, data, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff)(int* /*result*/, const void* /*a*/, const void* /*b*/, const long long* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff)(int* result, const void* a, const void* b, const long long* size)
{
#if !defined(NDEBUG)
  static int error_once = 0;
  if (NULL != result && NULL != a && NULL != b && NULL != size && 0 <= *size)
#endif
  {
    *result = libxsmm_memcmp(a, b, (size_t)*size);
  }
#if !defined(NDEBUG)
  else if (0 != libxsmm_verbosity /* library code is expected to be mute */
    && 1 == LIBXSMM_ATOMIC_ADD_FETCH(&error_once, 1, LIBXSMM_ATOMIC_RELAXED))
  {
    fprintf(stderr, "LIBXSMM ERROR: invalid arguments for libxsmm_memcmp specified!\n");
  }
#endif
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_char)(int* /*result*/, const void* /*a*/, const void* /*b*/, const long long* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_char)(int* result, const void* a, const void* b, const long long* size)
{
  LIBXSMM_FSYMBOL(libxsmm_diff)(result, a, b, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i8)(int* /*result*/, const void* /*a*/, const void* /*b*/, const long long* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i8)(int* result, const void* a, const void* b, const long long* size)
{
  LIBXSMM_FSYMBOL(libxsmm_diff)(result, a, b, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i32)(int* /*result*/, const void* /*a*/, const void* /*b*/, const long long* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i32)(int* result, const void* a, const void* b, const long long* size)
{
  LIBXSMM_FSYMBOL(libxsmm_diff)(result, a, b, size);
}


/* implementation provided for Fortran 77 compatibility */
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i64)(int* /*result*/, const void* /*a*/, const void* /*b*/, const long long* /*size*/);
LIBXSMM_API void LIBXSMM_FSYMBOL(libxsmm_diff_i64)(int* result, const void* a, const void* b, const long long* size)
{
  LIBXSMM_FSYMBOL(libxsmm_diff)(result, a, b, size);
}

#endif /*defined(LIBXSMM_BUILD) && (!defined(LIBXSMM_NOFORTRAN) || defined(__clang_analyzer__))*/


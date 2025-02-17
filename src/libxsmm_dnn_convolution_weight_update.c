/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Rajkishore Barik, Alexander Heinecke, Ankush Mandal, Jason Sewall (Intel Corp.)
******************************************************************************/
#include "libxsmm_dnn_convolution_weight_update.h"
#include <libxsmm_intrinsics_x86.h>
#include "libxsmm_main.h"
#include <libxsmm.h>
#include <stdio.h>

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <string.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif


/* function prototypes for below implementations */
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16_emu(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_rsck_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);

LIBXSMM_API_INLINE LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
void transpose_32x16(libxsmm_bfloat16 *in, libxsmm_bfloat16 *out, int ld_in, int ld_out)
{
#if defined(LIBXSMM_INTRINSICS_AVX512_CORE)
  __m512i r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, ra, rb, rc, rd, re, rf;
  __m512i t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, ta, tb, tc, td, te, tf;
  const int in_width=ld_in, out_width=ld_out;
  const __m512i idx_lo         = _mm512_set_epi64(13, 12, 5, 4, 9, 8, 1, 0);
  const __m512i idx_hi         = _mm512_set_epi64(7, 6, 15, 14, 3, 2, 11, 10);

  r0 = _mm512_load_epi32(in + 0*in_width);
  r1 = _mm512_load_epi32(in + 1*in_width);
  r2 = _mm512_load_epi32(in + 2*in_width);
  r3 = _mm512_load_epi32(in + 3*in_width);
  r4 = _mm512_load_epi32(in + 4*in_width);
  r5 = _mm512_load_epi32(in + 5*in_width);
  r6 = _mm512_load_epi32(in + 6*in_width);
  r7 = _mm512_load_epi32(in + 7*in_width);
  r8 = _mm512_load_epi32(in + 8*in_width);
  r9 = _mm512_load_epi32(in + 9*in_width);
  ra = _mm512_load_epi32(in + 10*in_width);
  rb = _mm512_load_epi32(in + 11*in_width);
  rc = _mm512_load_epi32(in + 12*in_width);
  rd = _mm512_load_epi32(in + 13*in_width);
  re = _mm512_load_epi32(in + 14*in_width);
  rf = _mm512_load_epi32(in + 15*in_width);

  t0 = _mm512_unpacklo_epi16(r0,r1);
  t1 = _mm512_unpackhi_epi16(r0,r1);
  t2 = _mm512_unpacklo_epi16(r2,r3);
  t3 = _mm512_unpackhi_epi16(r2,r3);
  t4 = _mm512_unpacklo_epi16(r4,r5);
  t5 = _mm512_unpackhi_epi16(r4,r5);
  t6 = _mm512_unpacklo_epi16(r6,r7);
  t7 = _mm512_unpackhi_epi16(r6,r7);
  t8 = _mm512_unpacklo_epi16(r8,r9);
  t9 = _mm512_unpackhi_epi16(r8,r9);
  ta = _mm512_unpacklo_epi16(ra,rb);
  tb = _mm512_unpackhi_epi16(ra,rb);
  tc = _mm512_unpacklo_epi16(rc,rd);
  td = _mm512_unpackhi_epi16(rc,rd);
  te = _mm512_unpacklo_epi16(re,rf);
  tf = _mm512_unpackhi_epi16(re,rf);

  r0 = _mm512_unpacklo_epi32(t0,t2);
  r1 = _mm512_unpackhi_epi32(t0,t2);
  r2 = _mm512_unpacklo_epi32(t1,t3);
  r3 = _mm512_unpackhi_epi32(t1,t3);
  r4 = _mm512_unpacklo_epi32(t4,t6);
  r5 = _mm512_unpackhi_epi32(t4,t6);
  r6 = _mm512_unpacklo_epi32(t5,t7);
  r7 = _mm512_unpackhi_epi32(t5,t7);
  r8 = _mm512_unpacklo_epi32(t8,ta);
  r9 = _mm512_unpackhi_epi32(t8,ta);
  ra = _mm512_unpacklo_epi32(t9,tb);
  rb = _mm512_unpackhi_epi32(t9,tb);
  rc = _mm512_unpacklo_epi32(tc,te);
  rd = _mm512_unpackhi_epi32(tc,te);
  re = _mm512_unpacklo_epi32(td,tf);
  rf = _mm512_unpackhi_epi32(td,tf);

  t0 = _mm512_unpacklo_epi64(r0,r4);
  t1 = _mm512_unpackhi_epi64(r0,r4);
  t2 = _mm512_unpacklo_epi64(r1,r5);
  t3 = _mm512_unpackhi_epi64(r1,r5);
  t4 = _mm512_unpacklo_epi64(r2,r6);
  t5 = _mm512_unpackhi_epi64(r2,r6);
  t6 = _mm512_unpacklo_epi64(r3,r7);
  t7 = _mm512_unpackhi_epi64(r3,r7);
  t8 = _mm512_unpacklo_epi64(r8,rc);
  t9 = _mm512_unpackhi_epi64(r8,rc);
  ta = _mm512_unpacklo_epi64(r9,rd);
  tb = _mm512_unpackhi_epi64(r9,rd);
  tc = _mm512_unpacklo_epi64(ra,re);
  td = _mm512_unpackhi_epi64(ra,re);
  te = _mm512_unpacklo_epi64(rb,rf);
  tf = _mm512_unpackhi_epi64(rb,rf);

  r0 = _mm512_shuffle_i32x4(t0, t1, 0x88);
  r1 = _mm512_shuffle_i32x4(t2, t3, 0x88);
  r2 = _mm512_shuffle_i32x4(t4, t5, 0x88);
  r3 = _mm512_shuffle_i32x4(t6, t7, 0x88);
  r4 = _mm512_shuffle_i32x4(t0, t1, 0xdd);
  r5 = _mm512_shuffle_i32x4(t2, t3, 0xdd);
  r6 = _mm512_shuffle_i32x4(t4, t5, 0xdd);
  r7 = _mm512_shuffle_i32x4(t6, t7, 0xdd);
  r8 = _mm512_shuffle_i32x4(t8, t9, 0x88);
  r9 = _mm512_shuffle_i32x4(ta, tb, 0x88);
  ra = _mm512_shuffle_i32x4(tc, td, 0x88);
  rb = _mm512_shuffle_i32x4(te, tf, 0x88);
  rc = _mm512_shuffle_i32x4(t8, t9, 0xdd);
  rd = _mm512_shuffle_i32x4(ta, tb, 0xdd);
  re = _mm512_shuffle_i32x4(tc, td, 0xdd);
  rf = _mm512_shuffle_i32x4(te, tf, 0xdd);

  t0 = _mm512_permutex2var_epi64(r0, idx_lo, r8);
  t1 = _mm512_permutex2var_epi64(r1, idx_lo, r9);
  t2 = _mm512_permutex2var_epi64(r2, idx_lo, ra);
  t3 = _mm512_permutex2var_epi64(r3, idx_lo, rb);
  t4 = _mm512_permutex2var_epi64(r4, idx_lo, rc);
  t5 = _mm512_permutex2var_epi64(r5, idx_lo, rd);
  t6 = _mm512_permutex2var_epi64(r6, idx_lo, re);
  t7 = _mm512_permutex2var_epi64(r7, idx_lo, rf);
  t8 = _mm512_permutex2var_epi64(r8, idx_hi, r0);
  t9 = _mm512_permutex2var_epi64(r9, idx_hi, r1);
  ta = _mm512_permutex2var_epi64(ra, idx_hi, r2);
  tb = _mm512_permutex2var_epi64(rb, idx_hi, r3);
  tc = _mm512_permutex2var_epi64(rc, idx_hi, r4);
  td = _mm512_permutex2var_epi64(rd, idx_hi, r5);
  te = _mm512_permutex2var_epi64(re, idx_hi, r6);
  tf = _mm512_permutex2var_epi64(rf, idx_hi, r7);

  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 0*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t0, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 1*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t0, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 2*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t1, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 3*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t1, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 4*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t2, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 5*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t2, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 6*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t3, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 7*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t3, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 8*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t4, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 9*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t4, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 10*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t5, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 11*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t5, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 12*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t6, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 13*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t6, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 14*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t7, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 15*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t7, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 16*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t8, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 17*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t8, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 18*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t9, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 19*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t9, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 20*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(ta, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 21*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(ta, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 22*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tb, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 23*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tb, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 24*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tc, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 25*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tc, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 26*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(td, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 27*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(td, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 28*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(te, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 29*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(te, 1));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 30*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tf, 0));
  LIBXSMM_INTRINSICS_MM256_STORE_EPI32(out + 31*out_width, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tf, 1));
#else
  LIBXSMM_UNUSED(in); LIBXSMM_UNUSED(out); LIBXSMM_UNUSED(ld_in); LIBXSMM_UNUSED(ld_out);
#endif
}

LIBXSMM_API_INLINE LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
void transpose_32xcols(libxsmm_bfloat16 *in, libxsmm_bfloat16 *out, int col, int ld_in, int ld_out)
{
#if defined(LIBXSMM_INTRINSICS_AVX512_CORE)
  __m512i r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, ra, rb, rc, rd, re, rf;
  __m512i t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, ta, tb, tc, td, te, tf;
  const int in_width=ld_in, out_width=ld_out;
  const __m512i idx_lo         = _mm512_set_epi64(13, 12, 5, 4, 9, 8, 1, 0);
  const __m512i idx_hi         = _mm512_set_epi64(7, 6, 15, 14, 3, 2, 11, 10);
  __mmask16 store_mask         = LIBXSMM_INTRINSICS_MM512_CVTU32_MASK16(((unsigned int)1 << col) - 1);

  rf = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
  if (col == 15) {
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
    ra = _mm512_load_epi32(in + 10*in_width);
    rb = _mm512_load_epi32(in + 11*in_width);
    rc = _mm512_load_epi32(in + 12*in_width);
    rd = _mm512_load_epi32(in + 13*in_width);
    re = _mm512_load_epi32(in + 14*in_width);
  } else if (col == 14) {
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
    ra = _mm512_load_epi32(in + 10*in_width);
    rb = _mm512_load_epi32(in + 11*in_width);
    rc = _mm512_load_epi32(in + 12*in_width);
    rd = _mm512_load_epi32(in + 13*in_width);
  } else if (col == 13) {
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
    ra = _mm512_load_epi32(in + 10*in_width);
    rb = _mm512_load_epi32(in + 11*in_width);
    rc = _mm512_load_epi32(in + 12*in_width);
  } else if (col == 12) {
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
    ra = _mm512_load_epi32(in + 10*in_width);
    rb = _mm512_load_epi32(in + 11*in_width);
  } else if (col == 11) {
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
    ra = _mm512_load_epi32(in + 10*in_width);
  } else if (col == 10) {
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
    r9 = _mm512_load_epi32(in + 9*in_width);
  } else if (col == 9) {
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
    r8 = _mm512_load_epi32(in + 8*in_width);
  } else if (col == 8) {
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
    r7 = _mm512_load_epi32(in + 7*in_width);
  } else if (col == 7) {
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
    r6 = _mm512_load_epi32(in + 6*in_width);
  } else if (col == 6) {
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
    r5 = _mm512_load_epi32(in + 5*in_width);
  } else if (col == 5) {
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
    r4 = _mm512_load_epi32(in + 4*in_width);
  } else if (col == 4) {
    r4 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
    r3 = _mm512_load_epi32(in + 3*in_width);
  } else if (col == 3) {
    r3 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r4 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
    r2 = _mm512_load_epi32(in + 2*in_width);
  } else if (col == 2) {
    r2 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r3 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r4 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
    r1 = _mm512_load_epi32(in + 1*in_width);
  } else if (col == 1) {
    r1 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r2 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r3 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r4 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r0 = _mm512_load_epi32(in + 0*in_width);
  } else {
    r0 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r1 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r2 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r3 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r4 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r5 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r6 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r7 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r8 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    r9 = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    ra = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rb = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rc = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    rd = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
    re = LIBXSMM_INTRINSICS_MM512_UNDEFINED_EPI32();
  }

  t0 = _mm512_unpacklo_epi16(r0,r1);
  t1 = _mm512_unpackhi_epi16(r0,r1);
  t2 = _mm512_unpacklo_epi16(r2,r3);
  t3 = _mm512_unpackhi_epi16(r2,r3);
  t4 = _mm512_unpacklo_epi16(r4,r5);
  t5 = _mm512_unpackhi_epi16(r4,r5);
  t6 = _mm512_unpacklo_epi16(r6,r7);
  t7 = _mm512_unpackhi_epi16(r6,r7);
  t8 = _mm512_unpacklo_epi16(r8,r9);
  t9 = _mm512_unpackhi_epi16(r8,r9);
  ta = _mm512_unpacklo_epi16(ra,rb);
  tb = _mm512_unpackhi_epi16(ra,rb);
  tc = _mm512_unpacklo_epi16(rc,rd);
  td = _mm512_unpackhi_epi16(rc,rd);
  te = _mm512_unpacklo_epi16(re,rf);
  tf = _mm512_unpackhi_epi16(re,rf);

  r0 = _mm512_unpacklo_epi32(t0,t2);
  r1 = _mm512_unpackhi_epi32(t0,t2);
  r2 = _mm512_unpacklo_epi32(t1,t3);
  r3 = _mm512_unpackhi_epi32(t1,t3);
  r4 = _mm512_unpacklo_epi32(t4,t6);
  r5 = _mm512_unpackhi_epi32(t4,t6);
  r6 = _mm512_unpacklo_epi32(t5,t7);
  r7 = _mm512_unpackhi_epi32(t5,t7);
  r8 = _mm512_unpacklo_epi32(t8,ta);
  r9 = _mm512_unpackhi_epi32(t8,ta);
  ra = _mm512_unpacklo_epi32(t9,tb);
  rb = _mm512_unpackhi_epi32(t9,tb);
  rc = _mm512_unpacklo_epi32(tc,te);
  rd = _mm512_unpackhi_epi32(tc,te);
  re = _mm512_unpacklo_epi32(td,tf);
  rf = _mm512_unpackhi_epi32(td,tf);

  t0 = _mm512_unpacklo_epi64(r0,r4);
  t1 = _mm512_unpackhi_epi64(r0,r4);
  t2 = _mm512_unpacklo_epi64(r1,r5);
  t3 = _mm512_unpackhi_epi64(r1,r5);
  t4 = _mm512_unpacklo_epi64(r2,r6);
  t5 = _mm512_unpackhi_epi64(r2,r6);
  t6 = _mm512_unpacklo_epi64(r3,r7);
  t7 = _mm512_unpackhi_epi64(r3,r7);
  t8 = _mm512_unpacklo_epi64(r8,rc);
  t9 = _mm512_unpackhi_epi64(r8,rc);
  ta = _mm512_unpacklo_epi64(r9,rd);
  tb = _mm512_unpackhi_epi64(r9,rd);
  tc = _mm512_unpacklo_epi64(ra,re);
  td = _mm512_unpackhi_epi64(ra,re);
  te = _mm512_unpacklo_epi64(rb,rf);
  tf = _mm512_unpackhi_epi64(rb,rf);

  r0 = _mm512_shuffle_i32x4(t0, t1, 0x88);
  r1 = _mm512_shuffle_i32x4(t2, t3, 0x88);
  r2 = _mm512_shuffle_i32x4(t4, t5, 0x88);
  r3 = _mm512_shuffle_i32x4(t6, t7, 0x88);
  r4 = _mm512_shuffle_i32x4(t0, t1, 0xdd);
  r5 = _mm512_shuffle_i32x4(t2, t3, 0xdd);
  r6 = _mm512_shuffle_i32x4(t4, t5, 0xdd);
  r7 = _mm512_shuffle_i32x4(t6, t7, 0xdd);
  r8 = _mm512_shuffle_i32x4(t8, t9, 0x88);
  r9 = _mm512_shuffle_i32x4(ta, tb, 0x88);
  ra = _mm512_shuffle_i32x4(tc, td, 0x88);
  rb = _mm512_shuffle_i32x4(te, tf, 0x88);
  rc = _mm512_shuffle_i32x4(t8, t9, 0xdd);
  rd = _mm512_shuffle_i32x4(ta, tb, 0xdd);
  re = _mm512_shuffle_i32x4(tc, td, 0xdd);
  rf = _mm512_shuffle_i32x4(te, tf, 0xdd);

  t0 = _mm512_permutex2var_epi64(r0, idx_lo, r8);
  t1 = _mm512_permutex2var_epi64(r1, idx_lo, r9);
  t2 = _mm512_permutex2var_epi64(r2, idx_lo, ra);
  t3 = _mm512_permutex2var_epi64(r3, idx_lo, rb);
  t4 = _mm512_permutex2var_epi64(r4, idx_lo, rc);
  t5 = _mm512_permutex2var_epi64(r5, idx_lo, rd);
  t6 = _mm512_permutex2var_epi64(r6, idx_lo, re);
  t7 = _mm512_permutex2var_epi64(r7, idx_lo, rf);
  t8 = _mm512_permutex2var_epi64(r8, idx_hi, r0);
  t9 = _mm512_permutex2var_epi64(r9, idx_hi, r1);
  ta = _mm512_permutex2var_epi64(ra, idx_hi, r2);
  tb = _mm512_permutex2var_epi64(rb, idx_hi, r3);
  tc = _mm512_permutex2var_epi64(rc, idx_hi, r4);
  td = _mm512_permutex2var_epi64(rd, idx_hi, r5);
  te = _mm512_permutex2var_epi64(re, idx_hi, r6);
  tf = _mm512_permutex2var_epi64(rf, idx_hi, r7);

  _mm256_mask_storeu_epi16(out + 0*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t0, 0));
  _mm256_mask_storeu_epi16(out + 1*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t0, 1));
  _mm256_mask_storeu_epi16(out + 2*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t1, 0));
  _mm256_mask_storeu_epi16(out + 3*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t1, 1));
  _mm256_mask_storeu_epi16(out + 4*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t2, 0));
  _mm256_mask_storeu_epi16(out + 5*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t2, 1));
  _mm256_mask_storeu_epi16(out + 6*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t3, 0));
  _mm256_mask_storeu_epi16(out + 7*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t3, 1));
  _mm256_mask_storeu_epi16(out + 8*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t4, 0));
  _mm256_mask_storeu_epi16(out + 9*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t4, 1));
  _mm256_mask_storeu_epi16(out + 10*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t5, 0));
  _mm256_mask_storeu_epi16(out + 11*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t5, 1));
  _mm256_mask_storeu_epi16(out + 12*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t6, 0));
  _mm256_mask_storeu_epi16(out + 13*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t6, 1));
  _mm256_mask_storeu_epi16(out + 14*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t7, 0));
  _mm256_mask_storeu_epi16(out + 15*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t7, 1));
  _mm256_mask_storeu_epi16(out + 16*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t8, 0));
  _mm256_mask_storeu_epi16(out + 17*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t8, 1));
  _mm256_mask_storeu_epi16(out + 18*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t9, 0));
  _mm256_mask_storeu_epi16(out + 19*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(t9, 1));
  _mm256_mask_storeu_epi16(out + 20*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(ta, 0));
  _mm256_mask_storeu_epi16(out + 21*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(ta, 1));
  _mm256_mask_storeu_epi16(out + 22*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tb, 0));
  _mm256_mask_storeu_epi16(out + 23*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tb, 1));
  _mm256_mask_storeu_epi16(out + 24*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tc, 0));
  _mm256_mask_storeu_epi16(out + 25*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tc, 1));
  _mm256_mask_storeu_epi16(out + 26*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(td, 0));
  _mm256_mask_storeu_epi16(out + 27*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(td, 1));
  _mm256_mask_storeu_epi16(out + 28*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(te, 0));
  _mm256_mask_storeu_epi16(out + 29*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(te, 1));
  _mm256_mask_storeu_epi16(out + 30*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tf, 0));
  _mm256_mask_storeu_epi16(out + 31*out_width, store_mask, LIBXSMM_INTRINSICS_MM512_EXTRACTI64X4_EPI64(tf, 1));
#else
  LIBXSMM_UNUSED(in); LIBXSMM_UNUSED(out); LIBXSMM_UNUSED(col); LIBXSMM_UNUSED(ld_in); LIBXSMM_UNUSED(ld_out);
#endif
}

LIBXSMM_API_INLINE LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
void transpose_input_pixels_bf16(libxsmm_bfloat16 *in, libxsmm_bfloat16 *out, int M, int N, int ld_in, int ld_out){
#if defined(LIBXSMM_INTRINSICS_AVX512_CORE)
  int i, j;
  int full16_chunks = N/16;
  int remainder_cols = N%16;
  int _N = N - remainder_cols;

  if (full16_chunks) {
    for (i=0; i<M; i+=32) {
      for (j=0; j<_N; j+=16) {
        transpose_32x16((libxsmm_bfloat16*)in + i + ld_in*j, (libxsmm_bfloat16*)out + j + i*ld_out, ld_in, ld_out);
      }
    }
  }

  if (remainder_cols) {
    for (i=0; i<M; i+=32) {
      transpose_32xcols((libxsmm_bfloat16*)in + i + ld_in*full16_chunks*16, (libxsmm_bfloat16*)out + full16_chunks*16 + i*ld_out, remainder_cols, ld_in, ld_out);
    }
  }
#else
  LIBXSMM_UNUSED(in); LIBXSMM_UNUSED(out); LIBXSMM_UNUSED(M); LIBXSMM_UNUSED(N); LIBXSMM_UNUSED(ld_in); LIBXSMM_UNUSED(ld_out);
#endif
}

LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction gemm_function;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
# include "template/libxsmm_dnn_convolve_st_upd_custom_custom_generic.tpl.c"
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16_emu(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512_CORE) /*__AVX512F__,__AVX512BW__,__AVX512DQ__*/
  typedef libxsmm_bfloat16 element_input_type;
  typedef libxsmm_bfloat16 element_output_type;
  typedef libxsmm_bfloat16 element_filter_type;
  typedef libxsmm_bsmmfunction gemm_function;
  typedef libxsmm_bsmmfunction_reducebatch_addr gemm_br_function;
# include "template/libxsmm_dnn_convolve_st_upd_custom_custom_generic_bf16.tpl.c"
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


#if defined(LIBXSMM_INTRINSICS_AVX512_CPX)
LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CPX)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512_CPX) /*__AVX512F__,__AVX512BW__,__AVX512DQ__,__AVX512BF16__*/
  typedef libxsmm_bfloat16 element_input_type;
  typedef libxsmm_bfloat16 element_output_type;
  typedef libxsmm_bfloat16 element_filter_type;
  typedef libxsmm_bsmmfunction gemm_function;
  typedef libxsmm_bsmmfunction_reducebatch_addr gemm_br_function;
#define LIBXSMM_DNN_CONVOLUTION_UPD_AVX512_CPX
# include "template/libxsmm_dnn_convolve_st_upd_custom_custom_generic_bf16.tpl.c"
#undef LIBXSMM_DNN_CONVOLUTION_UPD_AVX512_CPX
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}
#else
LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  return libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16_emu( handle, start_thread, tid );
}
#endif


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction gemm_function;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
#define LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_CUSTOM
# include "template/libxsmm_dnn_convolve_st_upd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_CUSTOM
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_rsck_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction gemm_function;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
#define LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_RSCK
# include "template/libxsmm_dnn_convolve_st_upd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_RSCK
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_custom_custom(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->grad_output == 0 || handle->grad_filter == 0 || handle->scratch3 == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we scratch for MB parallel execution */
  if ( (handle->upd_use_thread_fil == 1) && (handle->scratch4 == 0) ) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_upd_custom_custom_f32_f32( handle, start_thread, tid);
    }
#if defined(LIBXSMM_INTRINSICS_AVX512_CPX) /*__AVX512F__,__AVX512BW__,__AVX512DQ__,__AVX512BF16__*/
    else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CORE && libxsmm_target_archid < LIBXSMM_X86_AVX512_CPX ) {
      status = libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16_emu( handle, start_thread, tid);
    } else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CPX ) {
      status = libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16( handle, start_thread, tid);
    }
#elif defined(LIBXSMM_INTRINSICS_AVX512_CORE) /*__AVX512F__,__AVX512BW__,__AVX512DQ__*/
    else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CORE ) {
      status = libxsmm_dnn_convolve_st_upd_custom_custom_bf16_bf16_emu( handle, start_thread, tid);
    }
#endif
    else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  } else
#endif
  {
    if (handle->datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction gemm_function;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
# include "template/libxsmm_dnn_convolve_st_upd_custom_custom_generic.tpl.c"
    }
    else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_custom(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->grad_output == 0 || handle->grad_filter == 0 || handle->scratch3 == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we scratch for MB parallel execution */
  if ( (handle->upd_use_thread_fil == 1) && (handle->scratch4 == 0) ) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_upd_nhwc_custom_f32_f32( handle, start_thread, tid);
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  } else
#endif
  {
    if (handle->datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction gemm_function;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
#define LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_CUSTOM
# include "template/libxsmm_dnn_convolve_st_upd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_CUSTOM
    }
    else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_upd_nhwc_rsck(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->grad_output == 0 || handle->grad_filter == 0 || handle->scratch3 == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we scratch for MB parallel execution */
  if ( (handle->upd_use_thread_fil == 1) && (handle->scratch4 == 0) ) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_upd_nhwc_rsck_f32_f32( handle, start_thread, tid);
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  } else
#endif
  {
    if (handle->datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction gemm_function;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
#define LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_RSCK
# include "template/libxsmm_dnn_convolve_st_upd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_UPD_DIRECT_GENERIC_NHWC_RSCK
    }
    else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Alexander Heinecke, Evangelos Georganas, Hans Pabst (Intel Corp.)
******************************************************************************/
#include "libxsmm_dnn_convolution_forward.h"
#include <libxsmm_intrinsics_x86.h>
#include "libxsmm_main.h"
#include <libxsmm.h>

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <string.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_rsck_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16_emu(libxsmm_dnn_layer* handle, int start_thread, int tid);
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid);
#if 0
LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_i8_i32(libxsmm_dnn_layer* handle, int start_thread, int tid);
#endif


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->ifmblock : (libxsmm_blasint)handle->desc.v*handle->ifmblock;
  const libxsmm_blasint ldA = handle->ofmblock;
  const libxsmm_blasint ldC = handle->ofmblock;
  const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
  int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
  /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
  gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
# include "template/libxsmm_dnn_convolve_st_fwd_custom_custom_generic.tpl.c"
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16_emu(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512_CORE) /*__AVX512F__,__AVX512BW__,__AVX512DQ__*/
  const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->ifmblock : (libxsmm_blasint)handle->desc.v*handle->ifmblock;
  const libxsmm_blasint ldA = handle->ofmblock;
  const libxsmm_blasint ldC = handle->ofmblock;
  const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
  typedef libxsmm_bfloat16 element_input_type;
  typedef libxsmm_bfloat16 element_output_type;
  typedef libxsmm_bfloat16 element_filter_type;
  typedef libxsmm_bsmmfunction_reducebatch_addr gemm_br_function;
  typedef libxsmm_bmmfunction_reducebatch_addr gemm_br_function_bf16bf16;
  int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
  /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
  gemm_br_function br_gemm_kernel = libxsmm_bsmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function br_gemm_kernel2 = libxsmm_bsmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function_bf16bf16 br_gemm_kernel_bf16bf16 = libxsmm_bmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function_bf16bf16 br_gemm_kernel2_bf16bf16 = libxsmm_bmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
# include "template/libxsmm_dnn_convolve_st_fwd_custom_custom_generic_bf16.tpl.c"
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


#if defined(LIBXSMM_INTRINSICS_AVX512_CPX)
LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CPX)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512_CPX) /*__AVX512F__,__AVX512BW__,__AVX512DQ__,__AVX512BF16__*/
  const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->ifmblock : (libxsmm_blasint)handle->desc.v*handle->ifmblock;
  const libxsmm_blasint ldA = handle->ofmblock;
  const libxsmm_blasint ldC = handle->ofmblock;
  const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
  typedef libxsmm_bfloat16 element_input_type;
  typedef libxsmm_bfloat16 element_output_type;
  typedef libxsmm_bfloat16 element_filter_type;
  typedef libxsmm_bsmmfunction_reducebatch_addr gemm_br_function;
  typedef libxsmm_bmmfunction_reducebatch_addr gemm_br_function_bf16bf16;
  int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
  /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
  gemm_br_function br_gemm_kernel = libxsmm_bsmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function br_gemm_kernel2 = libxsmm_bsmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function_bf16bf16 br_gemm_kernel_bf16bf16 = libxsmm_bmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function_bf16bf16 br_gemm_kernel2_bf16bf16 = libxsmm_bmmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);

#define LIBXSMM_DNN_CONVOLUTION_FWD_AVX512_CPX
# include "template/libxsmm_dnn_convolve_st_fwd_custom_custom_generic_bf16.tpl.c"
#undef LIBXSMM_DNN_CONVOLUTION_FWD_AVX512_CPX
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}
#else
LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512_CORE)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  return libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16_emu( handle, start_thread, tid );
}
#endif


#if 0
LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom_i8_i32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
  return status;
}
#endif


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_custom_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->blocksifm*handle->ifmblock : (libxsmm_blasint)handle->blocksifm*handle->desc.v*handle->ifmblock;
  const libxsmm_blasint ldA = handle->ofmblock;
  const libxsmm_blasint ldC = handle->blocksofm*handle->ofmblock;
  const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
  int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
  /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
  gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
#define LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_CUSTOM
# include "template/libxsmm_dnn_convolve_st_fwd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_CUSTOM
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN LIBXSMM_INTRINSICS(LIBXSMM_X86_AVX512)
libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_rsck_f32_f32(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->blocksifm*handle->ifmblock : (libxsmm_blasint)handle->blocksifm*handle->desc.v*handle->ifmblock;
  const libxsmm_blasint ldA = handle->blocksofm*handle->ofmblock;
  const libxsmm_blasint ldC = handle->blocksofm*handle->ofmblock;
  const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
  typedef float element_input_type;
  typedef float element_output_type;
  typedef float element_filter_type;
  typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
  int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
  /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
  gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
  gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
#define LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_RSCK
# include "template/libxsmm_dnn_convolve_st_fwd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_RSCK
#else /* should not happen */
  LIBXSMM_UNUSED(handle); LIBXSMM_UNUSED(start_thread); LIBXSMM_UNUSED(tid);
  status = LIBXSMM_DNN_ERR_UNSUPPORTED_ARCH;
#endif
  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_custom_custom(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->reg_output == 0 || handle->reg_filter == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_fwd_custom_custom_f32_f32( handle, start_thread, tid);
    }
#if defined(LIBXSMM_INTRINSICS_AVX512_CPX) /*__AVX512F__,__AVX512BW__,__AVX512DQ__,__AVX512BF16__*/
    else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CORE && libxsmm_target_archid < LIBXSMM_X86_AVX512_CPX) {
      status = libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16_emu( handle, start_thread, tid);
    } else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CPX ) {
      status = libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16( handle, start_thread, tid);
    }
#elif defined(LIBXSMM_INTRINSICS_AVX512_CORE) /*__AVX512F__,__AVX512BW__,__AVX512DQ__*/
    else if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_BF16 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_BF16 && libxsmm_target_archid >= LIBXSMM_X86_AVX512_CORE ) {
      status = libxsmm_dnn_convolve_st_fwd_custom_custom_bf16_bf16_emu( handle, start_thread, tid);
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
      const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->ifmblock : (libxsmm_blasint)handle->desc.v*handle->ifmblock;
      const libxsmm_blasint ldA = handle->ofmblock;
      const libxsmm_blasint ldC = handle->ofmblock;
      const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
      int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
      /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
      gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
      gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
# include "template/libxsmm_dnn_convolve_st_fwd_custom_custom_generic.tpl.c"
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_custom(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->reg_output == 0 || handle->reg_filter == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_fwd_nhwc_custom_f32_f32( handle, start_thread, tid);
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  } else
#endif
  {
    if (handle->datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->blocksifm*handle->ifmblock : (libxsmm_blasint)handle->blocksifm*handle->desc.v*handle->ifmblock;
      const libxsmm_blasint ldA = handle->ofmblock;
      const libxsmm_blasint ldC = handle->blocksofm*handle->ofmblock;
      const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
      int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
      /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
      gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
      gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
#define LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_CUSTOM
# include "template/libxsmm_dnn_convolve_st_fwd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_CUSTOM
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_convolve_st_fwd_nhwc_rsck(libxsmm_dnn_layer* handle, int start_thread, int tid)
{
  libxsmm_dnn_err_t status = LIBXSMM_DNN_SUCCESS;

  /* check if we have input, output and filter */
  if (handle->reg_input == 0 || handle->reg_output == 0 || handle->reg_filter == 0) {
    status = LIBXSMM_DNN_ERR_DATA_NOT_BOUND;
    return status;
  }

  /* check if we are on AVX512 */
#if defined(LIBXSMM_INTRINSICS_AVX512) /*__AVX512F__*/
  if ( libxsmm_target_archid >= LIBXSMM_X86_AVX512 ) {
    if ( handle->desc.datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->desc.datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      status = libxsmm_dnn_convolve_st_fwd_nhwc_rsck_f32_f32( handle, start_thread, tid);
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  } else
#endif
  {
    if (handle->datatype_in == LIBXSMM_DNN_DATATYPE_F32 && handle->datatype_out == LIBXSMM_DNN_DATATYPE_F32 ) {
      const libxsmm_blasint ldx = (handle->pack_input == 1) ? (libxsmm_blasint)handle->blocksifm*handle->ifmblock : (libxsmm_blasint)handle->blocksifm*handle->desc.v*handle->ifmblock;
      const libxsmm_blasint ldA = handle->blocksofm*handle->ofmblock;
      const libxsmm_blasint ldC = handle->blocksofm*handle->ofmblock;
      const float beta = (handle->avoid_acc_load) ? 0.f : 1.f;
      typedef float element_input_type;
      typedef float element_output_type;
      typedef float element_filter_type;
      typedef libxsmm_smmfunction_reducebatch_addr gemm_br_function;
      int l_flags = ( LIBXSMM_GEMM_FLAGS('N', 'N') ) | handle->fwd_flags;
      /* let's do a ofmblock x ofw_rb x ifmblock GEMM :-) or in other words M=nbOfm, N=ofw, K=nbIfm (col-major) */
      gemm_br_function br_gemm_kernel = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*handle->fwd_ofw_rb, handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
      gemm_br_function br_gemm_kernel2 = libxsmm_smmdispatch_reducebatch_addr(handle->ofmblock, handle->fwd_ofh_rb*(handle->fwd_ofw_rb-1), handle->ifmblock, &ldA, &ldx, &ldC, NULL, &beta, &l_flags, NULL);
#define LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_RSCK
# include "template/libxsmm_dnn_convolve_st_fwd_nhwc_custom-rsck_generic.tpl.c"
#undef LIBXSMM_DNN_TPL_FWD_DIRECT_GENERIC_NHWC_RSCK
    } else {
      status = LIBXSMM_DNN_ERR_UNSUPPORTED_DATATYPE;
      return status;
    }
  }

  return status;
}


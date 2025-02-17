/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Alexander Heinecke (Intel Corp.)
******************************************************************************/
#include "generator_spgemm_csr_bsparse_soa.h"
#include "generator_x86_instructions.h"
#include "generator_gemm_common.h"
#include "generator_common.h"
#include "libxsmm_main.h"

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

LIBXSMM_API_INTERN
void libxsmm_generator_spgemm_csr_bsparse_soa( libxsmm_generated_code*         io_generated_code,
                                               const libxsmm_gemm_descriptor*  i_xgemm_desc,
                                               const char*                     i_arch,
                                               const unsigned int*             i_row_idx,
                                               const unsigned int*             i_column_idx,
                                               const void*                     i_values ) {
  if ( strcmp(i_arch, "knl") == 0 ||
       strcmp(i_arch, "knm") == 0 ||
       strcmp(i_arch, "skx") == 0 ||
       strcmp(i_arch, "clx") == 0 ||
       strcmp(i_arch, "cpx") == 0 ||
       strcmp(i_arch, "hsw") == 0 ||
       strcmp(i_arch, "snb") == 0 ) {
    if ( strcmp(i_arch, "snb") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX;
    } else if ( strcmp(i_arch, "hsw") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX2;
    } else if ( strcmp(i_arch, "knl") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX512_MIC;
    } else if ( strcmp(i_arch, "knm") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX512_KNM;
    } else if ( strcmp(i_arch, "skx") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX512_CORE;
    } else if ( strcmp(i_arch, "clx") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX512_CLX;
    } else if ( strcmp(i_arch, "cpx") == 0 ) {
      io_generated_code->arch = LIBXSMM_X86_AVX512_CPX;
    } else {
      /* cannot happen */
    }

    libxsmm_generator_spgemm_csr_bsparse_soa_avx256_512( io_generated_code,
                                                         i_xgemm_desc,
                                                         i_row_idx,
                                                         i_column_idx,
                                                         i_values );
  } else {
    fprintf( stderr, "CSR + SOA is only available for AVX/AVX2/AVX512 at this point\n" );
    exit(-1);
  }
}

LIBXSMM_API_INTERN
void libxsmm_generator_spgemm_csr_bsparse_soa_avx256_512( libxsmm_generated_code*         io_generated_code,
                                                          const libxsmm_gemm_descriptor*  i_xgemm_desc,
                                                          const unsigned int*             i_row_idx,
                                                          const unsigned int*             i_column_idx,
                                                          const void*                     i_values ) {
  unsigned int l_m;
  unsigned int l_n;
  unsigned int l_k;
  unsigned int l_z;
  unsigned int l_row_elements;
  unsigned int l_soa_width;
  unsigned int l_max_cols = 0;
  unsigned int l_n_processed = 0;
  unsigned int l_n_limit = 0;
  unsigned int l_n_chunks = 0;
  unsigned int l_n_chunksize = 0;
  unsigned int l_found_mul = 0;
  unsigned int l_max_reg_block = 0;

  libxsmm_micro_kernel_config l_micro_kernel_config;
  libxsmm_loop_label_tracker l_loop_label_tracker;
  libxsmm_gp_reg_mapping l_gp_reg_mapping;

  LIBXSMM_UNUSED(i_values);

  /* select soa width */
  if ( LIBXSMM_GEMM_PRECISION_F64 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype )  ) {
    if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX512 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
      l_soa_width = 8;
      l_max_reg_block = 28;
    } else {
      l_soa_width = 4;
      l_max_reg_block = 14;
    }
  } else {
    if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX512 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
      l_soa_width = 16;
      l_max_reg_block = 28;
    } else {
      l_soa_width = 8;
      l_max_reg_block = 14;
    }
  }

  /* define gp register mapping */
  libxsmm_reset_x86_gp_reg_mapping( &l_gp_reg_mapping );
  /* matching calling convention on Linux */
#if defined(_WIN32) || defined(__CYGWIN__)
  l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_RCX;
  l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RDX;
  l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_R8;
  /* TODO: full support for Windows calling convention */
  l_gp_reg_mapping.gp_reg_a_prefetch = LIBXSMM_X86_GP_REG_RDI;
  l_gp_reg_mapping.gp_reg_b_prefetch = LIBXSMM_X86_GP_REG_RSI;
#else /* match calling convention on Linux */
  l_gp_reg_mapping.gp_reg_a = LIBXSMM_X86_GP_REG_RDI;
  l_gp_reg_mapping.gp_reg_b = LIBXSMM_X86_GP_REG_RSI;
  l_gp_reg_mapping.gp_reg_c = LIBXSMM_X86_GP_REG_RDX;
  l_gp_reg_mapping.gp_reg_a_prefetch = LIBXSMM_X86_GP_REG_RCX;
  l_gp_reg_mapping.gp_reg_b_prefetch = LIBXSMM_X86_GP_REG_R8;
#endif
  l_gp_reg_mapping.gp_reg_c_prefetch = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_mloop = LIBXSMM_X86_GP_REG_R12;
  l_gp_reg_mapping.gp_reg_nloop = LIBXSMM_X86_GP_REG_R13;
  l_gp_reg_mapping.gp_reg_kloop = LIBXSMM_X86_GP_REG_R14;
  l_gp_reg_mapping.gp_reg_help_0 = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_help_1 = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_help_2 = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_help_3 = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_help_4 = LIBXSMM_X86_GP_REG_UNDEF;
  l_gp_reg_mapping.gp_reg_help_5 = LIBXSMM_X86_GP_REG_UNDEF;

  /* define loop_label_tracker */
  libxsmm_reset_loop_label_tracker( &l_loop_label_tracker );

  /* define the micro kernel code gen properties */
  libxsmm_generator_gemm_init_micro_kernel_config_fullvector( &l_micro_kernel_config, io_generated_code->arch, i_xgemm_desc, 0 );

  /* get max column in C */
  for ( l_m = 0; l_m < i_row_idx[i_xgemm_desc->k]; l_m++ ) {
    if (l_max_cols < i_column_idx[l_m]) {
      l_max_cols = i_column_idx[l_m];
    }
  }
  l_max_cols++;

  /* calculate the chunk size of current columns to work on */
  l_n_chunks = ( (l_max_cols % l_max_reg_block) == 0 ) ? (l_max_cols / l_max_reg_block) : (l_max_cols / l_max_reg_block) + 1;
  l_n_chunksize = ( (l_max_cols % l_n_chunks) == 0 ) ? (l_max_cols / l_n_chunks) : (l_max_cols / l_n_chunks) + 1;

  /* open asm */
  libxsmm_x86_instruction_open_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );

  /* m loop */
  libxsmm_x86_instruction_register_jump_back_label( io_generated_code, &l_loop_label_tracker );
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_add_instruction, l_gp_reg_mapping.gp_reg_mloop, 1 );

  /* loop over n-blocks */
  l_n_processed = 0;
  l_n_limit = l_n_chunksize;
  while ( l_n_processed < l_max_cols ) {
#if 0
    printf("l_max_cols: %i, l_n_processed: %i, l_n_limit: %i\n", l_max_cols, l_n_processed, l_n_limit);
#endif
    /* load C accumulator */
    for ( l_n = 0; l_n < l_n_limit - l_n_processed; l_n++ ) {
      if (0 != (LIBXSMM_GEMM_FLAG_BETA_0 & i_xgemm_desc->flags)) { /* Beta=0 */
        libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                 l_micro_kernel_config.instruction_set,
                                                 l_micro_kernel_config.vxor_instruction,
                                                 l_micro_kernel_config.vector_name,
                                                 l_n, l_n, l_n );
      } else {
        libxsmm_x86_instruction_vec_move( io_generated_code,
                                          l_micro_kernel_config.instruction_set,
                                          l_micro_kernel_config.c_vmove_instruction,
                                          l_gp_reg_mapping.gp_reg_c,
                                          LIBXSMM_X86_GP_REG_UNDEF, 0,
                                          (l_n_processed + l_n)*l_soa_width*l_micro_kernel_config.datatype_size,
                                          l_micro_kernel_config.vector_name,
                                          l_n, 0, 1, 0 );
      }
    }

    /* do dense soa times sparse multiplication */
    for ( l_k = 0; l_k < (unsigned int)i_xgemm_desc->k; l_k++ ) {
      l_row_elements = i_row_idx[l_k+1] - i_row_idx[l_k];
      l_found_mul = 0;
      /* check if we actually need to multiply */
      for ( l_z = 0; l_z < l_row_elements; l_z++ ) {
        if ( (i_column_idx[i_row_idx[l_k] + l_z] < (unsigned int)i_xgemm_desc->n) &&
             (i_column_idx[i_row_idx[l_k] + l_z] >= l_n_processed)                &&
             (i_column_idx[i_row_idx[l_k] + l_z] < l_n_limit) )                        {
          l_found_mul = 1;
        }
      }
      /* only load A if multiplication loop is not empty */
      if (l_found_mul != 0) {
        libxsmm_x86_instruction_vec_move( io_generated_code,
                                          l_micro_kernel_config.instruction_set,
                                          l_micro_kernel_config.a_vmove_instruction,
                                          l_gp_reg_mapping.gp_reg_a,
                                          LIBXSMM_X86_GP_REG_UNDEF, 0,
                                          l_k*l_soa_width*l_micro_kernel_config.datatype_size,
                                          l_micro_kernel_config.vector_name,
                                          /*28+(l_k%4), 0, 0 );*/
                                          l_max_reg_block, 0, 1, 0 );
      }
      /* loop over element in the row of B and multiply*/
      for ( l_z = 0; l_z < l_row_elements; l_z++ ) {
        /* check k such that we just use columns which actually need to be multiplied */
        if ( (i_column_idx[i_row_idx[l_k] + l_z] < (unsigned int)i_xgemm_desc->n) &&
             (i_column_idx[i_row_idx[l_k] + l_z] >= l_n_processed)                &&
             (i_column_idx[i_row_idx[l_k] + l_z] < l_n_limit) )                        {
          if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX512 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
            libxsmm_x86_instruction_vec_compute_mem( io_generated_code,
                                                       l_micro_kernel_config.instruction_set,
                                                       l_micro_kernel_config.vmul_instruction,
                                                       1,
                                                       l_gp_reg_mapping.gp_reg_b,
                                                       LIBXSMM_X86_GP_REG_UNDEF,
                                                       0,
                                                       (i_row_idx[l_k] + l_z) * l_micro_kernel_config.datatype_size,
                                                       l_micro_kernel_config.vector_name,
                                                       l_max_reg_block, /*28+(l_k%4),*/
                                                       i_column_idx[i_row_idx[l_k] + l_z] - l_n_processed );
          } else if ( io_generated_code->arch == LIBXSMM_X86_AVX2 ) {
            libxsmm_x86_instruction_vec_move( io_generated_code,
                                              l_micro_kernel_config.instruction_set,
                                              l_micro_kernel_config.b_vmove_instruction,
                                              l_gp_reg_mapping.gp_reg_b,
                                              LIBXSMM_X86_GP_REG_UNDEF, 0,
                                              (i_row_idx[l_k] + l_z) * l_micro_kernel_config.datatype_size,
                                              l_micro_kernel_config.vector_name,
                                              15, 0, 1, 0 );
            libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                     l_micro_kernel_config.instruction_set,
                                                     l_micro_kernel_config.vmul_instruction,
                                                     l_micro_kernel_config.vector_name,
                                                     l_max_reg_block,
                                                     15,
                                                     i_column_idx[i_row_idx[l_k] + l_z] - l_n_processed );
          } else if ( io_generated_code->arch == LIBXSMM_X86_AVX ) {
            libxsmm_x86_instruction_vec_move( io_generated_code,
                                              l_micro_kernel_config.instruction_set,
                                              l_micro_kernel_config.b_vmove_instruction,
                                              l_gp_reg_mapping.gp_reg_b,
                                              LIBXSMM_X86_GP_REG_UNDEF, 0,
                                              (i_row_idx[l_k] + l_z) * l_micro_kernel_config.datatype_size,
                                              l_micro_kernel_config.vector_name,
                                              15, 0, 1, 0 );
            libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                     l_micro_kernel_config.instruction_set,
                                                     l_micro_kernel_config.vmul_instruction,
                                                     l_micro_kernel_config.vector_name,
                                                     l_max_reg_block,
                                                     15,
                                                     15 );
            libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                     l_micro_kernel_config.instruction_set,
                                                     l_micro_kernel_config.vadd_instruction,
                                                     l_micro_kernel_config.vector_name,
                                                     15,
                                                     i_column_idx[i_row_idx[l_k] + l_z] - l_n_processed,
                                                     i_column_idx[i_row_idx[l_k] + l_z] - l_n_processed );
          } else {
          }
        }
      }
    }

    /* store C accumulator */
    for ( l_n = 0; l_n < l_n_limit - l_n_processed; l_n++ ) {
      libxsmm_x86_instruction_vec_move( io_generated_code,
                                        l_micro_kernel_config.instruction_set,
                                        l_micro_kernel_config.c_vmove_instruction,
                                        l_gp_reg_mapping.gp_reg_c,
                                        LIBXSMM_X86_GP_REG_UNDEF, 0,
                                        (l_n_processed + l_n)*l_soa_width*l_micro_kernel_config.datatype_size,
                                        l_micro_kernel_config.vector_name,
                                        l_n, 0, 0, 1 );
    }

    /* adjust n progression */
    l_n_processed += l_n_chunksize;
    l_n_limit = LIBXSMM_MIN(l_n_processed + l_n_chunksize, l_max_cols);
  }

  /* advance C pointer */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_add_instruction, l_gp_reg_mapping.gp_reg_c,
                                     l_micro_kernel_config.datatype_size*l_soa_width*i_xgemm_desc->ldc);

  /* advance A pointer */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_add_instruction, l_gp_reg_mapping.gp_reg_a,
                                   l_micro_kernel_config.datatype_size*l_soa_width*i_xgemm_desc->lda);

  /* close m loop */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_cmp_instruction, l_gp_reg_mapping.gp_reg_mloop, i_xgemm_desc->m );
  libxsmm_x86_instruction_jump_back_to_label( io_generated_code, l_micro_kernel_config.alu_jmp_instruction, &l_loop_label_tracker );

  /* close asm */
  libxsmm_x86_instruction_close_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
}


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
#include "generator_spgemm_csr_asparse_soa.h"
#include "generator_x86_instructions.h"
#include "generator_gemm_common.h"
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
void libxsmm_generator_spgemm_csr_asparse_soa( libxsmm_generated_code*         io_generated_code,
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
       strcmp(i_arch, "snb") == 0 ||
       strcmp(i_arch, "hsw") == 0 ) {
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

    libxsmm_generator_spgemm_csr_asparse_soa_n_loop( io_generated_code,
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
void libxsmm_generator_spgemm_csr_asparse_soa_n_loop( libxsmm_generated_code*         io_generated_code,
                                                      const libxsmm_gemm_descriptor*  i_xgemm_desc,
                                                      const unsigned int*             i_row_idx,
                                                      const unsigned int*             i_column_idx,
                                                      const void*                     i_values ) {
  unsigned int l_soa_width = 0;
  unsigned int l_gen_m_trips = 0;
  unsigned int l_a_is_dense = 0;
  unsigned int l_n_chunks = 0;
  unsigned int l_n_chunksize = 0;
  unsigned int l_n_remain = 0;
  unsigned int l_n_max_block = 0;
  unsigned int l_n_loop = 0;

  libxsmm_micro_kernel_config l_micro_kernel_config;
  libxsmm_loop_label_tracker l_loop_label_tracker;
  libxsmm_gp_reg_mapping l_gp_reg_mapping;

  /* define gp register mapping */
  libxsmm_reset_x86_gp_reg_mapping( &l_gp_reg_mapping );
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

  /* select soa width */
  if ( LIBXSMM_GEMM_PRECISION_F64 == LIBXSMM_GETENUM_INP( i_xgemm_desc->datatype )  ) {
    if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX512 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
      l_soa_width = 8;
      l_n_max_block = 28;
    } else {
      l_soa_width = 4;
      l_n_max_block = 14;
    }
    l_micro_kernel_config.a_vmove_instruction = LIBXSMM_X86_INSTR_VBROADCASTSD;
  } else {
    if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX512 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
      l_soa_width = 16;
      l_n_max_block = 28;
    } else {
      l_soa_width = 8;
      l_n_max_block = 14;
    }
    l_micro_kernel_config.a_vmove_instruction = LIBXSMM_X86_INSTR_VBROADCASTSS;
  }

  /* open asm */
  libxsmm_x86_instruction_open_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );

  /* test if we should generate a dense version */
  if ( i_row_idx[i_xgemm_desc->m] == (unsigned int)(i_xgemm_desc->m*i_xgemm_desc->k) ) {
    l_gen_m_trips = 1;
    l_a_is_dense = 1;
  } else {
    l_gen_m_trips = i_xgemm_desc->m;
    l_a_is_dense = 0;
  }

  /* calculate the chunk size of current columns to work on */
  l_n_chunks = ( (i_xgemm_desc->n % l_n_max_block) == 0 ) ? (i_xgemm_desc->n / l_n_max_block) : (i_xgemm_desc->n / l_n_max_block) + 1;
  l_n_chunksize = ( (i_xgemm_desc->n % l_n_chunks) == 0 ) ? (i_xgemm_desc->n / l_n_chunks) : (i_xgemm_desc->n / l_n_chunks) + 1;
  l_n_remain = ( ((i_xgemm_desc->n % l_n_chunksize) == 0) || ((unsigned int)i_xgemm_desc->n <= l_n_max_block) ) ? 0 : 1;
  l_n_loop = ( l_n_remain == 0 ) ? (l_n_chunks * l_n_chunksize) : ((l_n_chunks-1) * l_n_chunksize);

  /* loop over blocks of n */
  libxsmm_x86_instruction_register_jump_back_label( io_generated_code, &l_loop_label_tracker );
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_add_instruction, l_gp_reg_mapping.gp_reg_nloop, l_n_chunksize );

  /* do matix multiplicatoin for a block of N columns */
  libxsmm_generator_spgemm_csr_asparse_soa_m_loop( io_generated_code, i_xgemm_desc, &l_loop_label_tracker, &l_micro_kernel_config, &l_gp_reg_mapping,
                                                     i_row_idx, i_column_idx, i_values,
                                                     l_soa_width, l_gen_m_trips, l_a_is_dense, l_n_chunksize );

  /* adjust B pointer */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_add_instruction, l_gp_reg_mapping.gp_reg_b,
                                     l_micro_kernel_config.datatype_size*l_soa_width*l_n_chunksize);

  /* advance B prefetch pointer */
  if ( (i_xgemm_desc->prefetch & LIBXSMM_GEMM_PREFETCH_BL2_VIA_C) > 0 ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_sub_instruction, l_gp_reg_mapping.gp_reg_b_prefetch,
                                       (l_micro_kernel_config.datatype_size*l_soa_width*i_xgemm_desc->ldb*i_xgemm_desc->m)-(l_micro_kernel_config.datatype_size*l_soa_width*l_n_chunksize));
  }

  /* adjust C pointer */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_sub_instruction, l_gp_reg_mapping.gp_reg_c,
                                     (l_micro_kernel_config.datatype_size*l_soa_width*i_xgemm_desc->ldc*i_xgemm_desc->m)-(l_micro_kernel_config.datatype_size*l_soa_width*l_n_chunksize));


  /* N loop jump back */
  libxsmm_x86_instruction_alu_imm( io_generated_code, l_micro_kernel_config.alu_cmp_instruction, l_gp_reg_mapping.gp_reg_nloop, l_n_loop );
  libxsmm_x86_instruction_jump_back_to_label( io_generated_code, l_micro_kernel_config.alu_jmp_instruction, &l_loop_label_tracker );

  /* handle remainder of N loop */
  if ( l_n_remain != 0 ) {
    libxsmm_generator_spgemm_csr_asparse_soa_m_loop( io_generated_code, i_xgemm_desc, &l_loop_label_tracker, &l_micro_kernel_config, &l_gp_reg_mapping,
                                                       i_row_idx, i_column_idx, i_values,
                                                       l_soa_width, l_gen_m_trips, l_a_is_dense, i_xgemm_desc->n - (l_n_chunksize * (l_n_chunks - 1)) );
  }

  /* close asm */
  libxsmm_x86_instruction_close_stream( io_generated_code, &l_gp_reg_mapping, i_xgemm_desc->prefetch );
}

LIBXSMM_API_INTERN
void libxsmm_generator_spgemm_csr_asparse_soa_m_loop( libxsmm_generated_code*            io_generated_code,
                                                      const libxsmm_gemm_descriptor*     i_xgemm_desc,
                                                      libxsmm_loop_label_tracker*        io_loop_label_tracker,
                                                      const libxsmm_micro_kernel_config* i_micro_kernel_config,
                                                      const libxsmm_gp_reg_mapping*      i_gp_reg_mapping,
                                                      const unsigned int*                i_row_idx,
                                                      const unsigned int*                i_column_idx,
                                                      const void*                        i_values,
                                                      const unsigned int                 i_soa_width,
                                                      const unsigned int                 i_gen_m_trips,
                                                      const unsigned int                 i_a_is_dense,
                                                      const unsigned int                 i_num_c_cols ) {
  unsigned int l_m;
  unsigned int l_n;
  unsigned int l_z;
  unsigned int l_row_elements;
  unsigned int l_b_offset;
  unsigned int l_b_total_offset;

  LIBXSMM_UNUSED(i_values);

  /* do sparse times dense soa multiplication */
  for ( l_m = 0; l_m < i_gen_m_trips; l_m++ ) {
    /* handle b offset */
    l_b_offset = 0;
    l_b_total_offset = 0;

    /* generate M loop */
    if (i_a_is_dense != 0 ) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_mov_instruction, i_gp_reg_mapping->gp_reg_mloop, 0 );
      libxsmm_x86_instruction_register_jump_back_label( io_generated_code, io_loop_label_tracker );
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_mloop, 1 );
    }

    l_row_elements = i_row_idx[l_m+1] - i_row_idx[l_m];
    if (l_row_elements > 0) {
      /* load C accumulator */
      for ( l_n = 0; l_n < i_num_c_cols; l_n++ ) {
        if (0 != (LIBXSMM_GEMM_FLAG_BETA_0 & i_xgemm_desc->flags)) { /* Beta=0 */
          libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                   i_micro_kernel_config->instruction_set,
                                                   i_micro_kernel_config->vxor_instruction,
                                                   i_micro_kernel_config->vector_name,
                                                   l_n, l_n, l_n );
        } else {
          libxsmm_x86_instruction_vec_move( io_generated_code,
                                            i_micro_kernel_config->instruction_set,
                                            i_micro_kernel_config->c_vmove_instruction,
                                            i_gp_reg_mapping->gp_reg_c,
                                            LIBXSMM_X86_GP_REG_UNDEF, 0,
                                            l_n*i_soa_width*i_micro_kernel_config->datatype_size,
                                            i_micro_kernel_config->vector_name,
                                            l_n, 0, 1, 0 );
        }
        if ( (i_xgemm_desc->prefetch & LIBXSMM_GEMM_PREFETCH_BL2_VIA_C) > 0 ) {
          libxsmm_x86_instruction_prefetch( io_generated_code,
                                            i_micro_kernel_config->prefetch_instruction,
                                            i_gp_reg_mapping->gp_reg_b_prefetch,
                                            LIBXSMM_X86_GP_REG_UNDEF, 0,
                                            l_n*i_soa_width*i_micro_kernel_config->datatype_size );
        }
      }
      /* loop over the non-zeros in A row m */
      for ( l_z = 0; l_z < l_row_elements; l_z++ ) {
        /* broadcast values of A */
        libxsmm_x86_instruction_vec_move( io_generated_code,
                                          i_micro_kernel_config->instruction_set,
                                          i_micro_kernel_config->a_vmove_instruction,
                                          i_gp_reg_mapping->gp_reg_a,
                                          LIBXSMM_X86_GP_REG_UNDEF, 0,
                                          (i_row_idx[l_m] + l_z) * i_micro_kernel_config->datatype_size,
                                          i_micro_kernel_config->vector_name,
                                          i_num_c_cols, 0, 1, 0 );
        /* multiply with B */
        for ( l_n = 0; l_n < i_num_c_cols; l_n++ ) {
          l_b_offset = ((i_column_idx[i_row_idx[l_m] + l_z]*i_micro_kernel_config->datatype_size*i_soa_width*i_xgemm_desc->ldb)
                                                     +(l_n*i_soa_width*i_micro_kernel_config->datatype_size))-l_b_total_offset;

          if ( ( io_generated_code->arch >= LIBXSMM_X86_AVX2 ) && ( io_generated_code->arch <= LIBXSMM_X86_ALLFEAT ) ) {
            if (l_b_offset >= 8192) {
              l_b_total_offset += l_b_offset;
              libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_b,
                                               l_b_offset);
              l_b_offset = 0;
            }

            /* FMA with full vector load on AVX2 and AVX512 platforms */
            libxsmm_x86_instruction_vec_compute_mem( io_generated_code,
                                                     i_micro_kernel_config->instruction_set,
                                                     i_micro_kernel_config->vmul_instruction,
                                                     0,
                                                     i_gp_reg_mapping->gp_reg_b,
                                                     LIBXSMM_X86_GP_REG_UNDEF,
                                                     0,
                                                     l_b_offset,
                                                     i_micro_kernel_config->vector_name,
                                                     i_num_c_cols,
                                                     l_n );
          } else {
            /* Mul with full vector load and adding result to final accumulator */
            libxsmm_x86_instruction_vec_compute_mem( io_generated_code,
                                                     i_micro_kernel_config->instruction_set,
                                                     i_micro_kernel_config->vmul_instruction,
                                                     0,
                                                     i_gp_reg_mapping->gp_reg_b,
                                                     LIBXSMM_X86_GP_REG_UNDEF,
                                                     0,
                                                     l_b_offset,
                                                     i_micro_kernel_config->vector_name,
                                                     i_num_c_cols,
                                                     15 );

            libxsmm_x86_instruction_vec_compute_reg( io_generated_code,
                                                     i_micro_kernel_config->instruction_set,
                                                     i_micro_kernel_config->vadd_instruction,
                                                     i_micro_kernel_config->vector_name,
                                                     15, l_n, l_n );
          }
        }
      }
      /* store C accumulator */
      for ( l_n = 0; l_n < i_num_c_cols; l_n++ ) {
        libxsmm_x86_instruction_vec_move( io_generated_code,
                                          i_micro_kernel_config->instruction_set,
                                          i_micro_kernel_config->c_vmove_instruction,
                                          i_gp_reg_mapping->gp_reg_c,
                                          LIBXSMM_X86_GP_REG_UNDEF, 0,
                                          l_n*i_soa_width*i_micro_kernel_config->datatype_size,
                                          i_micro_kernel_config->vector_name,
                                          l_n, 0, 0, 1 );
      }
    }
    /* advance C pointer */
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_c,
                                     i_micro_kernel_config->datatype_size*i_soa_width*i_xgemm_desc->ldc);

    /* advance B prefetch pointer */
    if ( (i_xgemm_desc->prefetch & LIBXSMM_GEMM_PREFETCH_BL2_VIA_C) > 0 ) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_b_prefetch,
                                       i_micro_kernel_config->datatype_size*i_soa_width*i_xgemm_desc->ldb);
    }

    /* adjust B pointer */
    if (l_b_total_offset > 0) {
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_b,
                                       l_b_total_offset);
    }

    /* generate M loop */
    if (i_a_is_dense != 0 ) {
      /* advance A pointer */
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_add_instruction, i_gp_reg_mapping->gp_reg_a,
                                       i_micro_kernel_config->datatype_size*i_xgemm_desc->k);

      /* M loop jump back */
      libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_cmp_instruction, i_gp_reg_mapping->gp_reg_mloop, i_xgemm_desc->m );
      libxsmm_x86_instruction_jump_back_to_label( io_generated_code, i_micro_kernel_config->alu_jmp_instruction, io_loop_label_tracker );
    }
  }

  /* reset A pointer */
  if (i_a_is_dense != 0 ) {
    libxsmm_x86_instruction_alu_imm( io_generated_code, i_micro_kernel_config->alu_sub_instruction, i_gp_reg_mapping->gp_reg_a,
                                       i_micro_kernel_config->datatype_size*i_xgemm_desc->k*i_xgemm_desc->m);
  }
}


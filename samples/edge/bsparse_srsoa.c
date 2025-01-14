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

#define N_QUANTITIES 9

#include <libxsmm.h>

#include "common_edge_proxy.h"

int main(int argc, char* argv[]) {
  int M = ( argc == 7 ) ? atoi(argv[1]) : 9;
  int N = ( argc == 7 ) ? atoi(argv[2]) : 10;
  int K = ( argc == 7 ) ? atoi(argv[3]) : 20;
  unsigned int N_CRUNS = ( argc == 7 ) ? atoi(argv[4]) : 8;
  unsigned int REPS =    ( argc == 7 ) ? atoi(argv[5]) : 1;
  char* l_csr_file =     ( argc == 7 ) ?      argv[6]  : "file.csr";

  const libxsmm_gemm_prefetch_type prefetch = LIBXSMM_GEMM_PREFETCH_NONE;
  const int flags = LIBXSMM_GEMM_FLAGS('N', 'N');
  const REALTYPE alpha = 1, beta = 1;

  edge_mat_desc mat_desc = libxsmm_sparse_csr_reader_desc( l_csr_file );
  unsigned int l_rowcount = mat_desc.row_count;
  unsigned int l_colcount = mat_desc.col_count;
  unsigned int l_elements = mat_desc.num_elements;

  REALTYPE* l_a = (REALTYPE*)libxsmm_aligned_malloc(K * M * N_CRUNS * sizeof(REALTYPE), 64);
  REALTYPE* l_b_de = (REALTYPE*)libxsmm_aligned_malloc(K * N * sizeof(REALTYPE), 64);
  REALTYPE* l_b_sp = NULL;
  unsigned int* l_rowptr = NULL;
  unsigned int* l_colidx = NULL;
  REALTYPE* l_c_gold = (REALTYPE*)libxsmm_aligned_malloc(M * N * N_CRUNS * sizeof(REALTYPE), 64);
  REALTYPE* l_c_asm = (REALTYPE*)libxsmm_aligned_malloc(M * N * N_CRUNS * sizeof(REALTYPE), 64);
  REALTYPE l_max_error = 0.0;
  unsigned int l_k, l_n;
  int l_i, l_j, l_jj;

  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_a, l_a, K, N_CRUNS);
  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_c_asm, l_c_asm, N, N_CRUNS);
  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_c_gold, l_c_gold, N, N_CRUNS);

  libxsmm_descriptor_blob l_xgemm_blob;
  const libxsmm_gemm_descriptor* l_xgemm_desc = 0;
  LIBXSMM_MMFUNCTION_TYPE(REALTYPE) mykernel = NULL;

  unsigned long long l_start, l_end;
  double l_total;

  if (argc != 7) {
    fprintf( stderr, "arguments: M CRUNS #iters CSR-file!\n" );
    exit(-1);
  }

  if ((unsigned int)K != l_rowcount) {
    fprintf( stderr, "arguments K needs to match number of rows of the sparse matrix!\n" );
    exit(-1);
  }

  if ((unsigned int)N != l_colcount) {
    fprintf( stderr, "arguments N needs to match number of columns of the sparse matrix!\n" );
    exit(-1);
  }

  if (M != 9) {
    fprintf( stderr, "arguments M needs to match 9!\n" );
    exit(-1);
  }

  /* touch A */
  for ( l_i = 0; l_i < M; l_i++) {
    for ( l_j = 0; l_j < K; l_j++) {
      for ( l_k = 0; l_k < N_CRUNS; l_k++ ) {
        LIBXSMM_VLA_ACCESS(3, l_p_a, l_i, l_j, l_k, K, N_CRUNS) = (REALTYPE)libxsmm_rng_f64();
      }
    }
  }

  /* touch C */
  for ( l_i = 0; l_i < M; l_i++) {
    for ( l_j = 0; l_j < N; l_j++) {
      for ( l_k = 0; l_k < N_CRUNS; l_k++ ) {
        LIBXSMM_VLA_ACCESS(3, l_p_c_gold, l_i, l_j, l_k, N, N_CRUNS) = (REALTYPE)0.0;
        LIBXSMM_VLA_ACCESS(3, l_p_c_asm,  l_i, l_j, l_k, N, N_CRUNS) = (REALTYPE)0.0;
      }
    }
  }

  /* read B, CSR */
  libxsmm_sparse_csr_reader(  l_csr_file,
                             &l_rowptr,
                             &l_colidx,
                             &l_b_sp,
                             &l_rowcount, &l_colcount, &l_elements );

  /* copy b to dense */
  printf("CSR matrix data structure we just read:\n");
  printf("rows: %u, columns: %u, elements: %u\n", l_rowcount, l_colcount, l_elements);

  for ( l_n = 0; l_n < (((unsigned int)K) * N); l_n++) {
    l_b_de[l_n] = 0.0;
  }

  for ( l_n = 0; l_n < (unsigned int)K; l_n++) {
    const unsigned int l_rowelems = l_rowptr[l_n+1] - l_rowptr[l_n];
    assert(l_rowptr[l_n+1] >= l_rowptr[l_n]);

    for ( l_k = 0; l_k < l_rowelems; l_k++) {
      l_b_de[(l_n * N) + l_colidx[l_rowptr[l_n] + l_k]] = l_b_sp[l_rowptr[l_n] + l_k];
    }
  }

  /* dense routine */
  l_start = libxsmm_timer_tick();
#if 1
  for ( l_n = 0; l_n < REPS; l_n++) {
    for ( l_i = 0; l_i < N_QUANTITIES; l_i++) {
      for ( l_j = 0; l_j < N; l_j++) {
        for ( l_jj = 0; l_jj < K; l_jj++) {
          LIBXSMM_PRAGMA_SIMD
          for (l_k = 0; l_k < N_CRUNS; l_k++) {
            LIBXSMM_VLA_ACCESS(3, l_p_c_gold, l_i, l_j, l_k, N, N_CRUNS)
              +=   LIBXSMM_VLA_ACCESS(3, l_p_a, l_i, l_jj, l_k, K, N_CRUNS)
                 * l_b_de[(l_jj*N)+l_j];
          }
        }
      }
    }
  }
#endif
  l_end = libxsmm_timer_tick();
  l_total = libxsmm_timer_duration(l_start, l_end);
  printf("%fs for dense\n", l_total);
  printf("%f GFLOPS for dense\n", ((double)((double)REPS * (double)M * (double)N * (double)K * (double)N_CRUNS) * 2.0) / (l_total * 1.0e9));

  l_xgemm_desc = libxsmm_gemm_descriptor_dinit(&l_xgemm_blob, LIBXSMM_GEMM_PRECISION(REALTYPE),
    M, N, K, K, 0, N, alpha, beta, flags, prefetch);

  /* sparse routine */
#if defined(__EDGE_EXECUTE_F32__)
  mykernel = libxsmm_create_xcsr_soa( l_xgemm_desc, l_rowptr, l_colidx, (const void*)l_b_sp ).smm;
#else
  mykernel = libxsmm_create_xcsr_soa( l_xgemm_desc, l_rowptr, l_colidx, (const void*)l_b_sp ).dmm;
#endif

  l_start = libxsmm_timer_tick();
  for ( l_n = 0; l_n < REPS; l_n++) {
    mykernel( l_a, l_b_sp, l_c_asm );
  }
  l_end = libxsmm_timer_tick();
  l_total = libxsmm_timer_duration(l_start, l_end);
  printf("%fs for sparse (asm)\n", l_total);
  printf("%f GFLOPS for sparse (asm)\n", ((double)((double)REPS * (double)M * (double)l_elements * (double)N_CRUNS) * 2.0) / (l_total * 1.0e9));

  /* check for errors */
  l_max_error = (REALTYPE)0.0;
  for ( l_i = 0; l_i < M; l_i++) {
    for ( l_j = 0; l_j < N; l_j++) {
      for ( l_k = 0; l_k < N_CRUNS; l_k++ ) {
        if (fabs( LIBXSMM_VLA_ACCESS(3, l_p_c_gold, l_i, l_j, l_k, N, N_CRUNS)
                    - LIBXSMM_VLA_ACCESS(3, l_p_c_asm, l_i, l_j, l_k, N, N_CRUNS) ) > l_max_error ) {
          l_max_error = (REALTYPE)fabs( LIBXSMM_VLA_ACCESS(3, l_p_c_gold, l_i, l_j, l_k, N, N_CRUNS)
                                       -LIBXSMM_VLA_ACCESS(3, l_p_c_asm, l_i, l_j, l_k, N, N_CRUNS) );
        }
      }
    }
  }
  printf("max error: %f\n", l_max_error);

  printf("PERFDUMP,%s,%u,%i,%i,%i,%u,%u,%f,%f,%f\n", l_csr_file, REPS, M, N, K, l_elements, M * l_elements * N_CRUNS * 2, l_max_error, l_total, ((double)((double)REPS * (double)M * (double)l_elements * (double)N_CRUNS) * 2.0) / (l_total * 1.0e9) );

  /* free */
  libxsmm_free( l_b_de );
  libxsmm_free( l_a );
  libxsmm_free( l_c_gold );
  libxsmm_free( l_c_asm );

  free( l_b_sp );
  free( l_rowptr );
  free( l_colidx );

  return 0;
}


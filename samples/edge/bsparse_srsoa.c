/******************************************************************************
** Copyright (c) 2014-2018, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Alexander Heinecke (Intel Corp.)
******************************************************************************/

#define N_QUANTITIES 9

#include <libxsmm.h>

#include "common_edge_proxy.h"

int main(int argc, char* argv[]) {
  unsigned int M =       ( argc == 7 ) ? atoi(argv[1]) : 9;
  unsigned int N =       ( argc == 7 ) ? atoi(argv[2]) : 10;
  unsigned int K =       ( argc == 7 ) ? atoi(argv[3]) : 20;
  unsigned int N_CRUNS = ( argc == 7 ) ? atoi(argv[4]) : 8;
  unsigned int REPS =    ( argc == 7 ) ? atoi(argv[5]) : 1;
  char* l_csr_file =     ( argc == 7 ) ?      argv[6]  : "file.csr" ;

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
  unsigned int l_i;
  unsigned int l_j;
  unsigned int l_k;
  unsigned int l_jj;
  unsigned int l_n;

  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_a, l_a, K, N_CRUNS);
  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_c_asm, l_c_asm, N, N_CRUNS);
  LIBXSMM_VLA_DECL(3, REALTYPE, l_p_c_gold, l_c_gold, N, N_CRUNS);

  libxsmm_gemm_descriptor l_xgemm_desc;
#if defined(__EDGE_EXECUTE_F32__)
  libxsmm_smmfunction mykernel = NULL;
#else
  libxsmm_dmmfunction mykernel = NULL;
#endif

  unsigned long long l_start, l_end;
  double l_total;

  if (argc != 7) {
    fprintf( stderr, "arguments: M CRUNS #iters CSR-file!\n" );
    exit(-1);
  }

  if (K != l_rowcount) {
    fprintf( stderr, "arguments K needs to match number of rows of the sparse matrix!\n" );
    exit(-1);
  }

  if (N != l_colcount) {
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
        LIBXSMM_VLA_ACCESS(3, l_p_a, l_i, l_j, l_k, K, N_CRUNS) = (REALTYPE)drand48();
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

  for ( l_n = 0; l_n < (K * N); l_n++) {
    l_b_de[l_n] = 0.0;
  }

  for ( l_n = 0; l_n < K; l_n++) {
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

  /* sparse routine */
#if defined(__EDGE_EXECUTE_F32__)
  LIBXSMM_GEMM_DESCRIPTOR(l_xgemm_desc, LIBXSMM_GEMM_PRECISION_F32, 0/*flags*/,
    M, N, K, K, 0, N,
    1.0, 1.0, LIBXSMM_PREFETCH_NONE);
  mykernel = libxsmm_create_xcsr_soa( &l_xgemm_desc, l_rowptr, l_colidx, (const void*)l_b_sp ).smm;
#else
  LIBXSMM_GEMM_DESCRIPTOR(l_xgemm_desc, LIBXSMM_GEMM_PRECISION_F64, 0/*flags*/,
    M, N, K, K, 0, N,
    1.0, 1.0, LIBXSMM_PREFETCH_NONE);
  mykernel = libxsmm_create_xcsr_soa( &l_xgemm_desc, l_rowptr, l_colidx, (const void*)l_b_sp ).dmm;
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

  printf("PERFDUMP,%s,%i,%i,%i,%i,%i,%i,%f,%f,%f\n", l_csr_file, REPS, M, N, K, l_elements, M * l_elements * N_CRUNS * 2, l_max_error, l_total, ((double)((double)REPS * (double)M * (double)l_elements * (double)N_CRUNS) * 2.0) / (l_total * 1.0e9) );

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
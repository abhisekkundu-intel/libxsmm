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
#ifndef LIBXSMM_DNN_FULLYCONNECTED_WEIGHT_UPDATE_H
#define LIBXSMM_DNN_FULLYCONNECTED_WEIGHT_UPDATE_H

#include <libxsmm_dnn_fullyconnected.h>

LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_fullyconnected_st_upd_custom(libxsmm_dnn_fullyconnected* handle, int start_thread, int tid);

LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_fullyconnected_st_upd_ncnc_kcck(libxsmm_dnn_fullyconnected* handle, int start_thread, int tid);

LIBXSMM_API_INTERN libxsmm_dnn_err_t libxsmm_dnn_fullyconnected_st_upd_nhwc(libxsmm_dnn_fullyconnected* handle, int start_thread, int tid);

#endif /* LIBXSMM_DNN_FULLYCONNECTED_WEIGHT_UPDATE_H */

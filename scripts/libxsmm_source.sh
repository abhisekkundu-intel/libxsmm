#!/bin/sh

SRCDIR=../src
GREP=$(command -v grep)

if [ "" = "${GREP}" ]; then
  echo "Error: missing prerequisites!"
  exit 1
fi
cat << EOM
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
#ifndef LIBXSMM_SOURCE_H
#define LIBXSMM_SOURCE_H

#if defined(LIBXSMM_MACROS_H)
# error Please do not include any LIBXSMM header other than libxsmm_source.h!
#endif
#if defined(LIBXSMM_BUILD)
# error LIBXSMM_BUILD cannot be defined for the header-only LIBXSMM!
#endif
#if !defined(LIBXSMM_API)
# define LIBXSMM_API LIBXSMM_EXTERN_C LIBXSMM_INLINE LIBXSMM_RETARGETABLE
#endif

/**
 * This header is intentionally called "libxsmm_source.h" since the followings block
 * includes *internal* files, and thereby exposes LIBXSMM's implementation.
 * The so-called "header-only" usage model gives up the clearly defined binary interface
 * (including support for hot-fixes after deployment), and requires to rebuild client
 * code for every (internal) change of LIBXSMM. Please make sure to only rely on the
 * public interface as the internal implementation may change without notice.
 */
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
EOM

HERE=$(cd $(dirname $0); pwd -P)

if [ "" = "$1" ]; then
  DSTDIR=${SRCDIR}
else
  DSTDIR=$1
fi

# determine order of filenames in directory list
export LC_ALL=C

# good-enough pattern to match a main function, and to exclude this translation unit
for FILE in $(${GREP} -L "main[[:space:]]*(.*)" ${HERE}/${SRCDIR}/*.c); do
  BASENAME=$(basename ${FILE})
  echo "#include \"${DSTDIR}/${BASENAME}\""
done

cat << EOM
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif

#endif /*LIBXSMM_SOURCE_H*/
EOM


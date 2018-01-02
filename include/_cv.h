#ifndef _VOS_INTERNAL_H_
#define _VOS_INTERNAL_H_

#include "cv.h"
#include "cxmisc.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
/* helper tables */
extern const uchar iSaturate8u_cv[];
#define VOS_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), iSaturate8u_cv[(t)+256])
#define VOS_CALC_MIN_8U(a,b) (a) -= VOS_FAST_CAST_8U((a) - (b))
#define VOS_CALC_MAX_8U(a,b) (a) += VOS_FAST_CAST_8U((b) - (a))

extern const float i8x32fTab_cv[];
#define VOS_8TO32F(x)  i8x32fTab_cv[(x)+256]
#define  VOS_CALC_MIN(a, b) if((a) > (b)) (a) = (b)
#define  VOS_CALC_MAX(a, b) if((a) < (b)) (a) = (b)

#endif /*_VOS_INTERNAL_H_*/

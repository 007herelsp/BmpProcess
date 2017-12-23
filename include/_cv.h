

#ifndef _CV_INTERNAL_H_
#define _CV_INTERNAL_H_

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

/* helper tables */
extern const uchar icvSaturate8u_cv[];
#define CV_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), icvSaturate8u_cv[(t)+256])
#define CV_CALC_MIN_8U(a,b) (a) -= CV_FAST_CAST_8U((a) - (b))
#define CV_CALC_MAX_8U(a,b) (a) += CV_FAST_CAST_8U((b) - (a))

extern const float icv8x32fTab_cv[];
#define CV_8TO32F(x)  icv8x32fTab_cv[(x)+256]


#define  CV_CALC_MIN(a, b) if((a) > (b)) (a) = (b)

#define  CV_CALC_MAX(a, b) if((a) < (b)) (a) = (b)



#endif /*_CV_INTERNAL_H_*/

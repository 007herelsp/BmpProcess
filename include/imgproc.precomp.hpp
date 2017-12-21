
#ifndef __OPENCV_PRECOMP_H__
#define __OPENCV_PRECOMP_H__


#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/core/internal.hpp"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>


#define GET_OPTIMIZED(func) (func)

/* helper tables */
extern const uchar icvSaturate8u_cv[];
#define CV_FAST_CAST_8U(t)  (assert(-256 <= (t) && (t) <= 512), icvSaturate8u_cv[(t)+256])
#define CV_CALC_MIN_8U(a,b) (a) -= CV_FAST_CAST_8U((a) - (b))
#define CV_CALC_MAX_8U(a,b) (a) += CV_FAST_CAST_8U((b) - (a))

// -256.f ... 511.f
extern const float icv8x32fTab_cv[];
#define CV_8TO32F(x)  icv8x32fTab_cv[(x)+256]

// (-128.f)^2 ... (255.f)^2
extern const float icv8x32fSqrTab[];
#define CV_8TO32F_SQR(x)  icv8x32fSqrTab[(x)+128]

namespace cv
{

static inline Point normalizeAnchor( Point anchor, Size ksize )
{
    if( anchor.x == -1 )
        anchor.x = ksize.width/2;
    if( anchor.y == -1 )
        anchor.y = ksize.height/2;
    CV_Assert( anchor.inside(Rect(0, 0, ksize.width, ksize.height)) );
    return anchor;
}

void preprocess2DKernel( const Mat& kernel, vector<Point>& coords, vector<uchar>& coeffs );


}


#endif /*__OPENCV_CV_INTERNAL_H_*/

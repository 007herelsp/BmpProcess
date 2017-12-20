
#ifndef __OPENCV_PRECOMP_H__
#define __OPENCV_PRECOMP_H__

#include "imgproc.hpp"
#include "imgproc_c.h"
#include "internal.hpp"


#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>


#define GET_OPTIMIZED(func) (func)

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

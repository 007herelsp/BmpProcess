
#ifndef __OPENCV_PRECOMP_H__
#define __OPENCV_PRECOMP_H__


#include "core.hpp"
#include "core_c.h"
#include "internal.hpp"

#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define GET_OPTIMIZED(func) (func)


namespace cv
{

// -128.f ... 255.f


inline Size getContinuousSize_(int flags, int cols, int rows, int widthScale)
{
    int64 sz = (int64)cols * rows * widthScale;
    return (flags & Mat::CONTINUOUS_FLAG) != 0 &&
        (int)sz == sz ? Size((int)sz, 1) : Size(cols * widthScale, rows);
}

inline Size getContinuousSize(const Mat& m1, int widthScale = 1)
{
    return getContinuousSize_(m1.flags,
                              m1.cols, m1.rows, widthScale);
}

inline Size getContinuousSize(const Mat& m1, const Mat& m2, int widthScale = 1)
{
    return getContinuousSize_(m1.flags & m2.flags,
                              m1.cols, m1.rows, widthScale);
}



enum { BLOCK_SIZE = 1024 };


#define ARITHM_USE_IPP 0
#define IF_IPP(then_call, else_call) else_call

inline bool checkScalar(const Mat& sc, int atype, int sckind, int akind)
{
    if( sc.dims > 2 || (sc.cols != 1 && sc.rows != 1) || !sc.isContinuous() )
        return false;
    int cn = CV_MAT_CN(atype);
    if( akind == _InputArray::MATX && sckind != _InputArray::MATX )
        return false;
    return sc.size() == Size(1, 1) || sc.size() == Size(1, cn) || sc.size() == Size(cn, 1) ||
           (sc.size() == Size(1, 4) && sc.type() == CV_64F && cn <= 4);
}

void convertAndUnrollScalar( const Mat& sc, int buftype, uchar* scbuf, size_t blocksize );

}

#endif /*_CXCORE_INTERNAL_H_*/

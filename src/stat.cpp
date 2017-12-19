#include "core.precomp.hpp"
#include <climits>

namespace cv
{



template<typename T>
static int countNonZero_(const T* src, int len )
{
    int i=0, nz = 0;
    for( ; i < len; i++ )
        nz += src[i] != 0;
    return nz;
}

static int countNonZero8u( const uchar* src, int len )
{
    int i=0, nz = 0;
    for( ; i < len; i++ )
        nz += src[i] != 0;
    return nz;
}

static int countNonZero16u( const ushort* src, int len )
{ return countNonZero_(src, len); }

static int countNonZero32s( const int* src, int len )
{ return countNonZero_(src, len); }

static int countNonZero32f( const float* src, int len )
{ return countNonZero_(src, len); }

static int countNonZero64f( const double* src, int len )
{ return countNonZero_(src, len); }

typedef int (*CountNonZeroFunc)(const uchar*, int);

static CountNonZeroFunc getCountNonZeroTab(int depth)
{
    static CountNonZeroFunc countNonZeroTab[] =
    {
        (CountNonZeroFunc)GET_OPTIMIZED(countNonZero8u), (CountNonZeroFunc)GET_OPTIMIZED(countNonZero8u),
        (CountNonZeroFunc)GET_OPTIMIZED(countNonZero16u), (CountNonZeroFunc)GET_OPTIMIZED(countNonZero16u),
        (CountNonZeroFunc)GET_OPTIMIZED(countNonZero32s), (CountNonZeroFunc)GET_OPTIMIZED(countNonZero32f),
        (CountNonZeroFunc)GET_OPTIMIZED(countNonZero64f), 0
    };

    return countNonZeroTab[depth];
}



}


int cv::countNonZero( InputArray _src )
{
    Mat src = _src.getMat();
    CountNonZeroFunc func = getCountNonZeroTab(src.depth());

    CV_Assert( src.channels() == 1 && func != 0 );

    const Mat* arrays[] = {&src, 0};
    uchar* ptrs[1];
    NAryMatIterator it(arrays, ptrs);
    int total = (int)it.size, nz = 0;

    for( size_t i = 0; i < it.nplanes; i++, ++it )
        nz += func( ptrs[0], total );

    return nz;
}



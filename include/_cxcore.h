

#ifndef _CXCORE_INTERNAL_H_
#define _CXCORE_INTERNAL_H_
typedef unsigned long ulong;
#include "cxcore.h"
#include "cxmisc.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

// -128.f ... 255.f
extern const float icv8x32fTab[];
#define CV_8TO32F(x)  icv8x32fTab[(x)+128]

extern const char* icvHersheyGlyphs[];

extern const signed char icvDepthToType[];

#define icvIplToCvDepth( depth ) \
    icvDepthToType[(((depth) & 255) >> 2) + ((depth) < 0)]

extern const uchar icvSaturate8u[];
#define CV_FAST_CAST_8U(t)   (assert(-256 <= (t) && (t) <= 512), icvSaturate8u[(t)+256])



CvStatus CV_STDCALL icvLUT_Transform8u_8u_C1R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_16u_C1R( const uchar* src, int srcstep, ushort* dst,
                                                int dststep, CvSize size, const ushort* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_32s_C1R( const uchar* src, int srcstep, int* dst,
                                                int dststep, CvSize size, const int* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_64f_C1R( const uchar* src, int srcstep, double* dst,
                                                int dststep, CvSize size, const double* lut );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C2R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C3R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C4R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );

typedef CvStatus (CV_STDCALL * CvLUT_TransformFunc)( const void* src, int srcstep, void* dst,
                                                     int dststep, CvSize size, const void* lut );

CV_INLINE CvStatus
icvLUT_Transform8u_8s_C1R( const uchar* src, int srcstep, char* dst,
                            int dststep, CvSize size, const char* lut )
{
    return icvLUT_Transform8u_8u_C1R( src, srcstep, (uchar*)dst,
                                      dststep, size, (const uchar*)lut );
}

CV_INLINE CvStatus
icvLUT_Transform8u_16s_C1R( const uchar* src, int srcstep, short* dst,
                            int dststep, CvSize size, const short* lut )
{
    return icvLUT_Transform8u_16u_C1R( src, srcstep, (ushort*)dst,
                                       dststep, size, (const ushort*)lut );
}

CV_INLINE CvStatus
icvLUT_Transform8u_32f_C1R( const uchar* src, int srcstep, float* dst,
                            int dststep, CvSize size, const float* lut )
{
    return icvLUT_Transform8u_32s_C1R( src, srcstep, (int*)dst,
                                       dststep, size, (const int*)lut );
}

#endif /*_CXCORE_INTERNAL_H_*/

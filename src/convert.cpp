
#include "core.precomp.hpp"


/****************************************************************************************\
*                                convertScale[Abs]                                       *
\****************************************************************************************/

namespace cv
{


template<typename T, typename DT, typename WT> static void
cvtScale_( const T* src, size_t sstep,
           DT* dst, size_t dstep, Size size,
           WT scale, WT shift )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
    {
        int x = 0;

        for( ; x < size.width; x++ )
            dst[x] = saturate_cast<DT>(src[x]*scale + shift);
    }
}

//vz optimized template specialization
template<> void
cvtScale_<short, short, float>( const short* src, size_t sstep,
           short* dst, size_t dstep, Size size,
           float scale, float shift )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
    {
        int x = 0;
        for(; x < size.width; x++ )
            dst[x] = saturate_cast<short>(src[x]*scale + shift);
    }
}

template<> void
cvtScale_<short, int, float>( const short* src, size_t sstep,
           int* dst, size_t dstep, Size size,
           float scale, float shift )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
    {
        int x = 0;

        for(; x < size.width; x++ )
            dst[x] = saturate_cast<int>(src[x]*scale + shift);
    }
}

template<typename T, typename DT> static void
cvt_( const T* src, size_t sstep,
      DT* dst, size_t dstep, Size size )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
    {
        int x = 0;
        for( ; x < size.width; x++ )
            dst[x] = saturate_cast<DT>(src[x]);
    }
}

//vz optimized template specialization, test Core_ConvertScale/ElemWiseTest
template<>  void
cvt_<float, short>( const float* src, size_t sstep,
     short* dst, size_t dstep, Size size )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
    {
        int x = 0;
        for( ; x < size.width; x++ )
            dst[x] = saturate_cast<short>(src[x]);
    }

}


template<typename T> static void
cpy_( const T* src, size_t sstep, T* dst, size_t dstep, Size size )
{
    sstep /= sizeof(src[0]);
    dstep /= sizeof(dst[0]);

    for( ; size.height--; src += sstep, dst += dstep )
        memcpy(dst, src, size.width*sizeof(src[0]));
}


#define DEF_CVT_SCALE_FUNC(suffix, stype, dtype, wtype) \
static void cvtScale##suffix( const stype* src, size_t sstep, const uchar*, size_t, \
dtype* dst, size_t dstep, Size size, double* scale) \
{ \
    cvtScale_(src, sstep, dst, dstep, size, (wtype)scale[0], (wtype)scale[1]); \
}


#define DEF_CVT_FUNC(suffix, stype, dtype) \
static void cvt##suffix( const stype* src, size_t sstep, const uchar*, size_t, \
                         dtype* dst, size_t dstep, Size size, double*) \
{ \
    cvt_(src, sstep, dst, dstep, size); \
}

#define DEF_CPY_FUNC(suffix, stype) \
static void cvt##suffix( const stype* src, size_t sstep, const uchar*, size_t, \
stype* dst, size_t dstep, Size size, double*) \
{ \
    cpy_(src, sstep, dst, dstep, size); \
}


DEF_CVT_SCALE_FUNC(8u,     uchar, uchar, float)
DEF_CVT_SCALE_FUNC(8s8u,   schar, uchar, float)
DEF_CVT_SCALE_FUNC(16u8u,  ushort, uchar, float)
DEF_CVT_SCALE_FUNC(16s8u,  short, uchar, float)
DEF_CVT_SCALE_FUNC(32s8u,  int, uchar, float)
DEF_CVT_SCALE_FUNC(32f8u,  float, uchar, float)
DEF_CVT_SCALE_FUNC(64f8u,  double, uchar, float)

DEF_CVT_SCALE_FUNC(8u8s,   uchar, schar, float)
DEF_CVT_SCALE_FUNC(8s,     schar, schar, float)
DEF_CVT_SCALE_FUNC(16u8s,  ushort, schar, float)
DEF_CVT_SCALE_FUNC(16s8s,  short, schar, float)
DEF_CVT_SCALE_FUNC(32s8s,  int, schar, float)
DEF_CVT_SCALE_FUNC(32f8s,  float, schar, float)
DEF_CVT_SCALE_FUNC(64f8s,  double, schar, float)

DEF_CVT_SCALE_FUNC(8u16u,  uchar, ushort, float)
DEF_CVT_SCALE_FUNC(8s16u,  schar, ushort, float)
DEF_CVT_SCALE_FUNC(16u,    ushort, ushort, float)
DEF_CVT_SCALE_FUNC(16s16u, short, ushort, float)
DEF_CVT_SCALE_FUNC(32s16u, int, ushort, float)
DEF_CVT_SCALE_FUNC(32f16u, float, ushort, float)
DEF_CVT_SCALE_FUNC(64f16u, double, ushort, float)

DEF_CVT_SCALE_FUNC(8u16s,  uchar, short, float)
DEF_CVT_SCALE_FUNC(8s16s,  schar, short, float)
DEF_CVT_SCALE_FUNC(16u16s, ushort, short, float)
DEF_CVT_SCALE_FUNC(16s,    short, short, float)
DEF_CVT_SCALE_FUNC(32s16s, int, short, float)
DEF_CVT_SCALE_FUNC(32f16s, float, short, float)
DEF_CVT_SCALE_FUNC(64f16s, double, short, float)

DEF_CVT_SCALE_FUNC(8u32s,  uchar, int, float)
DEF_CVT_SCALE_FUNC(8s32s,  schar, int, float)
DEF_CVT_SCALE_FUNC(16u32s, ushort, int, float)
DEF_CVT_SCALE_FUNC(16s32s, short, int, float)
DEF_CVT_SCALE_FUNC(32s,    int, int, double)
DEF_CVT_SCALE_FUNC(32f32s, float, int, float)
DEF_CVT_SCALE_FUNC(64f32s, double, int, double)

DEF_CVT_SCALE_FUNC(8u32f,  uchar, float, float)
DEF_CVT_SCALE_FUNC(8s32f,  schar, float, float)
DEF_CVT_SCALE_FUNC(16u32f, ushort, float, float)
DEF_CVT_SCALE_FUNC(16s32f, short, float, float)
DEF_CVT_SCALE_FUNC(32s32f, int, float, double)
DEF_CVT_SCALE_FUNC(32f,    float, float, float)
DEF_CVT_SCALE_FUNC(64f32f, double, float, double)

DEF_CVT_SCALE_FUNC(8u64f,  uchar, double, double)
DEF_CVT_SCALE_FUNC(8s64f,  schar, double, double)
DEF_CVT_SCALE_FUNC(16u64f, ushort, double, double)
DEF_CVT_SCALE_FUNC(16s64f, short, double, double)
DEF_CVT_SCALE_FUNC(32s64f, int, double, double)
DEF_CVT_SCALE_FUNC(32f64f, float, double, double)
DEF_CVT_SCALE_FUNC(64f,    double, double, double)

DEF_CPY_FUNC(8u,     uchar)
DEF_CVT_FUNC(8s8u,   schar, uchar)
DEF_CVT_FUNC(16u8u,  ushort, uchar)
DEF_CVT_FUNC(16s8u,  short, uchar)
DEF_CVT_FUNC(32s8u,  int, uchar)
DEF_CVT_FUNC(32f8u,  float, uchar)
DEF_CVT_FUNC(64f8u,  double, uchar)

DEF_CVT_FUNC(8u8s,   uchar, schar)
DEF_CVT_FUNC(16u8s,  ushort, schar)
DEF_CVT_FUNC(16s8s,  short, schar)
DEF_CVT_FUNC(32s8s,  int, schar)
DEF_CVT_FUNC(32f8s,  float, schar)
DEF_CVT_FUNC(64f8s,  double, schar)

DEF_CVT_FUNC(8u16u,  uchar, ushort)
DEF_CVT_FUNC(8s16u,  schar, ushort)
DEF_CPY_FUNC(16u,    ushort)
DEF_CVT_FUNC(16s16u, short, ushort)
DEF_CVT_FUNC(32s16u, int, ushort)
DEF_CVT_FUNC(32f16u, float, ushort)
DEF_CVT_FUNC(64f16u, double, ushort)

DEF_CVT_FUNC(8u16s,  uchar, short)
DEF_CVT_FUNC(8s16s,  schar, short)
DEF_CVT_FUNC(16u16s, ushort, short)
DEF_CVT_FUNC(32s16s, int, short)
DEF_CVT_FUNC(32f16s, float, short)
DEF_CVT_FUNC(64f16s, double, short)

DEF_CVT_FUNC(8u32s,  uchar, int)
DEF_CVT_FUNC(8s32s,  schar, int)
DEF_CVT_FUNC(16u32s, ushort, int)
DEF_CVT_FUNC(16s32s, short, int)
DEF_CPY_FUNC(32s,    int)
DEF_CVT_FUNC(32f32s, float, int)
DEF_CVT_FUNC(64f32s, double, int)

DEF_CVT_FUNC(8u32f,  uchar, float)
DEF_CVT_FUNC(8s32f,  schar, float)
DEF_CVT_FUNC(16u32f, ushort, float)
DEF_CVT_FUNC(16s32f, short, float)
DEF_CVT_FUNC(32s32f, int, float)
DEF_CVT_FUNC(64f32f, double, float)

DEF_CVT_FUNC(8u64f,  uchar, double)
DEF_CVT_FUNC(8s64f,  schar, double)
DEF_CVT_FUNC(16u64f, ushort, double)
DEF_CVT_FUNC(16s64f, short, double)
DEF_CVT_FUNC(32s64f, int, double)
DEF_CVT_FUNC(32f64f, float, double)
DEF_CPY_FUNC(64s,    int64)



BinaryFunc getConvertFunc(int sdepth, int ddepth)
{
    static BinaryFunc cvtTab[][8] =
    {
        {
            (BinaryFunc)(cvt8u), (BinaryFunc)GET_OPTIMIZED(cvt8s8u), (BinaryFunc)GET_OPTIMIZED(cvt16u8u),
            (BinaryFunc)GET_OPTIMIZED(cvt16s8u), (BinaryFunc)GET_OPTIMIZED(cvt32s8u), (BinaryFunc)GET_OPTIMIZED(cvt32f8u),
            (BinaryFunc)GET_OPTIMIZED(cvt64f8u), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u8s), (BinaryFunc)cvt8u, (BinaryFunc)GET_OPTIMIZED(cvt16u8s),
            (BinaryFunc)GET_OPTIMIZED(cvt16s8s), (BinaryFunc)GET_OPTIMIZED(cvt32s8s), (BinaryFunc)GET_OPTIMIZED(cvt32f8s),
            (BinaryFunc)GET_OPTIMIZED(cvt64f8s), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u16u), (BinaryFunc)GET_OPTIMIZED(cvt8s16u), (BinaryFunc)cvt16u,
            (BinaryFunc)GET_OPTIMIZED(cvt16s16u), (BinaryFunc)GET_OPTIMIZED(cvt32s16u), (BinaryFunc)GET_OPTIMIZED(cvt32f16u),
            (BinaryFunc)GET_OPTIMIZED(cvt64f16u), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u16s), (BinaryFunc)GET_OPTIMIZED(cvt8s16s), (BinaryFunc)GET_OPTIMIZED(cvt16u16s),
            (BinaryFunc)cvt16u, (BinaryFunc)GET_OPTIMIZED(cvt32s16s), (BinaryFunc)GET_OPTIMIZED(cvt32f16s),
            (BinaryFunc)GET_OPTIMIZED(cvt64f16s), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u32s), (BinaryFunc)GET_OPTIMIZED(cvt8s32s), (BinaryFunc)GET_OPTIMIZED(cvt16u32s),
            (BinaryFunc)GET_OPTIMIZED(cvt16s32s), (BinaryFunc)cvt32s, (BinaryFunc)GET_OPTIMIZED(cvt32f32s),
            (BinaryFunc)GET_OPTIMIZED(cvt64f32s), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u32f), (BinaryFunc)GET_OPTIMIZED(cvt8s32f), (BinaryFunc)GET_OPTIMIZED(cvt16u32f),
            (BinaryFunc)GET_OPTIMIZED(cvt16s32f), (BinaryFunc)GET_OPTIMIZED(cvt32s32f), (BinaryFunc)cvt32s,
            (BinaryFunc)GET_OPTIMIZED(cvt64f32f), 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvt8u64f), (BinaryFunc)GET_OPTIMIZED(cvt8s64f), (BinaryFunc)GET_OPTIMIZED(cvt16u64f),
            (BinaryFunc)GET_OPTIMIZED(cvt16s64f), (BinaryFunc)GET_OPTIMIZED(cvt32s64f), (BinaryFunc)GET_OPTIMIZED(cvt32f64f),
            (BinaryFunc)(cvt64s), 0
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0
        }
    };

    return cvtTab[CV_MAT_DEPTH(ddepth)][CV_MAT_DEPTH(sdepth)];
}

BinaryFunc getConvertScaleFunc(int sdepth, int ddepth)
{
    static BinaryFunc cvtScaleTab[][8] =
    {
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u), (BinaryFunc)GET_OPTIMIZED(cvtScale8s8u), (BinaryFunc)GET_OPTIMIZED(cvtScale16u8u),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s8u), (BinaryFunc)GET_OPTIMIZED(cvtScale32s8u), (BinaryFunc)GET_OPTIMIZED(cvtScale32f8u),
            (BinaryFunc)cvtScale64f8u, 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u8s), (BinaryFunc)GET_OPTIMIZED(cvtScale8s), (BinaryFunc)GET_OPTIMIZED(cvtScale16u8s),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s8s), (BinaryFunc)GET_OPTIMIZED(cvtScale32s8s), (BinaryFunc)GET_OPTIMIZED(cvtScale32f8s),
            (BinaryFunc)cvtScale64f8s, 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u16u), (BinaryFunc)GET_OPTIMIZED(cvtScale8s16u), (BinaryFunc)GET_OPTIMIZED(cvtScale16u),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s16u), (BinaryFunc)GET_OPTIMIZED(cvtScale32s16u), (BinaryFunc)GET_OPTIMIZED(cvtScale32f16u),
            (BinaryFunc)cvtScale64f16u, 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u16s), (BinaryFunc)GET_OPTIMIZED(cvtScale8s16s), (BinaryFunc)GET_OPTIMIZED(cvtScale16u16s),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s), (BinaryFunc)GET_OPTIMIZED(cvtScale32s16s), (BinaryFunc)GET_OPTIMIZED(cvtScale32f16s),
            (BinaryFunc)cvtScale64f16s, 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u32s), (BinaryFunc)GET_OPTIMIZED(cvtScale8s32s), (BinaryFunc)GET_OPTIMIZED(cvtScale16u32s),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s32s), (BinaryFunc)GET_OPTIMIZED(cvtScale32s), (BinaryFunc)GET_OPTIMIZED(cvtScale32f32s),
            (BinaryFunc)cvtScale64f32s, 0
        },
        {
            (BinaryFunc)GET_OPTIMIZED(cvtScale8u32f), (BinaryFunc)GET_OPTIMIZED(cvtScale8s32f), (BinaryFunc)GET_OPTIMIZED(cvtScale16u32f),
            (BinaryFunc)GET_OPTIMIZED(cvtScale16s32f), (BinaryFunc)GET_OPTIMIZED(cvtScale32s32f), (BinaryFunc)GET_OPTIMIZED(cvtScale32f),
            (BinaryFunc)cvtScale64f32f, 0
        },
        {
            (BinaryFunc)cvtScale8u64f, (BinaryFunc)cvtScale8s64f, (BinaryFunc)cvtScale16u64f,
            (BinaryFunc)cvtScale16s64f, (BinaryFunc)cvtScale32s64f, (BinaryFunc)cvtScale32f64f,
            (BinaryFunc)cvtScale64f, 0
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0
        }
    };

    return cvtScaleTab[CV_MAT_DEPTH(ddepth)][CV_MAT_DEPTH(sdepth)];
}

}


void cv::Mat::convertTo(OutputArray _dst, int _type, double alpha, double beta) const
{
    bool noScale = fabs(alpha-1) < DBL_EPSILON && fabs(beta) < DBL_EPSILON;

    if( _type < 0 )
        _type = _dst.fixedType() ? _dst.type() : type();
    else
        _type = CV_MAKETYPE(CV_MAT_DEPTH(_type), channels());

    int sdepth = depth(), ddepth = CV_MAT_DEPTH(_type);
    if( sdepth == ddepth && noScale )
    {
        copyTo(_dst);
        return;
    }

    Mat src = *this;

    BinaryFunc func = noScale ? getConvertFunc(sdepth, ddepth) : getConvertScaleFunc(sdepth, ddepth);
    double scale[] = {alpha, beta};
    int cn = channels();
    CV_Assert( func != 0 );

    if( dims <= 2 )
    {
        _dst.create( size(), _type );
        Mat dst = _dst.getMat();
        Size sz = getContinuousSize(src, dst, cn);
        func( src.data, src.step, 0, 0, dst.data, dst.step, sz, scale );
    }
    else
    {
        _dst.create( dims, size, _type );
        Mat dst = _dst.getMat();
        const Mat* arrays[] = {&src, &dst, 0};
        uchar* ptrs[2];
        NAryMatIterator it(arrays, ptrs);
        Size sz((int)(it.size*cn), 1);

        for( size_t i = 0; i < it.nplanes; i++, ++it )
            func(ptrs[0], 0, 0, 0, ptrs[1], 0, sz, scale);
    }
}




/* End of file. */

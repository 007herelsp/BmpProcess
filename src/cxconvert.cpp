

#include "_cxcore.h"




#define ICV_FIX_SHIFT  15
#define ICV_SCALE(x)   (((x) + (1 << (ICV_FIX_SHIFT-1))) >> ICV_FIX_SHIFT)

/****************************************************************************************\
*                                      cvConvertScale                                    *
\****************************************************************************************/

#define ICV_DEF_CVT_SCALE_CASE( srctype, worktype,          \
                            scale_macro, cast_macro, a, b ) \
                                                            \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i]+(b));     \
            worktype t1 = scale_macro((a)*_src[i+1]+(b));   \
                                                            \
            dst[i] = cast_macro(t0);                        \
            dst[i+1] = cast_macro(t1);                      \
                                                            \
            t0 = scale_macro((a)*_src[i+2] + (b));          \
            t1 = scale_macro((a)*_src[i+3] + (b));          \
                                                            \
            dst[i+2] = cast_macro(t0);                      \
            dst[i+3] = cast_macro(t1);                      \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i] + (b));   \
            dst[i] = cast_macro(t0);                        \
        }                                                   \
    }                                                       \
}


#define  ICV_DEF_CVT_SCALE_FUNC_INT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
            {                                                           \
                int t = cvRound(val);                                   \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
            {                                                           \
                int t = cvRound( (char)i*scale + shift );               \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( char, int, ICV_SCALE,               \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, int, cvRound,                 \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        if( fabs( scale ) <= 1. && fabs(shift) < DBL_EPSILON )          \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, ICV_SCALE,             \
                                    cast_macro, iscale, 0 );            \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, cvRound,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16S:                                                       \
        if( fabs( scale ) <= 1. &&                                      \
            fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))        \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( short, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( short, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, int, cvRound,                      \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, int, cvRound,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, int, cvRound,                   \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


#define  ICV_DEF_CVT_SCALE_FUNC_FLT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
                lut[i] = (dsttype)val;                                  \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, double, CV_NOP,              \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
                lut[i] = (dsttype)((char)i*scale + shift);              \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, double, CV_NOP,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        ICV_DEF_CVT_SCALE_CASE( ushort, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_16S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( short, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, double, CV_NOP,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_SCALE_FUNC_INT( 8u, uchar, CV_CAST_8U )
ICV_DEF_CVT_SCALE_FUNC_INT( 8s, char, CV_CAST_8S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16s, short, CV_CAST_16S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16u, ushort, CV_CAST_16U )
ICV_DEF_CVT_SCALE_FUNC_INT( 32s, int, CV_CAST_32S )

ICV_DEF_CVT_SCALE_FUNC_FLT( 32f, float, CV_CAST_32F )
ICV_DEF_CVT_SCALE_FUNC_FLT( 64f, double, CV_CAST_64F )

CV_DEF_INIT_FUNC_TAB_2D( CvtScaleTo, C1R )


/****************************************************************************************\
*                             Conversion w/o scaling macros                              *
\****************************************************************************************/

#define ICV_DEF_CVT_CASE_2D( srctype, worktype,             \
                             cast_macro1, cast_macro2 )     \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        int i;                                              \
                                                            \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            worktype t1 = cast_macro1(_src[i+1]);           \
                                                            \
            dst[i] = cast_macro2(t0);                       \
            dst[i+1] = cast_macro2(t1);                     \
                                                            \
            t0 = cast_macro1(_src[i+2]);                    \
            t1 = cast_macro1(_src[i+3]);                    \
                                                            \
            dst[i+2] = cast_macro2(t0);                     \
            dst[i+3] = cast_macro2(t1);                     \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            dst[i] = cast_macro2(t0);                       \
        }                                                   \
    }                                                       \
}


#define ICV_DEF_CVT_FUNC_2D( flavor, dsttype, worktype, cast_macro2,    \
                             srcdepth1, srctype1, cast_macro11,         \
                             srcdepth2, srctype2, cast_macro12,         \
                             srcdepth3, srctype3, cast_macro13,         \
                             srcdepth4, srctype4, cast_macro14,         \
                             srcdepth5, srctype5, cast_macro15,         \
                             srcdepth6, srctype6, cast_macro16 )        \
static CvStatus CV_STDCALL                                              \
icvCvtTo_##flavor##_C1R( const uchar* src, int srcstep,                 \
                         dsttype* dst, int dststep,                     \
                         CvSize size, int param )                       \
{                                                                       \
    int srctype = param;                                                \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case srcdepth1:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype1, worktype,                        \
                             cast_macro11, cast_macro2 );               \
        break;                                                          \
    case srcdepth2:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype2, worktype,                        \
                             cast_macro12, cast_macro2 );               \
        break;                                                          \
    case srcdepth3:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype3, worktype,                        \
                             cast_macro13, cast_macro2 );               \
        break;                                                          \
    case srcdepth4:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype4, worktype,                        \
                             cast_macro14, cast_macro2 );               \
        break;                                                          \
    case srcdepth5:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype5, worktype,                        \
                             cast_macro15, cast_macro2 );               \
        break;                                                          \
    case srcdepth6:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype6, worktype,                        \
                             cast_macro16, cast_macro2 );               \
        break;                                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_FUNC_2D( 8u, uchar, int, CV_CAST_8U,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 8s, char, int, CV_CAST_8S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16u, ushort, int, CV_CAST_16U,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16s, short, int, CV_CAST_16S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32s, int, int, CV_NOP,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32f, float, float, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_CAST_32F,
                     CV_64F, double, CV_CAST_32F )

ICV_DEF_CVT_FUNC_2D( 64f, double, double, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  CV_NOP )

CV_DEF_INIT_FUNC_TAB_2D( CvtTo, C1R )


typedef  CvStatus (CV_STDCALL *CvCvtFunc)( const void* src, int srcstep,
                                           void* dst, int dststep, CvSize size,
                                           int param );

typedef  CvStatus (CV_STDCALL *CvCvtScaleFunc)( const void* src, int srcstep,
                                             void* dst, int dststep, CvSize size,
                                             double scale, double shift,
                                             int param );

CV_IMPL void
cvConvertScale( const void* srcarr, void* dstarr,
                double scale, double shift )
{
    static CvFuncTable cvt_tab, cvtscale_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvConvertScale" );

    __BEGIN__;

    int type;
    int is_nd = 0;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int no_scale = scale == 1 && shift == 0;

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            int coi = 0;
            CV_CALL( src = cvGetMat( src, &srcstub, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;
        int dsttype;

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator, CV_NO_DEPTH_CHECK ));

        type = iterator.hdr[0]->type;
        dsttype = iterator.hdr[1]->type;
        iterator.size.width *= CV_MAT_CN(type);

        if( !inittab )
        {
            icvInitCvtToC1RTable( &cvt_tab );
            icvInitCvtScaleToC1RTable( &cvtscale_tab );
            inittab = 1;
        }

        if( no_scale )
        {
            CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        else
        {
            CvCvtScaleFunc func =
                (CvCvtScaleFunc)(cvtscale_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, scale, shift, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        EXIT;
    }

    if( no_scale && CV_ARE_TYPES_EQ( src, dst ) )
    {
        if( src != dst )
          cvCopy( src, dst );
        EXIT;
    }

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( src );
    type = CV_MAT_TYPE(src->type);
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    size.width *= CV_MAT_CN( type );

    if( CV_ARE_TYPES_EQ( src, dst ) && size.height == 1 &&
        size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
    {
        if( CV_MAT_DEPTH(type) == CV_32F )
        {
            const float* srcdata = (const float*)(src->data.ptr);
            float* dstdata = (float*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = (float)(srcdata[size.width-1]*scale + shift);
            }
            while( --size.width );

            EXIT;
        }

        if( CV_MAT_DEPTH(type) == CV_64F )
        {
            const double* srcdata = (const double*)(src->data.ptr);
            double* dstdata = (double*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = srcdata[size.width-1]*scale + shift;
            }
            while( --size.width );

            EXIT;
        }
    }

    if( !inittab )
    {
        icvInitCvtToC1RTable( &cvt_tab );
        icvInitCvtScaleToC1RTable( &cvtscale_tab );
        inittab = 1;
    }

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( no_scale )
    {
        CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size, type ));
    }
    else
    {
        CvCvtScaleFunc func = (CvCvtScaleFunc)
            (cvtscale_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size,
                   scale, shift, type ));
    }

    __END__;
}

/********************* helper functions for converting 32f<->64f ************************/

IPCVAPI_IMPL( CvStatus, icvCvt_32f64f,
    ( const float* src, double* dst, int len ), (src, dst, len) )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i];
        double t1 = src[i+1];

        dst[i] = t0;
        dst[i+1] = t1;

        t0 = src[i+2];
        t1 = src[i+3];

        dst[i+2] = t0;
        dst[i+3] = t1;
    }

    for( ; i < len; i++ )
        dst[i] = src[i];

    return CV_OK;
}


IPCVAPI_IMPL( CvStatus, icvCvt_64f32f,
    ( const double* src, float* dst, int len ), (src, dst, len) )
{
    int i = 0;
    for( ; i <= len - 4; i += 4 )
    {
        double t0 = src[i];
        double t1 = src[i+1];

        dst[i] = (float)t0;
        dst[i+1] = (float)t1;

        t0 = src[i+2];
        t1 = src[i+3];

        dst[i+2] = (float)t0;
        dst[i+3] = (float)t1;
    }

    for( ; i < len; i++ )
        dst[i] = (float)src[i];

    return CV_OK;
}


CvStatus CV_STDCALL icvScale_32f( const float* src, float* dst, int len, float a, float b )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i]*a + b;
        double t1 = src[i+1]*a + b;

        dst[i] = (float)t0;
        dst[i+1] = (float)t1;

        t0 = src[i+2]*a + b;
        t1 = src[i+3]*a + b;

        dst[i+2] = (float)t0;
        dst[i+3] = (float)t1;
    }

    for( ; i < len; i++ )
        dst[i] = (float)(src[i]*a + b);

    return CV_OK;
}


CvStatus CV_STDCALL icvScale_64f( const double* src, double* dst, int len, double a, double b )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i]*a + b;
        double t1 = src[i+1]*a + b;

        dst[i] = t0;
        dst[i+1] = t1;

        t0 = src[i+2]*a + b;
        t1 = src[i+3]*a + b;

        dst[i+2] = t0;
        dst[i+3] = t1;
    }

    for( ; i < len; i++ )
        dst[i] = src[i]*a + b;

    return CV_OK;
}

/* End of file. */

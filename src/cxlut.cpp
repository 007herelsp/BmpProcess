

#include "_cxcore.h"

/****************************************************************************************\
*                                    LUT Transform                                       *
\****************************************************************************************/

#define ICV_LUT_CASE_C1( type )             \
    for( i = 0; i <= size.width-4; i += 4 ) \
    {                                       \
        type t0 = lut[src[i]];              \
        type t1 = lut[src[i+1]];            \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
                                            \
        t0 = lut[src[i+2]];                 \
        t1 = lut[src[i+3]];                 \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        type t0 = lut[src[i]];              \
        dst[i] = t0;                        \
    }


#define ICV_LUT_CASE_C2( type )             \
    for( i = 0; i < size.width; i += 2 )    \
    {                                       \
        type t0 = lut[src[i]*2];            \
        type t1 = lut[src[i+1]*2 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
    }

#define ICV_LUT_CASE_C3( type )             \
    for( i = 0; i < size.width; i += 3 )    \
    {                                       \
        type t0 = lut[src[i]*3];            \
        type t1 = lut[src[i+1]*3 + 1];      \
        type t2 = lut[src[i+2]*3 + 2];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        dst[i+2] = t2;                      \
    }

#define ICV_LUT_CASE_C4( type )             \
    for( i = 0; i < size.width; i += 4 )    \
    {                                       \
        type t0 = lut[src[i]*4];            \
        type t1 = lut[src[i+1]*4 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        t0 = lut[src[i+2]*4 + 2];           \
        t1 = lut[src[i+3]*4 + 3];           \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }


#define  ICV_DEF_LUT_FUNC_8U_CN( flavor, dsttype, cn )      \
CvStatus CV_STDCALL icvLUT_Transform8u_##flavor##_C##cn##R( \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* lut )                                    \
{                                                           \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        int i;                                              \
        ICV_LUT_CASE_C##cn( dsttype )                       \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 16u, ushort, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 32s, int, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 64f, double, 1 )

ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 2 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 3 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 4 )


#define  ICV_DEF_LUT_FUNC_8U( flavor, dsttype )             \
static CvStatus CV_STDCALL                                  \
icvLUT_Transform8u_##flavor##_CnR(                          \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* _lut, int cn )                           \
{                                                           \
    int max_block_size = (1 << 10)*cn;                      \
    dsttype lutp[1024];                                     \
    int i, k;                                               \
                                                            \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
                                                            \
    if( size.width*size.height < 256 )                      \
    {                                                       \
        for( ; size.height--; src+=srcstep, dst+=dststep )  \
            for( k = 0; k < cn; k++ )                       \
                for( i = 0; i < size.width; i += cn )       \
                    dst[i+k] = _lut[src[i+k]*cn+k];         \
        return CV_OK;                                       \
    }                                                       \
                                                            \
    /* repack the lut to planar layout */                   \
    for( k = 0; k < cn; k++ )                               \
        for( i = 0; i < 256; i++ )                          \
            lutp[i+k*256] = _lut[i*cn+k];                   \
                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        for( i = 0; i < size.width; )                       \
        {                                                   \
            int j, limit = MIN(size.width,i+max_block_size);\
            for( k=0; k<cn; k++, src++, dst++ )             \
            {                                               \
                const dsttype* lut = lutp + k*256;          \
                for( j = i; j <= limit - cn*2; j += cn*2 )  \
                {                                           \
                    dsttype t0 = lut[src[j]];               \
                    dsttype t1 = lut[src[j+cn]];            \
                    dst[j] = t0; dst[j+cn] = t1;            \
                }                                           \
                                                            \
                for( ; j < limit; j += cn )                 \
                    dst[j] = lut[src[j]];                   \
            }                                               \
            src -= cn;                                      \
            dst -= cn;                                      \
            i += limit;                                     \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}

ICV_DEF_LUT_FUNC_8U( 8u, uchar )
ICV_DEF_LUT_FUNC_8U( 16u, ushort )
ICV_DEF_LUT_FUNC_8U( 32s, int )
ICV_DEF_LUT_FUNC_8U( 64f, double )

#undef   icvLUT_Transform8u_8s_C1R
#undef   icvLUT_Transform8u_16s_C1R
#undef   icvLUT_Transform8u_32f_C1R

#define  icvLUT_Transform8u_8s_C1R    icvLUT_Transform8u_8u_C1R
#define  icvLUT_Transform8u_16s_C1R   icvLUT_Transform8u_16u_C1R
#define  icvLUT_Transform8u_32f_C1R   icvLUT_Transform8u_32s_C1R

#define  icvLUT_Transform8u_8s_CnR    icvLUT_Transform8u_8u_CnR
#define  icvLUT_Transform8u_16s_CnR   icvLUT_Transform8u_16u_CnR
#define  icvLUT_Transform8u_32f_CnR   icvLUT_Transform8u_32s_CnR

CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, C1R )
CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, CnR )

/* End of file. */

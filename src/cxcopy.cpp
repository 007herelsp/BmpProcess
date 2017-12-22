

/* ////////////////////////////////////////////////////////////////////
//
//  CvMat basic operations: cvCopy, cvSet
//
// */

#include "_cxcore.h"

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                  L/L COPY & SET FUNCTIONS                           //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


IPCVAPI_IMPL( CvStatus, icvCopy_8u_C1R, ( const uchar* src, int srcstep,
                                          uchar* dst, int dststep, CvSize size ),
                                          (src, srcstep, dst, dststep, size) )
{
    for( ; size.height--; src += srcstep, dst += dststep )
        memcpy( dst, src, size.width );

    return  CV_OK;
}


static CvStatus CV_STDCALL
icvSet_8u_C1R( uchar* dst, int dst_step, CvSize size,
               const void* scalar, int pix_size )
{
    int copy_len = 12*pix_size;
    uchar* dst_limit = dst + size.width;
    
    if( size.height-- )
    {
        while( dst + copy_len <= dst_limit )
        {
            memcpy( dst, scalar, copy_len );
            dst += copy_len;
        }

        memcpy( dst, scalar, dst_limit - dst );
    }

    if( size.height )
    {
        dst = dst_limit - size.width + dst_step;

        for( ; size.height--; dst += dst_step )
            memcpy( dst, dst - dst_step, size.width );
    }

    return CV_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                L/L COPY WITH MASK FUNCTIONS                         //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


#define ICV_DEF_COPY_MASK_C1_CASE( type )   \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
        if( mask[i+1] )                     \
            dst[i+1] = src[i+1];            \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
    }

#define ICV_DEF_COPY_MASK_C3_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*3];             \
            type t1 = src[i*3+1];           \
            type t2 = src[i*3+2];           \
                                            \
            dst[i*3] = t0;                  \
            dst[i*3+1] = t1;                \
            dst[i*3+2] = t2;                \
        }



#define ICV_DEF_COPY_MASK_C4_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*4];             \
            type t1 = src[i*4+1];           \
            dst[i*4] = t0;                  \
            dst[i*4+1] = t1;                \
                                            \
            t0 = src[i*4+2];                \
            t1 = src[i*4+3];                \
            dst[i*4+2] = t0;                \
            dst[i*4+3] = t1;                \
        }


#define ICV_DEF_COPY_MASK_2D( name, type, cn )              \
IPCVAPI_IMPL( CvStatus,                                     \
name,( const type* src, int srcstep, type* dst, int dststep,\
       CvSize size, const uchar* mask, int maskstep ),      \
       (src, srcstep, dst, dststep, size, mask, maskstep))  \
{                                                           \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
    for( ; size.height--; src += srcstep,                   \
            dst += dststep, mask += maskstep )              \
    {                                                       \
        int i;                                              \
        ICV_DEF_COPY_MASK_C##cn##_CASE( type )              \
    }                                                       \
                                                            \
    return  CV_OK;                                          \
}


#define ICV_DEF_SET_MASK_C1_CASE( type )    \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
        if( mask[i+1] )                     \
            dst[i+1] = s0;                  \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
    }


#define ICV_DEF_SET_MASK_C3_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*3] = s0;                  \
            dst[i*3+1] = s1;                \
            dst[i*3+2] = s2;                \
        }

#define ICV_DEF_SET_MASK_C4_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*4] = s0;                  \
            dst[i*4+1] = s1;                \
            dst[i*4+2] = s2;                \
            dst[i*4+3] = s3;                \
        }

#define ICV_DEF_SET_MASK_2D( name, type, cn )       \
IPCVAPI_IMPL( CvStatus,                             \
name,( type* dst, int dststep,                      \
       const uchar* mask, int maskstep,             \
       CvSize size, const type* scalar ),           \
       (dst, dststep, mask, maskstep, size, scalar))\
{                                                   \
    CV_UN_ENTRY_C##cn( type );                      \
    dststep /= sizeof(dst[0]);                      \
                                                    \
    for( ; size.height--; mask += maskstep,         \
                          dst += dststep )          \
    {                                               \
        int i;                                      \
        ICV_DEF_SET_MASK_C##cn##_CASE( type )       \
    }                                               \
                                                    \
    return CV_OK;                                   \
}


ICV_DEF_SET_MASK_2D( icvSet_8u_C1MR, uchar, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C1MR, ushort, 1 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C3MR, uchar, 3 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C4MR, int, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C3MR, ushort, 3 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C4MR, int64, 1 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C3MR, int, 3 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C4MR, int, 4 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C3MR, int64, 3 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C4MR, int64, 4 )

ICV_DEF_COPY_MASK_2D( icvCopy_8u_C1MR, uchar, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C1MR, ushort, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C3MR, uchar, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C4MR, int, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C3MR, ushort, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C4MR, int64, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C3MR, int, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C4MR, int, 4 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C3MR, int64, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C4MR, int64, 4 )

#define CV_DEF_INIT_COPYSET_TAB_2D( FUNCNAME, FLAG )                \
static void icvInit##FUNCNAME##FLAG##Table( CvBtFuncTable* table )  \
{                                                                   \
    table->fn_2d[1]  = (void*)icv##FUNCNAME##_8u_C1##FLAG;          \
    table->fn_2d[2]  = (void*)icv##FUNCNAME##_16s_C1##FLAG;         \
    table->fn_2d[3]  = (void*)icv##FUNCNAME##_8u_C3##FLAG;          \
    table->fn_2d[4]  = (void*)icv##FUNCNAME##_8u_C4##FLAG;          \
    table->fn_2d[6]  = (void*)icv##FUNCNAME##_16s_C3##FLAG;         \
    table->fn_2d[8]  = (void*)icv##FUNCNAME##_16s_C4##FLAG;         \
    table->fn_2d[12] = (void*)icv##FUNCNAME##_32f_C3##FLAG;         \
    table->fn_2d[16] = (void*)icv##FUNCNAME##_32f_C4##FLAG;         \
    table->fn_2d[24] = (void*)icv##FUNCNAME##_64s_C3##FLAG;         \
    table->fn_2d[32] = (void*)icv##FUNCNAME##_64s_C4##FLAG;         \
}

CV_DEF_INIT_COPYSET_TAB_2D( Set, MR )
CV_DEF_INIT_COPYSET_TAB_2D( Copy, MR )

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                H/L COPY & SET FUNCTIONS                             //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


CvCopyMaskFunc
icvGetCopyMaskFunc( int elem_size )
{
    static CvBtFuncTable copym_tab;
    static int inittab = 0;

    if( !inittab )
    {
        icvInitCopyMRTable( &copym_tab );
        inittab = 1;
    }
    return (CvCopyMaskFunc)copym_tab.fn_2d[elem_size];
}


/* dst = src */
CV_IMPL void
cvCopy( const void* srcarr, void* dstarr, const void* maskarr )
{
    CV_FUNCNAME( "cvCopy" );
    
    __BEGIN__;

    int pix_size;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    assert("herelsp remove" && (CV_IS_MAT(src) && CV_IS_MAT(dst)));

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE(src->type);


    if( !maskarr )
    {
        int src_step = src->step, dst_step = dst->step;
        size.width *= pix_size;
        if( CV_IS_MAT_CONT( src->type & dst->type ) && (src_step == dst_step) && (src_step == src->width * pix_size))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*
                              CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
            {
                memcpy( dst->data.ptr, src->data.ptr, size.width );
                EXIT;
            }

            size.height = 1;
            src_step = dst_step = CV_STUB_STEP;
        }

        if( src->data.ptr != dst->data.ptr )
            icvCopy_8u_C1R( src->data.ptr, src_step,
                            dst->data.ptr, dst_step, size );
    }
    else
    {
        CV_ERROR( CV_StsBadMask, "not supported" );
    }

    __END__;
}



/* End of file. */


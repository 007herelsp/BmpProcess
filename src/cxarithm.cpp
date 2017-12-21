

/* ////////////////////////////////////////////////////////////////////
//
//  CvMat arithmetic operations: +, - ...
//
// */

#include "_cxcore.h"

/****************************************************************************************\
*                      Arithmetic operations (+, -) without mask                         *
\****************************************************************************************/

#define ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype, cast_macro, len )\
{                                                                   \
    int i;                                                          \
                                                                    \
    for( i = 0; i <= (len) - 4; i += 4 )                            \
    {                                                               \
        worktype t0 = __op__((src1)[i], (src2)[i]);                 \
        worktype t1 = __op__((src1)[i+1], (src2)[i+1]);             \
                                                                    \
        (dst)[i] = cast_macro( t0 );                                \
        (dst)[i+1] = cast_macro( t1 );                              \
                                                                    \
        t0 = __op__((src1)[i+2],(src2)[i+2]);                       \
        t1 = __op__((src1)[i+3],(src2)[i+3]);                       \
                                                                    \
        (dst)[i+2] = cast_macro( t0 );                              \
        (dst)[i+3] = cast_macro( t1 );                              \
    }                                                               \
                                                                    \
    for( ; i < (len); i++ )                                         \
    {                                                               \
        worktype t0 = __op__((src1)[i],(src2)[i]);                  \
        (dst)[i] = cast_macro( t0 );                                \
    }                                                               \
}

#define ICV_DEF_BIN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )   \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size ),                                   \
      (src1, step1, src2, step2, dst, step, size) )                         \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_OP_2D_SFS(__op__, name, type, worktype, cast_macro) \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size, int /*scalefactor*/ ),              \
      (src1, step1, src2, step2, dst, step, size, 0) )                      \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,               \
                                src, scalar, dst, len )                     \
{                                                                           \
    int i;                                                                  \
                                                                            \
    for( ; ((len) -= 12) >= 0; (dst) += 12, (src) += 12 )                   \
    {                                                                       \
        worktype t0 = __op__((scalar)[0], (src)[0]);                        \
        worktype t1 = __op__((scalar)[1], (src)[1]);                        \
                                                                            \
        (dst)[0] = cast_macro( t0 );                                        \
        (dst)[1] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[2], (src)[2]);                                 \
        t1 = __op__((scalar)[3], (src)[3]);                                 \
                                                                            \
        (dst)[2] = cast_macro( t0 );                                        \
        (dst)[3] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[4], (src)[4]);                                 \
        t1 = __op__((scalar)[5], (src)[5]);                                 \
                                                                            \
        (dst)[4] = cast_macro( t0 );                                        \
        (dst)[5] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[6], (src)[6]);                                 \
        t1 = __op__((scalar)[7], (src)[7]);                                 \
                                                                            \
        (dst)[6] = cast_macro( t0 );                                        \
        (dst)[7] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[8], (src)[8]);                                 \
        t1 = __op__((scalar)[9], (src)[9]);                                 \
                                                                            \
        (dst)[8] = cast_macro( t0 );                                        \
        (dst)[9] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[10], (src)[10]);                               \
        t1 = __op__((scalar)[11], (src)[11]);                               \
                                                                            \
        (dst)[10] = cast_macro( t0 );                                       \
        (dst)[11] = cast_macro( t1 );                                       \
    }                                                                       \
                                                                            \
    for( (len) += 12, i = 0; i < (len); i++ )                               \
    {                                                                       \
        worktype t0 = __op__((scalar)[i],(src)[i]);                         \
        (dst)[i] = cast_macro( t0 );                                        \
    }                                                                       \
}


#define ICV_DEF_UN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )    \
static CvStatus CV_STDCALL name                                             \
    ( const type* src, int step1, type* dst, int step,                      \
      CvSize size, const worktype* scalar )                                 \
{                                                                           \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                        \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            worktype t0 = __op__(*(scalar),*(src));                         \
            *(dst) = cast_macro( t0 );                                      \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            const type *tsrc = src;                                         \
            type *tdst = dst;                                               \
            int width = size.width;                                         \
                                                                            \
            ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,           \
                                    tsrc, scalar, tdst, width );            \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_ALL( __op__, name, cast_8u )                                \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_8u_C1R, uchar, int, cast_8u )        \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )  \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )   \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )         \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )     \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#define ICV_DEF_UN_ARI_ALL( __op__, name )                                          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_8u_C1R, uchar, int, CV_CAST_8U )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )       \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )        \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )      \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#undef CV_SUB_R
#define CV_SUB_R(a,b) ((b) - (a))

ICV_DEF_BIN_ARI_ALL( CV_ADD, Add, CV_FAST_CAST_8U )
ICV_DEF_BIN_ARI_ALL( CV_SUB_R, Sub, CV_FAST_CAST_8U )

ICV_DEF_UN_ARI_ALL( CV_ADD, AddC )
ICV_DEF_UN_ARI_ALL( CV_SUB, SubRC )

#define ICV_DEF_INIT_ARITHM_FUNC_TAB( FUNCNAME, FLAG )          \
static  void  icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )\
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_ARITHM_FUNC_TAB( Sub, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( SubRC, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Add, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( AddC, C1R )

/****************************************************************************************\
*                       External Functions for Arithmetic Operations                     *
\****************************************************************************************/

/*************************************** S U B ******************************************/

CV_IMPL void
cvSub( const void* srcarr1, const void* srcarr2,
       void* dstarr, const void* maskarr )
{
    static CvFuncTable sub_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSub" );

    __BEGIN__;

    const CvArr* tmp;
    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, srcstub2, *src1, *src2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    CV_SWAP( srcarr1, srcarr2, tmp ); // to comply with IPP
    src1 = (CvMat*)srcarr1;
    src2 = (CvMat*)srcarr2;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitSubC1RTable( &sub_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src2data[len-1] - src1data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src2data[len-1] - src1data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitSubC1RTable( &sub_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


CV_IMPL void
cvSubRS( const void* srcarr, CvScalar scalar, void* dstarr, const void* maskarr )
{
    static CvFuncTable subr_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSubRS" );

    __BEGIN__;

    int sctype, y, dy, type, depth, cn, coi = 0, cont_flag = 0;
    int src_step, dst_step, tdst_step, mask_step;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_2A1P func;
    CvCopyMaskFunc copym_func;
    double buf[12];
    int is_nd = 0;
    CvSize size, tsize; 

    if( !inittab )
    {
        icvInitSubRCC1RTable( &subr_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
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

        if( maskarr )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator ));

        sctype = type = CV_MAT_TYPE(iterator.hdr[0]->type);
        if( CV_MAT_DEPTH(sctype) < CV_32S )
            sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvFunc2D_2A1P)(subr_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
       
        CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.size, buf ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( depth < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src );

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( src->type & dst->type ))
        {
            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
            {
                int len = size.width * size.height;

                if( type == CV_32FC1 )
                {
                    const float* srcdata = (const float*)(src->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = (float)(scalar.val[0] - srcdata[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    const double* srcdata = (const double*)(src->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = scalar.val[0] - srcdata[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }
        
        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    func = (CvFunc2D_2A1P)(subr_tab.fn_2d[depth]);
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( func( src->data.ptr + y*src->step, src_step,
                         tdst->data.ptr, tdst_step,
                         cvSize( tsize.width*cn, tsize.height ), buf ));
        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


/******************************* A D D ********************************/



/* End of file. */

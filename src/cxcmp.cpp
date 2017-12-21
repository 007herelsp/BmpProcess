

/* ////////////////////////////////////////////////////////////////////
//
//  CvMat comparison functions: range checking, min, max
//
// */

#include "_cxcore.h"

/****************************************************************************************\
*                                         Cmp                                            *
\****************************************************************************************/

#define ICV_DEF_CMP_CASE_C1( __op__, _toggle_macro_ )                   \
for( x = 0; x <= size.width - 4; x += 4 )                               \
{                                                                       \
    int f0 = __op__( _toggle_macro_(src1[x]), _toggle_macro_(src2[x])); \
    int f1 = __op__( _toggle_macro_(src1[x+1]), _toggle_macro_(src2[x+1])); \
    dst[x] = (uchar)-f0;                                                \
    dst[x+1] = (uchar)-f1;                                              \
    f0 = __op__( _toggle_macro_(src1[x+2]), _toggle_macro_(src2[x+2])); \
    f1 = __op__( _toggle_macro_(src1[x+3]), _toggle_macro_(src2[x+3])); \
    dst[x+2] = (uchar)-f0;                                              \
    dst[x+3] = (uchar)-f1;                                              \
}                                                                       \
                                                                        \
for( ; x < size.width; x++ )                                            \
{                                                                       \
    int f0 = __op__( _toggle_macro_(src1[x]), _toggle_macro_(src2[x])); \
    dst[x] = (uchar)-f0;                                                \
}


#define ICV_DEF_CMP_FUNC( __op__, name, flavor, arrtype,        \
                          worktype, _toggle_macro_ )            \
static CvStatus CV_STDCALL                                      \
icv##name##_##flavor##_C1R( const arrtype* src1, int step1,     \
                            const arrtype* src2, int step2,     \
                            uchar* dst, int step, CvSize size ) \
{                                                               \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);         \
    step /= sizeof(dst[0]);                                     \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          dst += step )                         \
    {                                                           \
        int x;                                                  \
        ICV_DEF_CMP_CASE_C1( __op__, _toggle_macro_ )           \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


#define ICV_DEF_CMP_CONST_CASE_C1( __op__, _toggle_macro_ )     \
for( x = 0; x <= size.width - 4; x += 4 )                       \
{                                                               \
    int f0 = __op__( _toggle_macro_(src1[x]), scalar );         \
    int f1 = __op__( _toggle_macro_(src1[x+1]), scalar );       \
    dst[x] = (uchar)-f0;                                        \
    dst[x+1] = (uchar)-f1;                                      \
    f0 = __op__( _toggle_macro_(src1[x+2]), scalar );           \
    f1 = __op__( _toggle_macro_(src1[x+3]), scalar );           \
    dst[x+2] = (uchar)-f0;                                      \
    dst[x+3] = (uchar)-f1;                                      \
}                                                               \
                                                                \
for( ; x < size.width; x++ )                                    \
{                                                               \
    int f0 = __op__( _toggle_macro_(src1[x]), scalar );         \
    dst[x] = (uchar)-f0;                                        \
}


#define ICV_DEF_CMP_CONST_FUNC( __op__, name, flavor, arrtype,  \
                                worktype, _toggle_macro_)       \
static CvStatus CV_STDCALL                                      \
icv##name##C_##flavor##_C1R( const arrtype* src1, int step1,    \
                             uchar* dst, int step,              \
                             CvSize size, worktype* pScalar )   \
{                                                               \
    worktype scalar = *pScalar;                                 \
    step1 /= sizeof(src1[0]); step /= sizeof(dst[0]);           \
                                                                \
    for( ; size.height--; src1 += step1, dst += step )          \
    {                                                           \
        int x;                                                  \
        ICV_DEF_CMP_CONST_CASE_C1( __op__, _toggle_macro_ )     \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


#define ICV_DEF_CMP_ALL( flavor, arrtype, worktype, _toggle_macro_ )            \
ICV_DEF_CMP_FUNC( CV_GT, CmpGT, flavor, arrtype, worktype, _toggle_macro_ )     \
ICV_DEF_CMP_FUNC( CV_EQ, CmpEQ, flavor, arrtype, worktype, _toggle_macro_ )     \
ICV_DEF_CMP_CONST_FUNC( CV_GT, CmpGT, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_CMP_CONST_FUNC( CV_GE, CmpGE, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_CMP_CONST_FUNC( CV_EQ, CmpEQ, flavor, arrtype, worktype, _toggle_macro_)

ICV_DEF_CMP_ALL( 8u, uchar, int, CV_NOP )
ICV_DEF_CMP_ALL( 16u, ushort, int, CV_NOP )
ICV_DEF_CMP_ALL( 16s, short, int, CV_NOP )
ICV_DEF_CMP_ALL( 32s, int, int, CV_NOP )
ICV_DEF_CMP_ALL( 32f, float, double, CV_NOP )
ICV_DEF_CMP_ALL( 64f, double, double, CV_NOP )

#define icvCmpGT_8s_C1R     0
#define icvCmpEQ_8s_C1R     0
#define icvCmpGTC_8s_C1R    0
#define icvCmpGEC_8s_C1R    0
#define icvCmpEQC_8s_C1R    0

CV_DEF_INIT_FUNC_TAB_2D( CmpGT, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpEQ, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpGTC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpGEC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpEQC, C1R )

icvCompare_8u_C1R_t icvCompare_8u_C1R_p = 0;
icvCompare_16s_C1R_t icvCompare_16s_C1R_p = 0;
icvCompare_32f_C1R_t icvCompare_32f_C1R_p = 0;

icvCompareC_8u_C1R_t icvCompareC_8u_C1R_p = 0;
icvCompareC_16s_C1R_t icvCompareC_16s_C1R_p = 0;
icvCompareC_32f_C1R_t icvCompareC_32f_C1R_p = 0;

icvThreshold_GT_8u_C1R_t icvThreshold_GT_8u_C1R_p = 0;
icvThreshold_GT_16s_C1R_t icvThreshold_GT_16s_C1R_p = 0;
icvThreshold_GT_32f_C1R_t icvThreshold_GT_32f_C1R_p = 0;

icvThreshold_LT_8u_C1R_t icvThreshold_LT_8u_C1R_p = 0;
icvThreshold_LT_16s_C1R_t icvThreshold_LT_16s_C1R_p = 0;
icvThreshold_LT_32f_C1R_t icvThreshold_LT_32f_C1R_p = 0;

/***************************************** cvCmp ****************************************/


/*************************************** cvCmpS *****************************************/

CV_IMPL void
cvCmpS( const void* srcarr, double value, void* dstarr, int cmp_op )
{
    static CvFuncTable cmps_tab[3];
    static int inittab = 0;

    CV_FUNCNAME( "cvCmpS" );

    __BEGIN__;

    int y, type, coi = 0;
    int invflag = 0, ipp_cmp_op;
    int src1_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    int ival = 0;

    if( !inittab )
    {
        icvInitCmpEQCC1RTable( &cmps_tab[CV_CMP_EQ] );
        icvInitCmpGTCC1RTable( &cmps_tab[CV_CMP_GT] );
        icvInitCmpGECC1RTable( &cmps_tab[CV_CMP_GE] );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    switch( cmp_op )
    {
    case CV_CMP_GT:
    case CV_CMP_EQ:
    case CV_CMP_GE:
        break;
    case CV_CMP_LT:
        invflag = 1;
        cmp_op = CV_CMP_GE;
        break;
    case CV_CMP_LE:
        invflag = 1;
        cmp_op = CV_CMP_GT;
        break;
    case CV_CMP_NE:
        invflag = 1;
        cmp_op = CV_CMP_EQ;
        break;
    default:
        CV_ERROR( CV_StsBadArg, "Unknown comparison operation" );
    }

    if( !CV_IS_MASK_ARR( dst ))
        CV_ERROR( CV_StsUnsupportedFormat, "Destination array should be 8uC1 or 8sC1");

    if( CV_MAT_CN( src1->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input array must be single-channel");

    if( !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        dst_step = dst->step;
    }

    if( CV_MAT_DEPTH(type) <= CV_32S )
    {
        ival = cvRound(value);
        if( type == CV_8U || type == CV_16S )
        {
            int minval = type == CV_8U ? 0 : -32768;
            int maxval = type == CV_8U ? 255 : 32767;
            int fillval = -1;
            if( ival < minval )
                fillval = cmp_op == CV_CMP_NE || cmp_op == CV_CMP_GE || cmp_op == CV_CMP_GT ? 255 : 0;
            else if( ival > maxval )
                fillval = cmp_op == CV_CMP_NE || cmp_op == CV_CMP_LE || cmp_op == CV_CMP_LT ? 255 : 0;
            if( fillval >= 0 )
            {
                fillval ^= invflag ? 255 : 0;
                for( y = 0; y < size.height; y++ )
                    memset( dst->data.ptr + y*dst_step, fillval, size.width );
                EXIT;
            }
        }
    }

    ipp_cmp_op = cmp_op == CV_CMP_EQ ? cvCmpEq :
                 cmp_op == CV_CMP_GE ? cvCmpGreaterEq : cvCmpGreater;
    if( type == CV_8U && icvCompare_8u_C1R_p )
    {
        IPPI_CALL( icvCompareC_8u_C1R_p( src1->data.ptr, src1_step, (uchar)ival,
                                         dst->data.ptr, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_16S && icvCompare_16s_C1R_p )
    {
        IPPI_CALL( icvCompareC_16s_C1R_p( src1->data.s, src1_step, (short)ival,
                                          dst->data.s, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_32F && icvCompare_32f_C1R_p )
    {
        IPPI_CALL( icvCompareC_32f_C1R_p( src1->data.fl, src1_step, (float)value,
                                          dst->data.fl, dst_step, size, ipp_cmp_op ));
    }
    else
    {
        CvFunc2D_2A1P func = (CvFunc2D_2A1P)(cmps_tab[cmp_op].fn_2d[type]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( type <= CV_32S )
        {
            IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                             dst_step, size, &ival ));
        }
        else
        {
            IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                             dst_step, size, &value ));
        }
    }

    if( invflag )
        IPPI_CALL( icvNot_8u_C1R( dst->data.ptr, dst_step,
                           dst->data.ptr, dst_step, size ));

    __END__;
}


/****************************************************************************************\
*                                       Min/Max                                          *
\****************************************************************************************/


#define ICV_DEF_MINMAX_FUNC( __op__, name, flavor, arrtype, \
                             worktype, _toggle_macro_ )     \
static CvStatus CV_STDCALL                                  \
icv##name##_##flavor##_C1R( const arrtype* src1, int step1, \
    const arrtype* src2, int step2,                         \
    arrtype* dst, int step, CvSize size )                   \
{                                                           \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);     \
    step /= sizeof(dst[0]);                                 \
                                                            \
    for( ; size.height--; src1 += step1,                    \
            src2 += step2, dst += step )                    \
    {                                                       \
        int x;                                              \
        for( x = 0; x <= size.width - 4; x += 4 )           \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype b0 = _toggle_macro_(src2[x]);          \
            worktype a1 = _toggle_macro_(src1[x+1]);        \
            worktype b1 = _toggle_macro_(src2[x+1]);        \
            a0 = __op__( a0, b0 );                          \
            a1 = __op__( a1, b1 );                          \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
            dst[x+1] = (arrtype)_toggle_macro_(a1);         \
            a0 = _toggle_macro_(src1[x+2]);                 \
            b0 = _toggle_macro_(src2[x+2]);                 \
            a1 = _toggle_macro_(src1[x+3]);                 \
            b1 = _toggle_macro_(src2[x+3]);                 \
            a0 = __op__( a0, b0 );                          \
            a1 = __op__( a1, b1 );                          \
            dst[x+2] = (arrtype)_toggle_macro_(a0);         \
            dst[x+3] = (arrtype)_toggle_macro_(a1);         \
        }                                                   \
                                                            \
        for( ; x < size.width; x++ )                        \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype b0 = _toggle_macro_(src2[x]);          \
            a0 = __op__( a0, b0 );                          \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define ICV_DEF_MINMAX_CONST_FUNC( __op__, name,            \
    flavor, arrtype, worktype, _toggle_macro_)              \
static CvStatus CV_STDCALL                                  \
icv##name##C_##flavor##_C1R( const arrtype* src1, int step1,\
                             arrtype* dst, int step,        \
                             CvSize size, worktype* pScalar)\
{                                                           \
    worktype scalar = _toggle_macro_(*pScalar);             \
    step1 /= sizeof(src1[0]); step /= sizeof(dst[0]);       \
                                                            \
    for( ; size.height--; src1 += step1, dst += step )      \
    {                                                       \
        int x;                                              \
        for( x = 0; x <= size.width - 4; x += 4 )           \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype a1 = _toggle_macro_(src1[x+1]);        \
            a0 = __op__( a0, scalar );                      \
            a1 = __op__( a1, scalar );                      \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
            dst[x+1] = (arrtype)_toggle_macro_(a1);         \
            a0 = _toggle_macro_(src1[x+2]);                 \
            a1 = _toggle_macro_(src1[x+3]);                 \
            a0 = __op__( a0, scalar );                      \
            a1 = __op__( a1, scalar );                      \
            dst[x+2] = (arrtype)_toggle_macro_(a0);         \
            dst[x+3] = (arrtype)_toggle_macro_(a1);         \
        }                                                   \
                                                            \
        for( ; x < size.width; x++ )                        \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            a0 = __op__( a0, scalar );                      \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define ICV_DEF_MINMAX_ALL( flavor, arrtype, worktype,                             \
                            _toggle_macro_, _min_op_, _max_op_ )                   \
ICV_DEF_MINMAX_FUNC( _min_op_, Min, flavor, arrtype, worktype, _toggle_macro_ )    \
ICV_DEF_MINMAX_FUNC( _max_op_, Max, flavor, arrtype, worktype, _toggle_macro_ )    \
ICV_DEF_MINMAX_CONST_FUNC(_min_op_, Min, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_MINMAX_CONST_FUNC(_max_op_, Max, flavor, arrtype, worktype, _toggle_macro_)

ICV_DEF_MINMAX_ALL( 8u, uchar, int, CV_NOP, CV_MIN_8U, CV_MAX_8U )
ICV_DEF_MINMAX_ALL( 16u, ushort, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 16s, short, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 32s, int, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 32f, int, int, CV_TOGGLE_FLT, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 64f, double, double, CV_NOP, MIN, MAX )

#define icvMin_8s_C1R     0
#define icvMax_8s_C1R     0
#define icvMinC_8s_C1R    0
#define icvMaxC_8s_C1R    0

CV_DEF_INIT_FUNC_TAB_2D( Min, C1R )
CV_DEF_INIT_FUNC_TAB_2D( Max, C1R )
CV_DEF_INIT_FUNC_TAB_2D( MinC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( MaxC, C1R )




/****************************************************************************************\
*                                  Absolute Difference                                   *
\****************************************************************************************/

#define  ICV_DEF_BIN_ABS_DIFF_2D(name, arrtype, temptype, abs_macro, cast_macro)\
IPCVAPI_IMPL( CvStatus,                                 \
name,( const arrtype* src1, int step1,                  \
       const arrtype* src2, int step2,                  \
       arrtype* dst, int step, CvSize size ),           \
       (src1, step1, src2, step2, dst, step, size))     \
{                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); \
    step /= sizeof(dst[0]);                             \
                                                        \
    for( ; size.height--; src1 += step1, src2 += step2, \
                          dst += step )                 \
    {                                                   \
        int i;                                          \
                                                        \
        for( i = 0; i <= size.width - 4; i += 4 )       \
        {                                               \
            temptype t0 = src1[i] - src2[i];            \
            temptype t1 = src1[i+1] - src2[i+1];        \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[i] = cast_macro(t0);                    \
            dst[i+1] = cast_macro(t1);                  \
                                                        \
            t0 = src1[i+2] - src2[i+2];                 \
            t1 = src1[i+3] - src2[i+3];                 \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[i+2] = cast_macro(t0);                  \
            dst[i+3] = cast_macro(t1);                  \
        }                                               \
                                                        \
        for( ; i < size.width; i++ )                    \
        {                                               \
            temptype t0 = src1[i] - src2[i];            \
            t0 = (temptype)abs_macro(t0);               \
            dst[i] = cast_macro(t0);                    \
        }                                               \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_DEF_UN_ABS_DIFF_2D( name, arrtype, temptype, abs_macro, cast_macro)\
static CvStatus CV_STDCALL                              \
name( const arrtype* src0, int step1,                   \
      arrtype* dst0, int step,                          \
      CvSize size, const temptype* scalar )             \
{                                                       \
    step1 /= sizeof(src0[0]); step /= sizeof(dst0[0]);  \
                                                        \
    for( ; size.height--; src0 += step1, dst0 += step ) \
    {                                                   \
        int i, len = size.width;                        \
        const arrtype* src = src0;                      \
        arrtype* dst = dst0;                            \
                                                        \
        for( ; (len -= 12) >= 0; dst += 12, src += 12 ) \
        {                                               \
            temptype t0 = src[0] - scalar[0];           \
            temptype t1 = src[1] - scalar[1];           \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[0] = cast_macro( t0 );                  \
            dst[1] = cast_macro( t1 );                  \
                                                        \
            t0 = src[2] - scalar[2];                    \
            t1 = src[3] - scalar[3];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[2] = cast_macro( t0 );                  \
            dst[3] = cast_macro( t1 );                  \
                                                        \
            t0 = src[4] - scalar[4];                    \
            t1 = src[5] - scalar[5];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[4] = cast_macro( t0 );                  \
            dst[5] = cast_macro( t1 );                  \
                                                        \
            t0 = src[6] - scalar[6];                    \
            t1 = src[7] - scalar[7];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[6] = cast_macro( t0 );                  \
            dst[7] = cast_macro( t1 );                  \
                                                        \
            t0 = src[8] - scalar[8];                    \
            t1 = src[9] - scalar[9];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[8] = cast_macro( t0 );                  \
            dst[9] = cast_macro( t1 );                  \
                                                        \
            t0 = src[10] - scalar[10];                  \
            t1 = src[11] - scalar[11];                  \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[10] = cast_macro( t0 );                 \
            dst[11] = cast_macro( t1 );                 \
        }                                               \
                                                        \
        for( (len) += 12, i = 0; i < (len); i++ )       \
        {                                               \
            temptype t0 = src[i] - scalar[i];           \
            t0 = (temptype)abs_macro(t0);               \
            dst[i] = cast_macro( t0 );                  \
        }                                               \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_TO_8U(x)     ((uchar)(x))
#define  ICV_TO_16U(x)    ((ushort)(x))

ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_8u_C1R, uchar, int, CV_IABS, ICV_TO_8U )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_16u_C1R, ushort, int, CV_IABS, ICV_TO_16U )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_16s_C1R, short, int, CV_IABS, CV_CAST_16S )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_32s_C1R, int, int, CV_IABS, CV_CAST_32S )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_32f_C1R, float, float, fabs, CV_CAST_32F )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_64f_C1R, double, double, fabs, CV_CAST_64F )







/* End of file. */

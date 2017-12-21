

/* ////////////////////////////////////////////////////////////////////
//
//  Geometrical transforms on images and matrices: rotation, zoom etc.
//
// */

#include "_cv.h"


/************** interpolation constants and tables ***************/

#define ICV_WARP_MUL_ONE_8U(x)  ((x) << ICV_WARP_SHIFT)
#define ICV_WARP_DESCALE_8U(x)  CV_DESCALE((x), ICV_WARP_SHIFT*2)
#define ICV_WARP_CLIP_X(x)      ((unsigned)(x) < (unsigned)ssize.width ? \
                                (x) : (x) < 0 ? 0 : ssize.width - 1)
#define ICV_WARP_CLIP_Y(y)      ((unsigned)(y) < (unsigned)ssize.height ? \
                                (y) : (y) < 0 ? 0 : ssize.height - 1)

float icvLinearCoeffs[(ICV_LINEAR_TAB_SIZE+1)*2];

void icvInitLinearCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
        for( int i = 0; i <= ICV_LINEAR_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_LINEAR_TAB_SIZE;
            icvLinearCoeffs[i*2] = x;
            icvLinearCoeffs[i*2+1] = 1.f - x;
        }

        inittab = 1;
    }
}


float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

void icvInitCubicCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
#if 0
        // classical Mitchell-Netravali filter
        const double B = 1./3;
        const double C = 1./3;
        const double p0 = (6 - 2*B)/6.;
        const double p2 = (-18 + 12*B + 6*C)/6.;
        const double p3 = (12 - 9*B - 6*C)/6.;
        const double q0 = (8*B + 24*C)/6.;
        const double q1 = (-12*B - 48*C)/6.;
        const double q2 = (6*B + 30*C)/6.;
        const double q3 = (-B - 6*C)/6.;

        #define ICV_CUBIC_1(x)  (((x)*p3 + p2)*(x)*(x) + p0)
        #define ICV_CUBIC_2(x)  ((((x)*q3 + q2)*(x) + q1)*(x) + q0)
#else
        // alternative "sharp" filter
        const double A = -0.75;
        #define ICV_CUBIC_1(x)  (((A + 2)*(x) - (A + 3))*(x)*(x) + 1)
        #define ICV_CUBIC_2(x)  (((A*(x) - 5*A)*(x) + 8*A)*(x) - 4*A)
#endif
        for( int i = 0; i <= ICV_CUBIC_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_CUBIC_TAB_SIZE;
            icvCubicCoeffs[i*2] = (float)ICV_CUBIC_1(x);
            x += 1.f;
            icvCubicCoeffs[i*2+1] = (float)ICV_CUBIC_2(x);
        }

        inittab = 1;
    }
}


/****************************************************************************************\
*                                    WarpPerspective                                     *
\****************************************************************************************/

#define ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( flavor, arrtype, load_macro, cast_macro )\
static CvStatus CV_STDCALL                                                  \
icvWarpPerspective_Bilinear_##flavor##_CnR(                                 \
    const arrtype* src, int step, CvSize ssize,                             \
    arrtype* dst, int dststep, CvSize dsize,                                \
    const double* matrix, int cn,                                           \
    const arrtype* fillval )                                                \
{                                                                           \
    int x, y, k;                                                            \
    float A11 = (float)matrix[0], A12 = (float)matrix[1], A13 = (float)matrix[2];\
    float A21 = (float)matrix[3], A22 = (float)matrix[4], A23 = (float)matrix[5];\
    float A31 = (float)matrix[6], A32 = (float)matrix[7], A33 = (float)matrix[8];\
                                                                            \
    step /= sizeof(src[0]);                                                 \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( y = 0; y < dsize.height; y++, dst += dststep )                     \
    {                                                                       \
        float xs0 = A12*y + A13;                                            \
        float ys0 = A22*y + A23;                                            \
        float ws = A32*y + A33;                                             \
                                                                            \
        for( x = 0; x < dsize.width; x++, xs0 += A11, ys0 += A21, ws += A31 )\
        {                                                                   \
            float inv_ws = 1.f/ws;                                          \
            float xs = xs0*inv_ws;                                          \
            float ys = ys0*inv_ws;                                          \
            int ixs = cvFloor(xs);                                          \
            int iys = cvFloor(ys);                                          \
            float a = xs - ixs;                                             \
            float b = ys - iys;                                             \
            float p0, p1;                                                   \
                                                                            \
            if( (unsigned)ixs < (unsigned)(ssize.width - 1) &&              \
                (unsigned)iys < (unsigned)(ssize.height - 1) )              \
            {                                                               \
                const arrtype* ptr = src + step*iys + ixs*cn;               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr[k]) +                               \
                        a * (load_macro(ptr[k+cn]) - load_macro(ptr[k]));   \
                    p1 = load_macro(ptr[k+step]) +                          \
                        a * (load_macro(ptr[k+cn+step]) -                   \
                             load_macro(ptr[k+step]));                      \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( (unsigned)(ixs+1) < (unsigned)(ssize.width+1) &&       \
                     (unsigned)(iys+1) < (unsigned)(ssize.height+1))        \
            {                                                               \
                int x0 = ICV_WARP_CLIP_X( ixs );                            \
                int y0 = ICV_WARP_CLIP_Y( iys );                            \
                int x1 = ICV_WARP_CLIP_X( ixs + 1 );                        \
                int y1 = ICV_WARP_CLIP_Y( iys + 1 );                        \
                const arrtype* ptr0, *ptr1, *ptr2, *ptr3;                   \
                                                                            \
                ptr0 = src + y0*step + x0*cn;                               \
                ptr1 = src + y0*step + x1*cn;                               \
                ptr2 = src + y1*step + x0*cn;                               \
                ptr3 = src + y1*step + x1*cn;                               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr0[k]) +                              \
                        a * (load_macro(ptr1[k]) - load_macro(ptr0[k]));    \
                    p1 = load_macro(ptr2[k]) +                              \
                        a * (load_macro(ptr3[k]) - load_macro(ptr2[k]));    \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[x*cn+k] = fillval[k];                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_WARP_SCALE_ALPHA(x) ((x)*(1./(ICV_WARP_MASK+1)))

ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 32f, float, CV_NOP, CV_NOP )

typedef CvStatus (CV_STDCALL * CvWarpPerspectiveFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const double* matrix, int cn, const void* fillval );

static void icvInitWarpPerspectiveTab( CvFuncTable* bilin_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvWarpPerspective_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvWarpPerspective_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvWarpPerspective_Bilinear_32f_CnR;
}

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvWarpPerspective( const CvArr* srcarr, CvArr* dstarr,
                   const CvMat* matrix, int flags, CvScalar fillval )
{
    static CvFuncTable bilin_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvWarpPerspective" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int type, depth, cn;
    int method = flags & 3;
    double src_matrix[9], dst_matrix[9];
    double fillbuf[4];
    CvMat A = cvMat( 3, 3, CV_64F, src_matrix ),
          invA = cvMat( 3, 3, CV_64F, dst_matrix );
    CvWarpPerspectiveFunc func;
    CvSize ssize, dsize;

    if( method == CV_INTER_NN || method == CV_INTER_AREA )
        method = CV_INTER_LINEAR;
    
    if( !inittab )
    {
        icvInitWarpPerspectiveTab( &bilin_tab );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_IS_MAT(matrix) || CV_MAT_CN(matrix->type) != 1 ||
        CV_MAT_DEPTH(matrix->type) < CV_32F || matrix->rows != 3 || matrix->cols != 3 )
        CV_ERROR( CV_StsBadArg,
        "Transformation matrix should be 3x3 floating-point single-channel matrix" );

    if( flags & CV_WARP_INVERSE_MAP )
        cvConvertScale( matrix, &invA );
    else
    {
        cvConvertScale( matrix, &A );
        cvInvert( &A, &invA, CV_SVD );
    }

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );
    
    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);
    

    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );

    /*if( method == CV_INTER_LINEAR )*/
    {
        func = (CvWarpPerspectiveFunc)bilin_tab.fn_2d[depth];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, dst_matrix, cn,
                         flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0 ));
    }

    __END__;
}


/* Calculates coefficients of perspective transformation
 * which maps (xi,yi) to (ui,vi), (i=1,2,3,4):
 *
 *      c00*xi + c01*yi + c02
 * ui = ---------------------
 *      c20*xi + c21*yi + c22
 *
 *      c10*xi + c11*yi + c12
 * vi = ---------------------
 *      c20*xi + c21*yi + c22
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 -x0*u0 -y0*u0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 -x1*u1 -y1*u1 | |c01| |u1|
 * | x2 y2  1  0  0  0 -x2*u2 -y2*u2 | |c02| |u2|
 * | x3 y3  1  0  0  0 -x3*u3 -y3*u3 |.|c10|=|u3|,
 * |  0  0  0 x0 y0  1 -x0*v0 -y0*v0 | |c11| |v0|
 * |  0  0  0 x1 y1  1 -x1*v1 -y1*v1 | |c12| |v1|
 * |  0  0  0 x2 y2  1 -x2*v2 -y2*v2 | |c20| |v2|
 * \  0  0  0 x3 y3  1 -x3*v3 -y3*v3 / \c21/ \v3/
 *
 * where:
 *   cij - matrix coefficients, c22 = 1
 */
CV_IMPL CvMat*
cvGetPerspectiveTransform( const CvPoint2D32f* src,
                          const CvPoint2D32f* dst,
                          CvMat* matrix )
{
    CV_FUNCNAME( "cvGetPerspectiveTransform" );

    __BEGIN__;

    double a[8][8];
    double b[8], x[9];

    CvMat A = cvMat( 8, 8, CV_64FC1, a );
    CvMat B = cvMat( 8, 1, CV_64FC1, b );
    CvMat X = cvMat( 8, 1, CV_64FC1, x );

    int i;

    if( !src || !dst || !matrix )
        CV_ERROR( CV_StsNullPtr, "" );

    for( i = 0; i < 4; ++i )
    {
        a[i][0] = a[i+4][3] = src[i].x;
        a[i][1] = a[i+4][4] = src[i].y;
        a[i][2] = a[i+4][5] = 1;
        a[i][3] = a[i][4] = a[i][5] =
        a[i+4][0] = a[i+4][1] = a[i+4][2] = 0;
        a[i][6] = -src[i].x*dst[i].x;
        a[i][7] = -src[i].y*dst[i].x;
        a[i+4][6] = -src[i].x*dst[i].y;
        a[i+4][7] = -src[i].y*dst[i].y;
        b[i] = dst[i].x;
        b[i+4] = dst[i].y;
    }

    cvSolve( &A, &B, &X, CV_SVD );
    x[8] = 1;
    
    X = cvMat( 3, 3, CV_64FC1, x );
    cvConvert( &X, matrix );

    __END__;

    return matrix;
}

/****************************************************************************************\
*                          Generic Geometric Transformation: Remap                       *
\****************************************************************************************/

#define  ICV_DEF_REMAP_BILINEAR_FUNC( flavor, arrtype, load_macro, cast_macro ) \
static CvStatus CV_STDCALL                                                      \
icvRemap_Bilinear_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                         arrtype* dst, int dststep, CvSize dsize,           \
                         const float* mapx, int mxstep,                     \
                         const float* mapy, int mystep,                     \
                         int cn, const arrtype* fillval )                   \
{                                                                           \
    int i, j, k;                                                            \
    ssize.width--;                                                          \
    ssize.height--;                                                         \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    mxstep /= sizeof(mapx[0]);                                              \
    mystep /= sizeof(mapy[0]);                                              \
                                                                            \
    for( i = 0; i < dsize.height; i++, dst += dststep,                      \
                                  mapx += mxstep, mapy += mystep )          \
    {                                                                       \
        for( j = 0; j < dsize.width; j++ )                                  \
        {                                                                   \
            int ix = cvRound(mapx[j]*(1 << ICV_WARP_SHIFT));                \
            int iy = cvRound(mapy[j]*(1 << ICV_WARP_SHIFT));                \
            int ifx = ix & ICV_WARP_MASK;                                   \
            int ify = iy & ICV_WARP_MASK;                                   \
            ix >>= ICV_WARP_SHIFT;                                          \
            iy >>= ICV_WARP_SHIFT;                                          \
                                                                            \
            float x0 = icvLinearCoeffs[ifx*2];                              \
            float x1 = icvLinearCoeffs[ifx*2 + 1];                          \
            float y0 = icvLinearCoeffs[ify*2];                              \
            float y1 = icvLinearCoeffs[ify*2 + 1];                          \
                                                                            \
            if( (unsigned)ix < (unsigned)ssize.width &&                     \
                (unsigned)iy < (unsigned)ssize.height )                     \
            {                                                               \
                const arrtype* s = src + iy*srcstep + ix*cn;                \
                for( k = 0; k < cn; k++, s++ )                              \
                {                                                           \
                    float t0 = x1*load_macro(s[0]) + x0*load_macro(s[cn]);  \
                    float t1 = x1*load_macro(s[srcstep]) +                  \
                               x0*load_macro(s[srcstep + cn]);              \
                    dst[j*cn + k] = (arrtype)cast_macro(y1*t0 + y0*t1);     \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[j*cn + k] = fillval[k];                             \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define  ICV_DEF_REMAP_BICUBIC_FUNC( flavor, arrtype, worktype,                 \
                                     load_macro, cast_macro1, cast_macro2 )     \
static CvStatus CV_STDCALL                                                      \
icvRemap_Bicubic_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize, \
                         arrtype* dst, int dststep, CvSize dsize,               \
                         const float* mapx, int mxstep,                         \
                         const float* mapy, int mystep,                         \
                         int cn, const arrtype* fillval )                       \
{                                                                               \
    int i, j, k;                                                                \
    ssize.width = MAX( ssize.width - 3, 0 );                                    \
    ssize.height = MAX( ssize.height - 3, 0 );                                  \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    mxstep /= sizeof(mapx[0]);                                                  \
    mystep /= sizeof(mapy[0]);                                                  \
                                                                                \
    for( i = 0; i < dsize.height; i++, dst += dststep,                          \
                                  mapx += mxstep, mapy += mystep )              \
    {                                                                           \
        for( j = 0; j < dsize.width; j++ )                                      \
        {                                                                       \
            int ix = cvRound(mapx[j]*(1 << ICV_WARP_SHIFT));                    \
            int iy = cvRound(mapy[j]*(1 << ICV_WARP_SHIFT));                    \
            int ifx = ix & ICV_WARP_MASK;                                       \
            int ify = iy & ICV_WARP_MASK;                                       \
            ix >>= ICV_WARP_SHIFT;                                              \
            iy >>= ICV_WARP_SHIFT;                                              \
                                                                                \
            if( (unsigned)(ix-1) < (unsigned)ssize.width &&                     \
                (unsigned)(iy-1) < (unsigned)ssize.height )                     \
            {                                                                   \
                for( k = 0; k < cn; k++ )                                       \
                {                                                               \
                    const arrtype* s = src + (iy-1)*srcstep + ix*cn + k;        \
                                                                                \
                    float t0 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t1 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t2 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    s += srcstep;                                               \
                                                                                \
                    float t3 = load_macro(s[-cn])*icvCubicCoeffs[ifx*2 + 1] +   \
                               load_macro(s[0])*icvCubicCoeffs[ifx*2] +         \
                               load_macro(s[cn])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] +\
                               load_macro(s[cn*2])*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
                                                                                \
                    worktype t = cast_macro1( t0*icvCubicCoeffs[ify*2 + 1] +    \
                               t1*icvCubicCoeffs[ify*2] +                       \
                               t2*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ify)*2] +  \
                               t3*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ify)*2+1] );\
                                                                                \
                    dst[j*cn + k] = cast_macro2(t);                             \
                }                                                               \
            }                                                                   \
            else if( fillval )                                                  \
                for( k = 0; k < cn; k++ )                                       \
                    dst[j*cn + k] = fillval[k];                                 \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_REMAP_BILINEAR_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_REMAP_BILINEAR_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_REMAP_BILINEAR_FUNC( 32f, float, CV_NOP, CV_NOP )

ICV_DEF_REMAP_BICUBIC_FUNC( 8u, uchar, int, CV_8TO32F, cvRound, CV_FAST_CAST_8U )
ICV_DEF_REMAP_BICUBIC_FUNC( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_REMAP_BICUBIC_FUNC( 32f, float, float, CV_NOP, CV_NOP, CV_NOP )

typedef CvStatus (CV_STDCALL * CvRemapFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const float* mapx, int mxstep,
    const float* mapy, int mystep,
    int cn, const void* fillval );

static void icvInitRemapTab( CvFuncTable* bilinear_tab, CvFuncTable* bicubic_tab )
{
    bilinear_tab->fn_2d[CV_8U] = (void*)icvRemap_Bilinear_8u_CnR;
    bilinear_tab->fn_2d[CV_16U] = (void*)icvRemap_Bilinear_16u_CnR;
    bilinear_tab->fn_2d[CV_32F] = (void*)icvRemap_Bilinear_32f_CnR;

    bicubic_tab->fn_2d[CV_8U] = (void*)icvRemap_Bicubic_8u_CnR;
    bicubic_tab->fn_2d[CV_16U] = (void*)icvRemap_Bicubic_16u_CnR;
    bicubic_tab->fn_2d[CV_32F] = (void*)icvRemap_Bicubic_32f_CnR;
}


/******************** IPP remap functions *********************/

typedef CvStatus (CV_STDCALL * CvRemapIPPFunc)(
    const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
    const float* xmap, int xmapstep, const float* ymap, int ymapstep,
    void* dst, int dststep, CvSize dstsize, int interpolation ); 

icvRemap_8u_C1R_t icvRemap_8u_C1R_p = 0;
icvRemap_8u_C3R_t icvRemap_8u_C3R_p = 0;
icvRemap_8u_C4R_t icvRemap_8u_C4R_p = 0;

icvRemap_32f_C1R_t icvRemap_32f_C1R_p = 0;
icvRemap_32f_C3R_t icvRemap_32f_C3R_p = 0;
icvRemap_32f_C4R_t icvRemap_32f_C4R_p = 0;

/**************************************************************/

CV_IMPL void
cvRemap( const CvArr* srcarr, CvArr* dstarr,
         const CvArr* _mapx, const CvArr* _mapy,
         int flags, CvScalar fillval )
{
    static CvFuncTable bilinear_tab;
    static CvFuncTable bicubic_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvRemap" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat mxstub, *mapx = (CvMat*)_mapx;
    CvMat mystub, *mapy = (CvMat*)_mapy;
    int type, depth, cn;
    int method = flags & 3;
    double fillbuf[4];
    CvSize ssize, dsize;

    if( !inittab )
    {
        icvInitRemapTab( &bilinear_tab, &bicubic_tab );
        icvInitLinearCoeffTab();
        icvInitCubicCoeffTab();
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    CV_CALL( mapx = cvGetMat( mapx, &mxstub ));
    CV_CALL( mapy = cvGetMat( mapy, &mystub ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_TYPES_EQ( mapx, mapy ) || CV_MAT_TYPE( mapx->type ) != CV_32FC1 )
        CV_ERROR( CV_StsUnmatchedFormats, "Both map arrays must have 32fC1 type" );

    if( !CV_ARE_SIZES_EQ( mapx, mapy ) || !CV_ARE_SIZES_EQ( mapx, dst ))
        CV_ERROR( CV_StsUnmatchedSizes,
        "Both map arrays and the destination array must have the same size" );

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );
    
    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);
    
    if( icvRemap_8u_C1R_p )
    {
        CvRemapIPPFunc ipp_func =
            type == CV_8UC1 ? icvRemap_8u_C1R_p :
            type == CV_8UC3 ? icvRemap_8u_C3R_p :
            type == CV_8UC4 ? icvRemap_8u_C4R_p :
            type == CV_32FC1 ? icvRemap_32f_C1R_p :
            type == CV_32FC3 ? icvRemap_32f_C3R_p :
            type == CV_32FC4 ? icvRemap_32f_C4R_p : 0;
        
        if( ipp_func )
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            int mxstep = mapx->step ? mapx->step : CV_STUB_STEP;
            int mystep = mapy->step ? mapy->step : CV_STUB_STEP;
            CvStatus status;
            CvRect srcroi = {0, 0, ssize.width, ssize.height};

            // this is not the most efficient way to fill outliers
            if( flags & CV_WARP_FILL_OUTLIERS )
                cvSet( dst, fillval );

            status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               mapx->data.fl, mxstep, mapy->data.fl, mystep,
                               dst->data.ptr, dststep, dsize,
                               1 << (method == CV_INTER_NN || method == CV_INTER_LINEAR ||
                               method == CV_INTER_CUBIC ? method : CV_INTER_LINEAR) );
            if( status >= 0 )
                EXIT;
        }
    }

    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );

    {
        CvRemapFunc func = method == CV_INTER_CUBIC ?
            (CvRemapFunc)bicubic_tab.fn_2d[depth] :
            (CvRemapFunc)bilinear_tab.fn_2d[depth];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        func( src->data.ptr, src->step, ssize, dst->data.ptr, dst->step, dsize,
              mapx->data.fl, mapx->step, mapy->data.fl, mapy->step,
              cn, flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0 );
    }

    __END__;
}



/* End of file. */

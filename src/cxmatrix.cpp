

#include "_cxcore.h"

/****************************************************************************************\
*                                     Matrix transpose                                   *
\****************************************************************************************/

/////////////////// macros for inplace transposition of square matrix ////////////////////

#define ICV_DEF_TRANSP_INP_CASE_C1( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    step /= sizeof(arr[0]);         \
                                    \
    while( --len )                  \
    {                               \
        arr += step, arr1++;        \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        do                          \
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
                                    \
            arr2++;                 \
            arr3 += step;           \
        }                           \
        while( arr2 != arr3  );     \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C3( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 3;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=3, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
        }                           \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C4( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 4;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=4, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
            t0 = arr2[3];           \
            t1 = arr3[3];           \
            arr2[3] = t1;           \
            arr3[3] = t0;           \
        }                           \
    }                               \
}


//////////////// macros for non-inplace transposition of rectangular matrix //////////////

#define ICV_DEF_TRANSP_CASE_C1( arrtype )       \
{                                               \
    int x, y;                                   \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( y = 0; y <= size.height - 2; y += 2,   \
                src += 2*srcstep, dst += 2 )    \
    {                                           \
        const arrtype* src1 = src + srcstep;    \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += dststep )       \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1 += dststep;                    \
                                                \
            t0 = src[x + 1];                    \
            t1 = src1[x + 1];                   \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
    }                                           \
                                                \
    if( y < size.height )                       \
    {                                           \
        arrtype* dst1 = dst;                    \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += 2*dststep )     \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            dst1[0] = t0;                       \
            dst1[dststep] = t1;                 \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            dst1[0] = t0;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C3( arrtype )       \
{                                               \
    size.width *= 3;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=3 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 3,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            arrtype t2 = src[x + 2];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1[2] = t2;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C4( arrtype )       \
{                                               \
    size.width *= 4;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=4 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 4,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
                                                \
            t0 = src[x + 2];                    \
            t1 = src[x + 3];                    \
                                                \
            dst1[2] = t0;                       \
            dst1[3] = t1;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_INP_FUNC( flavor, arrtype, cn )      \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( arrtype* arr, int step, CvSize size )\
{                                                           \
    assert( size.width == size.height );                    \
                                                            \
    ICV_DEF_TRANSP_INP_CASE_C##cn( arrtype, size.width )    \
    return CV_OK;                                           \
}


#define ICV_DEF_TRANSP_FUNC( flavor, arrtype, cn )          \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( const arrtype* src, int srcstep,     \
                    arrtype* dst, int dststep, CvSize size )\
{                                                           \
    ICV_DEF_TRANSP_CASE_C##cn( arrtype )                    \
    return CV_OK;                                           \
}


ICV_DEF_TRANSP_INP_FUNC( 8u_C1IR, uchar, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C2IR, ushort, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C3IR, uchar, 3 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C2IR, int, 1 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C3IR, ushort, 3 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C2IR, int64, 1 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C3IR, int, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C2IR, int, 4 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C3IR, int64, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C4IR, int64, 4 )


ICV_DEF_TRANSP_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_TRANSP_FUNC( 16u_C2R, int, 1 )
ICV_DEF_TRANSP_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_TRANSP_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_TRANSP_FUNC( 32s_C3R, int, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C2R, int, 4 )
ICV_DEF_TRANSP_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, R )
CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, IR )

CV_IMPL void
cvTranspose( const CvArr* srcarr, CvArr* dstarr )
{
    static CvBtFuncTable tab, inp_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvTranspose" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    int type, pix_size;

    if( !inittab )
    {
        icvInitTransposeIRTable( &inp_tab );
        icvInitTransposeRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    type = CV_MAT_TYPE( src->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( src );

    if( dstarr == srcarr )
    {
        dst = src; 
    }
    else
    {
        if( !CV_IS_MAT( dst ))
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));

            if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
        }

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( size.width != dst->height || size.height != dst->width )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( src->data.ptr == dst->data.ptr )
    {
        if( size.width == size.height )
        {
            CvFunc2D_1A func = (CvFunc2D_1A)(inp_tab.fn_2d[pix_size]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            IPPI_CALL( func( src->data.ptr, src->step, size ));
        }
        else
        {
            if( size.width != 1 && size.height != 1 )
                CV_ERROR( CV_StsBadSize,
                    "Rectangular matrix can not be transposed inplace" );
            
            if( !CV_IS_MAT_CONT( src->type & dst->type ))
                CV_ERROR( CV_StsBadFlag, "In case of inplace column/row transposition "
                                       "both source and destination must be continuous" );

            if( dst == src )
            {
                int t;
                CV_SWAP( dst->width, dst->height, t );
                dst->step = dst->height == 1 ? 0 : pix_size;
            }
        }
    }
    else
    {
        CvFunc2D_2A func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step,
                         dst->data.ptr, dst->step, size ));
    }

    __END__;
}


/****************************************************************************************\
*                              LU decomposition/back substitution                        *
\****************************************************************************************/

#define arrtype float
#define temptype double

typedef  CvStatus (CV_STDCALL * CvLUDecompFunc)( double* A, int stepA, CvSize sizeA,
                                                 void* B, int stepB, CvSize sizeB,
                                                 double* det );

typedef  CvStatus (CV_STDCALL * CvLUBackFunc)( double* A, int stepA, CvSize sizeA,
                                               void* B, int stepB, CvSize sizeB );


#define ICV_DEF_LU_DECOMP_FUNC( flavor, arrtype )                               \
static CvStatus CV_STDCALL                                                      \
icvLUDecomp_##flavor( double* A, int stepA, CvSize sizeA,                       \
                      arrtype* B, int stepB, CvSize sizeB, double* _det )       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = 0, i;                                                               \
    double det = 1;                                                             \
                                                                                \
    assert( sizeA.width == sizeA.height );                                      \
                                                                                \
    if( B )                                                                     \
    {                                                                           \
        assert( sizeA.height == sizeB.height );                                 \
        m = sizeB.width;                                                        \
    }                                                                           \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    for( i = 0; i < n; i++, A += stepA, B += stepB )                            \
    {                                                                           \
        int j, k = i;                                                           \
        double* tA = A;                                                         \
        arrtype* tB = 0;                                                        \
        double kval = fabs(A[i]), tval;                                         \
                                                                                \
        /* find the pivot element */                                            \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tval = fabs(tA[i]);                                                 \
                                                                                \
            if( tval > kval )                                                   \
            {                                                                   \
                kval = tval;                                                    \
                k = j;                                                          \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( kval == 0 )                                                         \
        {                                                                       \
            det = 0;                                                            \
            break;                                                              \
        }                                                                       \
                                                                                \
        /* swap rows */                                                         \
        if( k != i )                                                            \
        {                                                                       \
            tA = A + stepA*(k - i);                                             \
            det = -det;                                                         \
                                                                                \
            for( j = i; j < n; j++ )                                            \
            {                                                                   \
                double t;                                                       \
                CV_SWAP( A[j], tA[j], t );                                      \
            }                                                                   \
                                                                                \
            if( m > 0 )                                                         \
            {                                                                   \
                tB = B + stepB*(k - i);                                         \
                                                                                \
                for( j = 0; j < m; j++ )                                        \
                {                                                               \
                    arrtype t = B[j];                                           \
                    CV_SWAP( B[j], tB[j], t );                                  \
                }                                                               \
            }                                                                   \
        }                                                                       \
                                                                                \
        tval = 1./A[i];                                                         \
        det *= A[i];                                                            \
        tA = A;                                                                 \
        tB = B;                                                                 \
        A[i] = tval; /* to replace division with multiplication in LUBack */    \
                                                                                \
        /* update matrix and the right side of the system */                    \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tB += stepB;                                                        \
            double alpha = -tA[i]*tval;                                         \
                                                                                \
            for( k = i + 1; k < n; k++ )                                        \
                tA[k] = tA[k] + alpha*A[k];                                     \
                                                                                \
            if( m > 0 )                                                         \
                for( k = 0; k < m; k++ )                                        \
                    tB[k] = (arrtype)(tB[k] + alpha*B[k]);                      \
        }                                                                       \
    }                                                                           \
                                                                                \
    if( _det )                                                                  \
        *_det = det;                                                            \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_DECOMP_FUNC( 32f, float )
ICV_DEF_LU_DECOMP_FUNC( 64f, double )


#define ICV_DEF_LU_BACK_FUNC( flavor, arrtype )                                 \
static CvStatus CV_STDCALL                                                      \
icvLUBack_##flavor( double* A, int stepA, CvSize sizeA,                         \
                    arrtype* B, int stepB, CvSize sizeB )                       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = sizeB.width, i;                                                     \
                                                                                \
    assert( m > 0 && sizeA.width == sizeA.height &&                             \
            sizeA.height == sizeB.height );                                     \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    A += stepA*(n - 1);                                                         \
    B += stepB*(n - 1);                                                         \
                                                                                \
    for( i = n - 1; i >= 0; i--, A -= stepA )                                   \
    {                                                                           \
        int j, k;                                                               \
        for( j = 0; j < m; j++ )                                                \
        {                                                                       \
            arrtype* tB = B + j;                                                \
            double x = 0;                                                       \
                                                                                \
            for( k = n - 1; k > i; k--, tB -= stepB )                           \
                x += A[k]*tB[0];                                                \
                                                                                \
            tB[0] = (arrtype)((tB[0] - x)*A[i]);                                \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_BACK_FUNC( 32f, float )
ICV_DEF_LU_BACK_FUNC( 64f, double )

static CvFuncTable lu_decomp_tab, lu_back_tab;
static int lu_inittab = 0;

static void icvInitLUTable( CvFuncTable* decomp_tab,
                            CvFuncTable* back_tab )
{
    decomp_tab->fn_2d[0] = (void*)icvLUDecomp_32f;
    decomp_tab->fn_2d[1] = (void*)icvLUDecomp_64f;
    back_tab->fn_2d[0] = (void*)icvLUBack_32f;
    back_tab->fn_2d[1] = (void*)icvLUBack_64f;
}



/****************************************************************************************\
*                                 Determinant of the matrix                              *
\****************************************************************************************/

#define det2(m)   (m(0,0)*m(1,1) - m(0,1)*m(1,0))
#define det3(m)   (m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -  \
                   m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +  \
                   m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0)))



/****************************************************************************************\
*                          Inverse (or pseudo-inverse) of the matrix                     *
\****************************************************************************************/

#define Sf( y, x ) ((float*)(srcdata + y*srcstep))[x]
#define Sd( y, x ) ((double*)(srcdata + y*srcstep))[x]
#define Df( y, x ) ((float*)(dstdata + y*dststep))[x]
#define Dd( y, x ) ((double*)(dstdata + y*dststep))[x]

/****************************************************************************************\
*                               Linear system [least-squares] solution                   *
\****************************************************************************************/

CV_IMPL int
cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    int result = 1;

    CV_FUNCNAME( "cvSolve" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)A;
    CvMat dstub, *dst = (CvMat*)x;
    CvMat bstub, *src2 = (CvMat*)b;
    int type;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( src2 ))
        CV_CALL( src2 = cvGetMat( src2, &bstub ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    if( method == CV_SVD || method == CV_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);

        if( method == CV_SVD_SYM && src->rows != src->cols )
            CV_ERROR( CV_StsBadSize, "CV_SVD_SYM method is used for non-square matrix" );

        CV_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != CV_SVD_SYM )
            CV_CALL( v = cvCreateMat( n, src->cols, src->type ));
        CV_CALL( w = cvCreateMat( n, 1, src->type ));
        CV_CALL( cvSVD( src, w, u, v, CV_SVD_U_T + CV_SVD_V_T ));
        CV_CALL( cvSVBkSb( w, u, v ? v : u, src2, dst, CV_SVD_U_T + CV_SVD_V_T ));
    }
    else if( method != CV_LU )
        CV_ERROR( CV_StsBadArg, "Unknown inversion method" );

    
    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( u || v || w )
    {
        cvReleaseMat( &u );
        cvReleaseMat( &v );
        cvReleaseMat( &w );
    }

    return result;
}



/* End of file. */

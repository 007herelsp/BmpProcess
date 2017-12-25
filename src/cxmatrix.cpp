

#include "_cxcore.h"

/****************************************************************************************\
*                               Linear system [least-squares] solution                   *
\****************************************************************************************/

VOS_IMPL int
cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    int result = 1;

    VOS_FUNCNAME( "cvSolve" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)A;
    CvMat dstub, *dst = (CvMat*)x;
    CvMat bstub, *src2 = (CvMat*)b;
    int type;

    if( !VOS_IS_MAT( src ))
        VOS_CALL( src = cvGetMat( src, &sstub ));

    if( !VOS_IS_MAT( src2 ))
        VOS_CALL( src2 = cvGetMat( src2, &bstub ));

    if( !VOS_IS_MAT( dst ))
        VOS_CALL( dst = cvGetMat( dst, &dstub ));

    if( method == VOS_SVD || method == VOS_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);

        if( method == VOS_SVD_SYM && src->rows != src->cols )
            VOS_ERROR( VOS_StsBadSize, "VOS_SVD_SYM method is used for non-square matrix" );

        VOS_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != VOS_SVD_SYM )
            VOS_CALL( v = cvCreateMat( n, src->cols, src->type ));
        VOS_CALL( w = cvCreateMat( n, 1, src->type ));
        VOS_CALL( cvSVD( src, w, u, v, VOS_SVD_U_T + VOS_SVD_V_T ));
        VOS_CALL( cvSVBkSb( w, u, v ? v : u, src2, dst, VOS_SVD_U_T + VOS_SVD_V_T ));
    }
    else if( method != VOS_LU )
        VOS_ERROR( VOS_StsBadArg, "Unknown inversion method" );

    
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

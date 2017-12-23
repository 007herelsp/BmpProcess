

#include "_cxcore.h"

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

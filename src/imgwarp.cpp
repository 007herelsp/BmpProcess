#include "_cv.h"


/****************************************************************************************\
*                               Linear system [least-squares] solution                   *
\****************************************************************************************/

static int
cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method )
{
    Mat* u = 0;
    Mat* v = 0;
    Mat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    int result = 1;

    VOS_FUNCNAME( "cvSolve" );

    __BEGIN__;

    Mat sstub, *src = (Mat*)A;
    Mat dstub, *dst = (Mat*)x;
    Mat bstub, *src2 = (Mat*)b;

    if( !VOS_IS_MAT( src ))
        VOS_CALL( src = GetMat( src, &sstub ));

    if( !VOS_IS_MAT( src2 ))
        VOS_CALL( src2 = GetMat( src2, &bstub ));

    if( !VOS_IS_MAT( dst ))
        VOS_CALL( dst = GetMat( dst, &dstub ));

    if( method == VOS_SVD || method == VOS_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);

        if( method == VOS_SVD_SYM && src->rows != src->cols )
            VOS_ERROR( VOS_StsBadSize, "VOS_SVD_SYM method is used for non-square matrix" );

        VOS_CALL( u = CreateMat( n, src->rows, src->type ));
        if( method != VOS_SVD_SYM )
            VOS_CALL( v = CreateMat( n, src->cols, src->type ));
        VOS_CALL( w = CreateMat( n, 1, src->type ));
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
        ReleaseMat( &u );
        ReleaseMat( &v );
        ReleaseMat( &w );
    }

    return result;
}


/************** interpolation constants and tables ***************/

#define IVOS_WARP_CLIP_X(x) ((unsigned)(x) < (unsigned)ssize.width ? (x) : (x) < 0 ? 0 : ssize.width - 1)
#define IVOS_WARP_CLIP_Y(y) ((unsigned)(y) < (unsigned)ssize.height ? (y) : (y) < 0 ? 0 : ssize.height - 1)

static CvStatus icvWarpPerspective_Bilinear_8u_CnR(const uchar *src, int step,
                                                   Size ssize, uchar *dst, int dststep, Size dsize,
                                                   const double *matrix, int cn, const uchar *fillval)
{
    int x, y, k;
    float A11 = (float)matrix[0], A12 = (float)matrix[1], A13 = (float)matrix[2];
    float A21 = (float)matrix[3], A22 = (float)matrix[4], A23 = (float)matrix[5];
    float A31 = (float)matrix[6], A32 = (float)matrix[7], A33 = (float)matrix[8];
    step /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    for (y = 0; y < dsize.height; y++, dst += dststep)
    {
        float xs0 = A12 * y + A13;
        float ys0 = A22 * y + A23;
        float ws = A32 * y + A33;
        for (x = 0; x < dsize.width; x++, xs0 += A11, ys0 += A21, ws += A31)
        {
            float inv_ws = 1.f / ws;
            float xs = xs0 * inv_ws;
            float ys = ys0 * inv_ws;
            int ixs = cvFloor(xs);
            int iys = cvFloor(ys);
            float a = xs - ixs;
            float b = ys - iys;
            float p0, p1;
            if ((unsigned)ixs < (unsigned)(ssize.width - 1) && (unsigned)iys < (unsigned)(ssize.height - 1))
            {
                const uchar *ptr = src + step * iys + ixs * cn;
                for (k = 0; k < cn; k++)
                {
                    p0 = icv8x32fTab_cv[(ptr[k]) + 256] + a * (icv8x32fTab_cv[(ptr[k + cn]) + 256] - icv8x32fTab_cv[(ptr[k]) + 256]);
                    p1 = icv8x32fTab_cv[(ptr[k + step]) + 256] + a * (icv8x32fTab_cv[(ptr[k + cn + step]) + 256] - icv8x32fTab_cv[(ptr[k + step]) + 256]);
                    dst[x * cn + k] = (uchar)cvRound(p0 + b * (p1 - p0));
                }
            }
            else if ((unsigned)(ixs + 1) < (unsigned)(ssize.width + 1) && (unsigned)(iys + 1) < (unsigned)(ssize.height + 1))
            {
                int x0 = IVOS_WARP_CLIP_X(ixs);
                int y0 = IVOS_WARP_CLIP_Y(iys);
                int x1 = IVOS_WARP_CLIP_Y(ixs + 1);
                int y1 = IVOS_WARP_CLIP_Y(iys + 1);
                const uchar *ptr0, *ptr1, *ptr2, *ptr3;
                ptr0 = src + y0 * step + x0 * cn;
                ptr1 = src + y0 * step + x1 * cn;
                ptr2 = src + y1 * step + x0 * cn;
                ptr3 = src + y1 * step + x1 * cn;
                for (k = 0; k < cn; k++)
                {
                    p0 = icv8x32fTab_cv[(ptr0[k]) + 256] + a * (icv8x32fTab_cv[(ptr1[k]) + 256] - icv8x32fTab_cv[(ptr0[k]) + 256]);
                    p1 = icv8x32fTab_cv[(ptr2[k]) + 256] + a * (icv8x32fTab_cv[(ptr3[k]) + 256] - icv8x32fTab_cv[(ptr2[k]) + 256]);
                    dst[x * cn + k] = (uchar)cvRound(p0 + b * (p1 - p0));
                }
            }
            else if (fillval)
                for (k = 0; k < cn; k++)
                    dst[x * cn + k] = fillval[k];
        }
    }
    return VOS_OK;
}

 void
cvWarpPerspective(const CvArr *srcarr, CvArr *dstarr,
                  const Mat *matrix, int flags, Scalar fillval)
{
    VOS_FUNCNAME("cvWarpPerspective");

    __BEGIN__;

    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    int type, depth, cn;
    double src_matrix[9], dst_matrix[9];
    double fillbuf[4];
    Mat A = cvMat(3, 3, VOS_64F, src_matrix),
          invA = cvMat(3, 3, VOS_64F, dst_matrix);

    Size ssize, dsize;
    VOS_CALL(src = GetMat(srcarr, &srcstub));
    VOS_CALL(dst = GetMat(dstarr, &dststub));

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    if (!VOS_IS_MAT(matrix) || VOS_MAT_CN(matrix->type) != 1 ||
        VOS_MAT_DEPTH(matrix->type) < VOS_32F || matrix->rows != 3 || matrix->cols != 3)
        VOS_ERROR(VOS_StsBadArg,
                 "Transformation matrix should be 3x3 floating-point single-channel matrix");

    if (flags & VOS_WARP_INVERSE_MAP)
        cvConvertScale(matrix, &invA);
    else
    {
        VOS_ERROR(VOS_StsUnmatchedFormats, "herelsp remove");
    }

    type = VOS_MAT_TYPE(src->type);
    depth = VOS_MAT_DEPTH(type);
    assert(depth == 0);
    cn = VOS_MAT_CN(type);
    if (cn > 4)
        VOS_ERROR(VOS_BadNumChannels, "");

    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);

    ScalarToRawData(&fillval, fillbuf, VOS_MAT_TYPE(src->type));

    FUN_CALL(icvWarpPerspective_Bilinear_8u_CnR(src->data.ptr, src->step, ssize, dst->data.ptr,
                                                 dst->step, dsize, dst_matrix, cn,
                                                 (const uchar *)(flags & VOS_WARP_FILL_OUTLIERS ? fillbuf : 0)));

    __END__;
}

 Mat *
cvGetPerspectiveTransform(const Point2D32f *src,
                          const Point2D32f *dst,
                          Mat *matrix)
{
    VOS_FUNCNAME("cvGetPerspectiveTransform");

    __BEGIN__;

    double a[8][8];
    double b[8], x[9];

    Mat A = cvMat(8, 8, VOS_64FC1, a);
    Mat B = cvMat(8, 1, VOS_64FC1, b);
    Mat X = cvMat(8, 1, VOS_64FC1, x);

    int i;

    if (!src || !dst || !matrix)
        VOS_ERROR(VOS_StsNullPtr, "");

    for (i = 0; i < 4; ++i)
    {
        a[i][0] = a[i + 4][3] = src[i].x;
        a[i][1] = a[i + 4][4] = src[i].y;
        a[i][2] = a[i + 4][5] = 1;
        a[i][3] = a[i][4] = a[i][5] =
            a[i + 4][0] = a[i + 4][1] = a[i + 4][2] = 0;
        a[i][6] = -src[i].x * dst[i].x;
        a[i][7] = -src[i].y * dst[i].x;
        a[i + 4][6] = -src[i].x * dst[i].y;
        a[i + 4][7] = -src[i].y * dst[i].y;
        b[i] = dst[i].x;
        b[i + 4] = dst[i].y;
    }

    cvSolve(&A, &B, &X, VOS_SVD);
    x[8] = 1;

    X = cvMat(3, 3, VOS_64FC1, x);
    cvConvert(&X, matrix);

    __END__;

    return matrix;
}

/* End of file. */
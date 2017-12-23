#include "_cv.h"

/************** interpolation constants and tables ***************/

#define ICV_WARP_CLIP_X(x) ((unsigned)(x) < (unsigned)ssize.width ? (x) : (x) < 0 ? 0 : ssize.width - 1)
#define ICV_WARP_CLIP_Y(y) ((unsigned)(y) < (unsigned)ssize.height ? (y) : (y) < 0 ? 0 : ssize.height - 1)

static CvStatus icvWarpPerspective_Bilinear_8u_CnR(const uchar *src, int step,
                                                   CvSize ssize, uchar *dst, int dststep, CvSize dsize,
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
                int x0 = ICV_WARP_CLIP_X(ixs);
                int y0 = ICV_WARP_CLIP_Y(iys);
                int x1 = ICV_WARP_CLIP_Y(ixs + 1);
                int y1 = ICV_WARP_CLIP_Y(iys + 1);
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
    return CV_OK;
}

CV_IMPL void
cvWarpPerspective(const CvArr *srcarr, CvArr *dstarr,
                  const CvMat *matrix, int flags, CvScalar fillval)
{
    CV_FUNCNAME("cvWarpPerspective");

    __BEGIN__;

    CvMat srcstub, *src = (CvMat *)srcarr;
    CvMat dststub, *dst = (CvMat *)dstarr;
    int type, depth, cn;
    double src_matrix[9], dst_matrix[9];
    double fillbuf[4];
    CvMat A = cvMat(3, 3, CV_64F, src_matrix),
          invA = cvMat(3, 3, CV_64F, dst_matrix);

    CvSize ssize, dsize;
    CV_CALL(src = cvGetMat(srcarr, &srcstub));
    CV_CALL(dst = cvGetMat(dstarr, &dststub));

    if (!CV_ARE_TYPES_EQ(src, dst))
        CV_ERROR(CV_StsUnmatchedFormats, "");

    if (!CV_IS_MAT(matrix) || CV_MAT_CN(matrix->type) != 1 ||
        CV_MAT_DEPTH(matrix->type) < CV_32F || matrix->rows != 3 || matrix->cols != 3)
        CV_ERROR(CV_StsBadArg,
                 "Transformation matrix should be 3x3 floating-point single-channel matrix");

    if (flags & CV_WARP_INVERSE_MAP)
        cvConvertScale(matrix, &invA);
    else
    {
        CV_ERROR(CV_StsUnmatchedFormats, "herelsp remove");
    }

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    assert(depth == 0);
    cn = CV_MAT_CN(type);
    if (cn > 4)
        CV_ERROR(CV_BadNumChannels, "");

    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);

    cvScalarToRawData(&fillval, fillbuf, CV_MAT_TYPE(src->type), 0);

    IPPI_CALL(icvWarpPerspective_Bilinear_8u_CnR(src->data.ptr, src->step, ssize, dst->data.ptr,
                                                 dst->step, dsize, dst_matrix, cn,
                                                 (const uchar *)(flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0)));

    __END__;
}

CV_IMPL CvMat *
cvGetPerspectiveTransform(const CvPoint2D32f *src,
                          const CvPoint2D32f *dst,
                          CvMat *matrix)
{
    CV_FUNCNAME("cvGetPerspectiveTransform");

    __BEGIN__;

    double a[8][8];
    double b[8], x[9];

    CvMat A = cvMat(8, 8, CV_64FC1, a);
    CvMat B = cvMat(8, 1, CV_64FC1, b);
    CvMat X = cvMat(8, 1, CV_64FC1, x);

    int i;

    if (!src || !dst || !matrix)
        CV_ERROR(CV_StsNullPtr, "");

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

    cvSolve(&A, &B, &X, CV_SVD);
    x[8] = 1;

    X = cvMat(3, 3, CV_64FC1, x);
    cvConvert(&X, matrix);

    __END__;

    return matrix;
}

/* End of file. */

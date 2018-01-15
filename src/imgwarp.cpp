#include "process.h"
#include "misc.h"

void ConvertScale(const AutoBuffer *src, AutoBuffer *dst)
{
    VOS_FUNCNAME("ConvertScale");

    __BEGIN__;

    if (src != dst)
    {
        Copy(src, dst);
    }

    __END__;
}

static int Solve(const VOID *A, const VOID *b, VOID *x, int method)
{
    AutoBuffer *u = NULL;
    AutoBuffer *v = NULL;
    AutoBuffer *w = NULL;

    uchar *buffer = NULL;
    int local_alloc = 0;
    int result = 1;

    VOS_FUNCNAME("Solve");

    __BEGIN__;

    AutoBuffer sstub, *src = (AutoBuffer *)A;
    AutoBuffer dstub, *dst = (AutoBuffer *)x;
    AutoBuffer bstub, *src2 = (AutoBuffer *)b;

    if (!VOS_IS_BUFFER(src))
        VOS_CALL(src = GetAutoBuffer(src, &sstub));

    if (!VOS_IS_BUFFER(src2))
        VOS_CALL(src2 = GetAutoBuffer(src2, &bstub));

    if (!VOS_IS_BUFFER(dst))
        VOS_CALL(dst = GetAutoBuffer(dst, &dstub));

    if (VOS_SVD == method || VOS_SVD_SYM == method)
    {
        int n = VOS_MIN(src->rows, src->cols);

        if (VOS_SVD_SYM == method && src->rows != src->cols)
            VOS_ERROR(VOS_StsBadSize, "");

        VOS_CALL(u = CreateAutoBuffer(n, src->rows, src->type));
        if (VOS_SVD_SYM != method)
            VOS_CALL(v = CreateAutoBuffer(n, src->cols, src->type));
        VOS_CALL(w = CreateAutoBuffer(n, 1, src->type));
        VOS_CALL(SVD(src, w, u, v, VOS_SVD_U_T + VOS_SVD_V_T));
        VOS_CALL(SVBkSb(w, u, v ? v : u, src2, dst, VOS_SVD_U_T + VOS_SVD_V_T));
    }
    else
        VOS_ERROR(VOS_StsBadArg, "");

    __END__;

    if (buffer && !local_alloc)
        SYS_FREE(&buffer);

    if (u || v || w)
    {
        ReleaseAutoBuffer(&u);
        ReleaseAutoBuffer(&v);
        ReleaseAutoBuffer(&w);
    }

    return result;
}

#define VOS_WARP_CLIP_X(x) ((unsigned)(x) < (unsigned)ssize.width ? (x) : (x) < 0 ? 0 : ssize.width - 1)
#define VOS_WARP_CLIP_Y(y) ((unsigned)(y) < (unsigned)ssize.height ? (y) : (y) < 0 ? 0 : ssize.height - 1)

static int iWarpPerspective_Bilinear_8u_CnR(const uchar *src, int step,
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
            int ixs = SysFloor(xs);
            int iys = SysFloor(ys);
            float a = xs - ixs;
            float b = ys - iys;
            float p0, p1;
            if ((unsigned)ixs < (unsigned)(ssize.width - 1) && (unsigned)iys < (unsigned)(ssize.height - 1))
            {
                const uchar *ptr = src + step * iys + ixs * cn;
                for (k = 0; k < cn; k++)
                {
                    p0 = VOS_8TO32F(ptr[k]) + a * (VOS_8TO32F(ptr[k + cn]) - VOS_8TO32F(ptr[k]));
                    p1 = VOS_8TO32F(ptr[k + step]) + a * (VOS_8TO32F(ptr[k + cn + step]) - VOS_8TO32F(ptr[k + step]));
                    dst[x * cn + k] = (uchar)SysRound(p0 + b * (p1 - p0));
                }
            }
            else if ((unsigned)(ixs + 1) < (unsigned)(ssize.width + 1) && (unsigned)(iys + 1) < (unsigned)(ssize.height + 1))
            {
                int x0 = VOS_WARP_CLIP_X(ixs);
                int y0 = VOS_WARP_CLIP_Y(iys);
                int x1 = VOS_WARP_CLIP_Y(ixs + 1);
                int y1 = VOS_WARP_CLIP_Y(iys + 1);
                const uchar *ptr0, *ptr1, *ptr2, *ptr3;
                ptr0 = src + y0 * step + x0 * cn;
                ptr1 = src + y0 * step + x1 * cn;
                ptr2 = src + y1 * step + x0 * cn;
                ptr3 = src + y1 * step + x1 * cn;
                for (k = 0; k < cn; k++)
                {
                    p0 = VOS_8TO32F(ptr0[k]) + a * (VOS_8TO32F(ptr1[k]) - VOS_8TO32F(ptr0[k]));
                    p1 = VOS_8TO32F(ptr2[k]) + a * (VOS_8TO32F(ptr3[k]) - VOS_8TO32F(ptr2[k]));
                    dst[x * cn + k] = (uchar)SysRound(p0 + b * (p1 - p0));
                }
            }
            else if (fillval)
                for (k = 0; k < cn; k++)
                    dst[x * cn + k] = fillval[k];
        }
    }
    return VOS_StsOk;
}

void WarpPerspective(const VOID *srcarr, VOID *dstarr,
                     const AutoBuffer *matrix, int flags, Scalar fillval)
{
    VOS_FUNCNAME("WarpPerspective");

    __BEGIN__;

    AutoBuffer srcstub, *src = (AutoBuffer *)srcarr;
    AutoBuffer dststub, *dst = (AutoBuffer *)dstarr;
    int type, depth, cn;
    double dst_matrix[9];
    double fillbuf[4];
    AutoBuffer invA = InitAutoBuffer(3, 3, VOS_64F, dst_matrix);

    Size ssize, dsize;
    VOS_CALL(src = GetAutoBuffer(srcarr, &srcstub));
    VOS_CALL(dst = GetAutoBuffer(dstarr, &dststub));

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    ConvertScale(matrix, &invA);

    type = VOS_BUFFER_TYPE(src->type);
    depth = VOS_BUFFER_DEPTH(type);
    assert(0 == depth);
    cn = VOS_BUFFER_CN(type);
    if (cn > 4)
        VOS_ERROR(VOS_BadNumChannels, "");

    ssize = GetBufferSize(src);
    dsize = GetBufferSize(dst);

    ScalarToRawData(&fillval, fillbuf, VOS_BUFFER_TYPE(src->type));

    VOS_FUN_CALL(iWarpPerspective_Bilinear_8u_CnR(src->data.ptr, src->step, ssize, dst->data.ptr,
                                                  dst->step, dsize, dst_matrix, cn,
                                                  (const uchar *)(flags & VOS_WARP_FILL_OUTLIERS ? fillbuf : 0)));

    __END__;
}

AutoBuffer *GetPerspectiveTransform(const Point2D32f *src,
                             const Point2D32f *dst,
                             AutoBuffer *matrix)
{
    VOS_FUNCNAME("GetPerspectiveTransform");

    __BEGIN__;

    double a[8][8];
    double b[8], x[9];

    AutoBuffer A = InitAutoBuffer(8, 8, VOS_64FC1, a);
    AutoBuffer B = InitAutoBuffer(8, 1, VOS_64FC1, b);
    AutoBuffer X = InitAutoBuffer(8, 1, VOS_64FC1, x);

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

    Solve(&A, &B, &X, VOS_SVD);
    x[8] = 1;

    X = InitAutoBuffer(3, 3, VOS_64FC1, x);
    Convert(&X, matrix);

    __END__;

    return matrix;
}

/* End of file. */

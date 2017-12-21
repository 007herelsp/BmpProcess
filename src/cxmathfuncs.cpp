#include "_cxcore.h"

//static const double _0_5 = 0.5, _1_5 = 1.5;

IPCVAPI_IMPL(CvStatus, icvInvSqrt_32f, (const float *src, float *dst, int len), (src, dst, len))
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)(1.f / sqrt(src[i]));

    return CV_OK;
}

IPCVAPI_IMPL(CvStatus, icvSqrt_32f, (const float *src, float *dst, int len), (src, dst, len))
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)sqrt(src[i]);

    return CV_OK;
}

IPCVAPI_IMPL(CvStatus, icvSqrt_64f, (const double *src, double *dst, int len), (src, dst, len))
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = sqrt(src[i]);

    return CV_OK;
}

IPCVAPI_IMPL(CvStatus, icvInvSqrt_64f, (const double *src, double *dst, int len), (src, dst, len))
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = 1. / sqrt(src[i]);

    return CV_OK;
}

#define ICV_DEF_SQR_MAGNITUDE_FUNC(flavor, arrtype, magtype)        \
    \
static CvStatus CV_STDCALL \
icvSqrMagnitude_##flavor(const arrtype *x, const arrtype *y,        \
                         magtype *mag, int len)                     \
    \
{                                                              \
        int i;                                                      \
                                                                    \
        for (i = 0; i <= len - 4; i += 4)                           \
        {                                                           \
            magtype x0 = (magtype)x[i], y0 = (magtype)y[i];         \
            magtype x1 = (magtype)x[i + 1], y1 = (magtype)y[i + 1]; \
                                                                    \
            x0 = x0 * x0 + y0 * y0;                                 \
            x1 = x1 * x1 + y1 * y1;                                 \
            mag[i] = x0;                                            \
            mag[i + 1] = x1;                                        \
            x0 = (magtype)x[i + 2], y0 = (magtype)y[i + 2];         \
            x1 = (magtype)x[i + 3], y1 = (magtype)y[i + 3];         \
            x0 = x0 * x0 + y0 * y0;                                 \
            x1 = x1 * x1 + y1 * y1;                                 \
            mag[i + 2] = x0;                                        \
            mag[i + 3] = x1;                                        \
        }                                                           \
                                                                    \
        for (; i < len; i++)                                        \
        {                                                           \
            magtype x0 = (magtype)x[i], y0 = (magtype)y[i];         \
            mag[i] = x0 * x0 + y0 * y0;                             \
        }                                                           \
                                                                    \
        return CV_OK;                                               \
    \
}

ICV_DEF_SQR_MAGNITUDE_FUNC(32f, float, float)
ICV_DEF_SQR_MAGNITUDE_FUNC(64f, double, double)


    /****************************************************************************************\
*                                    P O W E R                                           *
\****************************************************************************************/

#define ICV_DEF_IPOW_OP(flavor, arrtype, worktype, cast_macro)         \
    \
static CvStatus CV_STDCALL \
icvIPow_##flavor(const arrtype *src, arrtype *dst, int len, int power) \
    \
{                                                                 \
        int i;                                                         \
                                                                       \
        for (i = 0; i < len; i++)                                      \
        {                                                              \
            worktype a = 1, b = src[i];                                \
            int p = power;                                             \
            while (p > 1)                                              \
            {                                                          \
                if (p & 1)                                             \
                    a *= b;                                            \
                b *= b;                                                \
                p >>= 1;                                               \
            }                                                          \
                                                                       \
            a *= b;                                                    \
            dst[i] = cast_macro(a);                                    \
        }                                                              \
                                                                       \
        return CV_OK;                                                  \
    \
}

ICV_DEF_IPOW_OP(8u, uchar, int, CV_CAST_8U)
ICV_DEF_IPOW_OP(16u, ushort, int, CV_CAST_16U)
ICV_DEF_IPOW_OP(16s, short, int, CV_CAST_16S)
ICV_DEF_IPOW_OP(32s, int, int, CV_CAST_32S)
ICV_DEF_IPOW_OP(32f, float, double, CV_CAST_32F)
ICV_DEF_IPOW_OP(64f, double, double, CV_CAST_64F)

#define icvIPow_8s 0

CV_DEF_INIT_FUNC_TAB_1D(IPow)

typedef CvStatus(CV_STDCALL *CvIPowFunc)(const void *src, void *dst, int len, int power);
typedef CvStatus(CV_STDCALL *CvSqrtFunc)(const void *src, void *dst, int len);

CV_IMPL void cvPow(const CvArr *srcarr, CvArr *dstarr, double power)
{
    static CvFuncTable ipow_tab;
    static int inittab = 0;

    CV_FUNCNAME("cvPow");

    __BEGIN__;

    void *temp_buffer = 0;
    int block_size = 0;
    CvMat srcstub, *src = (CvMat *)srcarr;
    CvMat dststub, *dst = (CvMat *)dstarr;
    int coi1 = 0, coi2 = 0;
    int depth;
    CvSize size;
    int x, y;

    if (!CV_IS_MAT(src))
        CV_CALL(src = cvGetMat(src, &srcstub, &coi1));

    if (!CV_IS_MAT(dst))
        CV_CALL(dst = cvGetMat(dst, &dststub, &coi2));

    if (coi1 != 0 || coi2 != 0)
        CV_ERROR(CV_BadCOI, "");

    if (!CV_ARE_TYPES_EQ(src, dst))
        CV_ERROR_FROM_CODE(CV_StsUnmatchedFormats);

    if (!CV_ARE_SIZES_EQ(src, dst))
        CV_ERROR_FROM_CODE(CV_StsUnmatchedSizes);

    depth = CV_MAT_DEPTH(src->type);

    if (depth < CV_32F)
    {
        CV_ERROR(CV_StsUnsupportedFormat,
                 "Fractional or negative integer power factor can be used "
                 "with floating-point types only");
    }

    size = cvGetMatSize(src);
    size.width *= CV_MAT_CN(src->type);

    if (CV_IS_MAT_CONT(src->type & dst->type))
    {
        size.width *= size.height;
        size.height = 1;
    }

    assert("herelsp remove" && (fabs(fabs(power) - 0.5) < DBL_EPSILON));
    if( fabs(fabs(power) - 0.5) < DBL_EPSILON )
    {
        CvSqrtFunc sqrt_func = power < 0 ? (depth == CV_32F ? (CvSqrtFunc)icvInvSqrt_32f : (CvSqrtFunc)icvInvSqrt_64f) : (depth == CV_32F ? (CvSqrtFunc)icvSqrt_32f : (CvSqrtFunc)icvSqrt_64f);

        for (y = 0; y < size.height; y++)
        {
            uchar *src_data = src->data.ptr + src->step * y;
            uchar *dst_data = dst->data.ptr + dst->step * y;

            sqrt_func(src_data, dst_data, size.width);
        }
    }
    else
    {
         CV_ERROR(CV_StsUnsupportedFormat,
                 "Fractional or negative integer power factor can be used "
                 "with floating-point types only");
    }
    

    __END__;
}

/************************** CheckArray for NaN's, Inf's *********************************/

/* End of file. */

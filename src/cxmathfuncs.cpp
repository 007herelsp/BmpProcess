#include "_cxcore.h"

//static const double _0_5 = 0.5, _1_5 = 1.5;

static CvStatus icvInvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)(1.f / sqrt(src[i]));

    return CV_OK;
}

static CvStatus icvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return CV_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)sqrt(src[i]);

    return CV_OK;
}

typedef CvStatus(*CvSqrtFunc)(const void *src, void *dst, int len);

CV_IMPL void cvPow(const CvArr *srcarr, CvArr *dstarr, double power)
{
    CV_FUNCNAME("cvPow");

    __BEGIN__;


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

    assert(depth == CV_32F);
    if( fabs(fabs(power) - 0.5) < DBL_EPSILON )
    {
        CvSqrtFunc sqrt_func =( power < 0) ? (CvSqrtFunc)icvInvSqrt_32f : (CvSqrtFunc)icvSqrt_32f;

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
/* End of file. */

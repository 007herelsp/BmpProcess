#include "_cxcore.h"

//static const double _0_5 = 0.5, _1_5 = 1.5;

static CvStatus icvInvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return VOS_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)(1.f / sqrt(src[i]));

    return VOS_OK;
}

static CvStatus icvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return VOS_BADFACTOR_ERR;

    for (; i < len; i++)
        dst[i] = (float)sqrt(src[i]);

    return VOS_OK;
}

typedef CvStatus(*CvSqrtFunc)(const void *src, void *dst, int len);

 void SysPow(const CvArr *srcarr, CvArr *dstarr, double power)
{
    VOS_FUNCNAME("SysPow");

    __BEGIN__;


    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    int coi1 = 0, coi2 = 0;
    int depth;
    Size size;
    int y;

    if (!VOS_IS_MAT(src))
        VOS_CALL(src = GetMat(src, &srcstub, &coi1));

    if (!VOS_IS_MAT(dst))
        VOS_CALL(dst = GetMat(dst, &dststub, &coi2));

    if (coi1 != 0 || coi2 != 0)
        VOS_ERROR(VOS_BadCOI, "");

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedFormats);

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedSizes);

    depth = VOS_MAT_DEPTH(src->type);

    if (depth < VOS_32F)
    {
        VOS_ERROR(VOS_StsUnsupportedFormat,
                 "Fractional or negative integer power factor can be used "
                 "with floating-point types only");
    }

    size = GetMatSize(src);
    size.width *= VOS_MAT_CN(src->type);

    if (VOS_IS_MAT_CONT(src->type & dst->type))
    {
        size.width *= size.height;
        size.height = 1;
    }

    assert(depth == VOS_32F);
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
         VOS_ERROR(VOS_StsUnsupportedFormat,
                 "Fractional or negative integer power factor can be used "
                 "with floating-point types only");
    }
    __END__;
}
/* End of file. */

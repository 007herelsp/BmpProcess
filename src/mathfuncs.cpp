#include "core.h"
#include "misc.h"

static int iInvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return VOS_StsBadArg;

    for (; i < len; i++)
        dst[i] = (float)(1.f / sqrt(src[i]));

    return VOS_StsOk;
}

static int iSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    if (!(src && dst && len >= 0))
        return VOS_StsBadArg;

    for (; i < len; i++)
        dst[i] = (float)sqrt(src[i]);

    return VOS_StsOk;
}

typedef int(*SqrtFunc_t)(const void *src, void *dst, int len);

void SysPow(const AutoBuffer *src, AutoBuffer *dst, double power)
{
    VOS_FUNCNAME("SysPow");

    __BEGIN__;
    int depth;
    Size size;
    int y;

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedFormats);

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedSizes);

    depth = VOS_BUFFER_DEPTH(src->type);


    size = GetBufferSize(src);
    size.width *= VOS_BUFFER_CN(src->type);

    if (VOS_IS_BUFFER_CONT(src->type & dst->type))
    {
        size.width *= size.height;
        size.height = 1;
    }

    assert(VOS_32F == depth);
    if( fabs(fabs(power) - 0.5) < DBL_EPSILON )
    {
        SqrtFunc_t sqrt_func =( power < 0) ? (SqrtFunc_t)iInvSqrt_32f : (SqrtFunc_t)iSqrt_32f;

        for (y = 0; y < size.height; y++)
        {
            uchar *src_data = src->data.ptr + src->step * y;
            uchar *dst_data = dst->data.ptr + dst->step * y;
            sqrt_func(src_data, dst_data, size.width);
        }
    }
    else
    {
        VOS_ERROR(VOS_StsUnsupportedFormat,"");
    }
    __END__;
}
/* End of file. */

#include "_cxcore.h"
static CvStatus icvCopy_8u_C1R(const uchar *src, int srcstep,
                               uchar *dst, int dststep, CvSize size)
{
    for (; size.height--; src += srcstep, dst += dststep)
        memcpy(dst, src, size.width);

    return VOS_OK;
}

/* dst = src */
 void
cvCopy(const void *srcarr, void *dstarr, const void *maskarr)
{
    VOS_FUNCNAME("cvCopy");

    __BEGIN__;

    int pix_size;
    SysMat *src = (SysMat *)srcarr;
    SysMat *dst = (SysMat *)dstarr;
    CvSize size;
    assert("herelsp remove" && (VOS_IS_MAT(src) && VOS_IS_MAT(dst)));

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedFormats);

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR_FROM_CODE(VOS_StsUnmatchedSizes);

    size = cvGetMatSize(src);
    pix_size = VOS_ELEM_SIZE(src->type);

    if (!maskarr)
    {
        int src_step = src->step, dst_step = dst->step;
        size.width *= pix_size;
        if (VOS_IS_MAT_CONT(src->type & dst->type) && (src_step == dst_step) && (src_step == src->width * pix_size))
        {
            size.width *= size.height;

            if (size.width <= VOS_MAX_INLINE_MAT_OP_SIZE *
                                  VOS_MAX_INLINE_MAT_OP_SIZE * (int)sizeof(double))
            {
                memcpy(dst->data.ptr, src->data.ptr, size.width);
                EXIT;
            }

            size.height = 1;
            src_step = dst_step = VOS_STUB_STEP;
        }

        if (src->data.ptr != dst->data.ptr)
            icvCopy_8u_C1R(src->data.ptr, src_step,
                           dst->data.ptr, dst_step, size);
    }
    else
    {
        VOS_ERROR(VOS_StsBadMask, "not supported");
    }

    __END__;
}

/* End of file. */

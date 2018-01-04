#include "_cxcore.h"
static CvStatus iCopy_8u_C1R(const uchar *src, int srcstep,
                             uchar *dst, int dststep, Size size)
{
    for (; size.height--; src += srcstep, dst += dststep)
        VOS_MEMCPY(dst, src, size.width);

    return VOS_OK;
}

/* dst = src */
void Copy(const void *srcarr, void *dstarr, const void *maskarr)
{
    VOS_FUNCNAME("Copy");

    __BEGIN__;

    int pix_size;
    Mat *src = (Mat *)srcarr;
    Mat *dst = (Mat *)dstarr;
    Size size;

    size = GetMatSize(src);
    pix_size = VOS_ELEM_SIZE(src->type);

    int src_step = src->step, dst_step = dst->step;
    size.width *= pix_size;
    if (VOS_IS_MAT_CONT(src->type & dst->type) && (src_step == dst_step) && (src_step == src->width * pix_size))
    {
        size.width *= size.height;

        if (size.width <= VOS_MAX_INLINE_MAT_OP_SIZE *
                              VOS_MAX_INLINE_MAT_OP_SIZE * (int)sizeof(double))
        {
            VOS_MEMCPY(dst->data.ptr, src->data.ptr, size.width);
            EXIT;
        }

        size.height = 1;
        src_step = dst_step = VOS_STUB_STEP;
    }

    if (src->data.ptr != dst->data.ptr)
        iCopy_8u_C1R(src->data.ptr, src_step,
                     dst->data.ptr, dst_step, size);

    __END__;
}

/* End of file. */

#include "_cv.h"

/****************************************************************************************\
*                                 Color to/from Grayscale                                *
\****************************************************************************************/

#define fix(x, n) (int)((x) * (1 << (n)) + 0.5)
#define cscGr_32f 0.299f
#define cscGg_32f 0.587f
#define cscGb_32f 0.114f

/* BGR/RGB -> Gray */
#define csc_shift 14
#define cscGr fix(cscGr_32f, csc_shift)
#define cscGg fix(cscGg_32f, csc_shift)
#define cscGb /*fix(cscGb_32f,csc_shift)*/ ((1 << csc_shift) - cscGr - cscGg)

static CvStatus
BGR2Gray_8u_CnC1R(const uchar *src, int srcstep,
                      uchar *dst, int dststep, Size size,
                      int src_cn, int blue_idx)
{
    int i;
    srcstep -= size.width * src_cn;
    int b,g,r;
    for (; size.height--; src += srcstep, dst += dststep)
    {
        for (i = 0; i < size.width; i++, src += src_cn)
        {
            int t0 = src[blue_idx] * cscGb + src[1] * cscGg + src[blue_idx ^ 2] * cscGr;
            b = src[2];
            g = src[1];
            r = src[0];
            //t0 = b>g?b:g;
            //t0 = t0 >r?t0:r;
            //t0 = b;
            dst[i] = (uchar)VOS_DESCALE(t0, csc_shift);
            //dst[i+0]  =r;
        }
    }
    return VOS_OK;
}

/****************************************************************************************\
*                                   The main function                                    *
\****************************************************************************************/

 void
CvtColor(const VOID *srcarr, VOID *dstarr, int code)
{
    VOS_FUNCNAME("CvtColor");

    __BEGIN__;

    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    Size size;
    int src_step, dst_step;
    int src_cn, dst_cn, depth;

    VOS_CALL(src = GetMat(srcarr, &srcstub));
    VOS_CALL(dst = GetMat(dstarr, &dststub));

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedSizes, "");

    if (!VOS_ARE_DEPTHS_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    depth = VOS_MAT_DEPTH(src->type);
    if (depth != VOS_8U && depth != VOS_16U && depth != VOS_32F)
        VOS_ERROR(VOS_StsUnsupportedFormat, "");

    src_cn = VOS_MAT_CN(src->type);
    dst_cn = VOS_MAT_CN(dst->type);
    size = GetMatSize(src);
    src_step = src->step;
    dst_step = dst->step;

    if (src_cn != 3 || dst_cn != 1)
        VOS_ERROR(VOS_BadNumChannels,
                  "Incorrect number of channels for this conversion code");

    assert("herelsp remove" && (depth == VOS_8U));

    VOS_FUN_CALL(BGR2Gray_8u_CnC1R(src->data.ptr, src_step,
                                   dst->data.ptr, dst_step, size, src_cn, 2));

    __END__;
}

/* End of file. */

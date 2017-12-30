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

static CvStatus VOS_STDCALL
icvBGRx2Gray_8u_CnC1R(const uchar *src, int srcstep,
                      uchar *dst, int dststep, CvSize size,
                      int src_cn, int blue_idx)
{
    int i;
    srcstep -= size.width * src_cn;
    for (; size.height--; src += srcstep, dst += dststep)
    {
        for (i = 0; i < size.width; i++, src += src_cn)
        {
            int t0 = src[blue_idx] * cscGb + src[1] * cscGg + src[blue_idx ^ 2] * cscGr;
            dst[i] = (uchar)VOS_DESCALE(t0, csc_shift);
        }
    }
    return VOS_OK;
}

/****************************************************************************************\
*                                   The main function                                    *
\****************************************************************************************/

VOS_IMPL void
cvCvtColor(const CvArr *srcarr, CvArr *dstarr, int code)
{
    VOS_FUNCNAME("cvCvtColor");

    __BEGIN__;

    CvMat srcstub, *src = (CvMat *)srcarr;
    CvMat dststub, *dst = (CvMat *)dstarr;
    CvSize size;
    int src_step, dst_step;
    int src_cn, dst_cn, depth;
  
    VOS_CALL(src = cvGetMat(srcarr, &srcstub));
    VOS_CALL(dst = cvGetMat(dstarr, &dststub));

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedSizes, "");

    if (!VOS_ARE_DEPTHS_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    depth = VOS_MAT_DEPTH(src->type);
    if (depth != VOS_8U && depth != VOS_16U && depth != VOS_32F)
        VOS_ERROR(VOS_StsUnsupportedFormat, "");

    src_cn = VOS_MAT_CN(src->type);
    dst_cn = VOS_MAT_CN(dst->type);
    size = cvGetMatSize(src);
    src_step = src->step;
    dst_step = dst->step;

    if (src_cn != 3 || dst_cn != 1)
        VOS_ERROR(VOS_BadNumChannels,
                  "Incorrect number of channels for this conversion code");

    assert("herelsp remove" && (depth == VOS_8U));

    FUN_CALL(icvBGRx2Gray_8u_CnC1R(src->data.ptr, src_step,
                                   dst->data.ptr, dst_step, size, src_cn, 2));

    __END__;
}

/* End of file. */

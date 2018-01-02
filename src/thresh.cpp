#include "_cv.h"

static CvStatus VOS_STDCALL
icvThresh_8u_C1R(const uchar *src, int src_step, uchar *dst, int dst_step,
                 CvSize roi, uchar thresh, uchar maxval, int type)
{
    int i, j;
    uchar tab[256];
    if (VOS_THRESH_BINARY != type)
    {
        return VOS_BADFLAG_ERR;
    }
    for (i = 0; i <= thresh; i++)
        tab[i] = 0;
    for (; i < 256; i++)
        tab[i] = maxval;

    for (i = 0; i < roi.height; i++, src += src_step, dst += dst_step)
    {
        for (j = 0; j <= roi.width - 4; j += 4)
        {
            uchar t0 = tab[src[j]];
            uchar t1 = tab[src[j + 1]];

            dst[j] = t0;
            dst[j + 1] = t1;

            t0 = tab[src[j + 2]];
            t1 = tab[src[j + 3]];

            dst[j + 2] = t0;
            dst[j + 3] = t1;
        }

        for (; j < roi.width; j++)
            dst[j] = tab[src[j]];
    }

    return VOS_NO_ERR;
}

 void
cvThreshold(const void *srcarr, void *dstarr, double thresh, double maxval, int type)
{
    VOS_FUNCNAME("cvThreshold");

    __BEGIN__;

    CvSize roi;
    int src_step, dst_step;
    SysMat src_stub, *src = (SysMat *)srcarr;
    SysMat dst_stub, *dst = (SysMat *)dstarr;
    SysMat src0, dst0;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval, cn;
    bool use_otsu;

    VOS_CALL(src = GetMat(src, &src_stub, &coi1));
    VOS_CALL(dst = GetMat(dst, &dst_stub, &coi2));

    if (coi1 + coi2)
        VOS_ERROR(VOS_BadCOI, "COI is not supported by the function");

    if (!VOS_ARE_CNS_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "Both arrays must have equal number of channels");

    cn = VOS_MAT_CN(src->type);
    if (cn > 1)
    {
        src = Reshape(src, &src0, 1);
        dst = Reshape(dst, &dst0, 1);
    }

    use_otsu = (type & ~VOS_THRESH_MASK) == VOS_THRESH_OTSU;
    type &= VOS_THRESH_MASK;

    if (use_otsu)
    {
        assert("herelsp remove" && 0);
    }

    if (!VOS_ARE_DEPTHS_EQ(src, dst))
    {
        VOS_ERROR(VOS_BadDepth, cvUnsupportedFormat);
    }

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedSizes, "");

    roi = cvGetMatSize(src);
    if (VOS_IS_MAT_CONT(src->type & dst->type))
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = VOS_STUB_STEP;
    }
    else
    {
        src_step = src->step;
        dst_step = dst->step;
    }
    if (VOS_MAT_DEPTH(src->type) != VOS_8U)
    {
        VOS_ERROR(VOS_BadDepth, "herelsp remove");
    }

    ithresh = cvFloor(thresh);
    imaxval = cvRound(maxval);
    if (type == VOS_THRESH_TRUNC)
        imaxval = ithresh;
    imaxval = VOS_CAST_8U(imaxval);

    icvThresh_8u_C1R(src->data.ptr, src_step,
                     dst->data.ptr, dst_step, roi,
                     (uchar)ithresh, (uchar)imaxval, type);

    __END__;
}

/* End of file. */

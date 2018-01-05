#include "_cv.h"

static CvStatus
iThresh_8u_C1R(const uchar *src, int src_step, uchar *dst, int dst_step,
               Size roi, uchar thresh, uchar maxval, int type)
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

void Threshold(const void *srcarr, void *dstarr, double thresh, double maxval, int type)
{
    VOS_FUNCNAME("Threshold");

    __BEGIN__;

    Size roi;
    int src_step, dst_step;
    Mat src_stub, *src = (Mat *)srcarr;
    Mat dst_stub, *dst = (Mat *)dstarr;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval;

    VOS_CALL(src = GetMat(src, &src_stub, &coi1));
    VOS_CALL(dst = GetMat(dst, &dst_stub, &coi2));

    if (coi1 + coi2)
        VOS_ERROR(VOS_BadCOI, "COI is not supported by the function");

    if (!VOS_ARE_CNS_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats, "Both arrays must have equal number of channels");

    if (!VOS_ARE_DEPTHS_EQ(src, dst))
    {
        VOS_ERROR(VOS_BadDepth, "");
    }

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedSizes, "");

    roi = GetMatSize(src);
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
        VOS_ERROR(VOS_BadDepth, "");
    }

    ithresh = SysFloor(thresh);
    imaxval = SysRound(maxval);
    imaxval = VOS_CAST_8U(imaxval);

    iThresh_8u_C1R(src->data.ptr, src_step,
                   dst->data.ptr, dst_step, roi,
                   (uchar)ithresh, (uchar)imaxval, type);

    __END__;
}

/* End of file. */

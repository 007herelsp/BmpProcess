#include "_cv.h"

static CvStatus CV_STDCALL
icvThresh_8u_C1R(const uchar *src, int src_step, uchar *dst, int dst_step,
                 CvSize roi, uchar thresh, uchar maxval, int type)
{
    int i, j;
    uchar tab[256];
    if (CV_THRESH_BINARY != type)
    {
        return CV_BADFLAG_ERR;
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

    return CV_NO_ERR;
}

CV_IMPL void
cvThreshold(const void *srcarr, void *dstarr, double thresh, double maxval, int type)
{
    CV_FUNCNAME("cvThreshold");

    __BEGIN__;

    CvSize roi;
    int src_step, dst_step;
    CvMat src_stub, *src = (CvMat *)srcarr;
    CvMat dst_stub, *dst = (CvMat *)dstarr;
    CvMat src0, dst0;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval, cn;
    bool use_otsu;

    CV_CALL(src = cvGetMat(src, &src_stub, &coi1));
    CV_CALL(dst = cvGetMat(dst, &dst_stub, &coi2));

    if (coi1 + coi2)
        CV_ERROR(CV_BadCOI, "COI is not supported by the function");

    if (!CV_ARE_CNS_EQ(src, dst))
        CV_ERROR(CV_StsUnmatchedFormats, "Both arrays must have equal number of channels");

    cn = CV_MAT_CN(src->type);
    if (cn > 1)
    {
        src = cvReshape(src, &src0, 1);
        dst = cvReshape(dst, &dst0, 1);
    }

    use_otsu = (type & ~CV_THRESH_MASK) == CV_THRESH_OTSU;
    type &= CV_THRESH_MASK;

    if (use_otsu)
    {
        assert("herelsp remove" && 0);
    }

    if (!CV_ARE_DEPTHS_EQ(src, dst))
    {
        CV_ERROR(CV_BadDepth, cvUnsupportedFormat);
    }

    if (!CV_ARE_SIZES_EQ(src, dst))
        CV_ERROR(CV_StsUnmatchedSizes, "");

    roi = cvGetMatSize(src);
    if (CV_IS_MAT_CONT(src->type & dst->type))
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }
    else
    {
        src_step = src->step;
        dst_step = dst->step;
    }
    if (CV_MAT_DEPTH(src->type) != CV_8U)
    {
        CV_ERROR(CV_BadDepth, "herelsp remove");
    }

    ithresh = cvFloor(thresh);
    imaxval = cvRound(maxval);
    if (type == CV_THRESH_TRUNC)
        imaxval = ithresh;
    imaxval = CV_CAST_8U(imaxval);

    icvThresh_8u_C1R(src->data.ptr, src_step,
                     dst->data.ptr, dst_step, roi,
                     (uchar)ithresh, (uchar)imaxval, type);

    __END__;
}

/* End of file. */

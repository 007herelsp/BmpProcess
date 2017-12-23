#include "_cv.h"

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvSmooth(const void *srcarr, void *dstarr, int smooth_type,
         int param1, int param2, double param3, double param4)
{
    CvSepFilter gaussian_filter;

    CvMat *temp = 0;

    CV_FUNCNAME("cvSmooth");

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat *)srcarr;
    CvMat dststub, *dst = (CvMat *)dstarr;
    CvSize size;
    int src_type, dst_type, depth, cn;
    double sigma1 = 0, sigma2 = 0;
    bool have_ipp = false;

    CV_CALL(src = cvGetMat(src, &srcstub, &coi1));
    CV_CALL(dst = cvGetMat(dst, &dststub, &coi2));

    if (coi1 != 0 || coi2 != 0)
        CV_ERROR(CV_BadCOI, "");

    src_type = CV_MAT_TYPE(src->type);
    dst_type = CV_MAT_TYPE(dst->type);
    depth = CV_MAT_DEPTH(src_type);
    cn = CV_MAT_CN(src_type);
    size = cvGetMatSize(src);

    if (!CV_ARE_SIZES_EQ(src, dst))
        CV_ERROR(CV_StsUnmatchedSizes, "");

    if (smooth_type != CV_BLUR_NO_SCALE && !CV_ARE_TYPES_EQ(src, dst))
        CV_ERROR(CV_StsUnmatchedFormats,
                 "The specified smoothing algorithm requires input and ouput arrays be of the same type");

    if (smooth_type == CV_BLUR || smooth_type == CV_BLUR_NO_SCALE ||
        smooth_type == CV_GAUSSIAN || smooth_type == CV_MEDIAN)
    {
        // automatic detection of kernel size from sigma
        if (smooth_type == CV_GAUSSIAN)
        {
            sigma1 = param3;
            sigma2 = param4 ? param4 : param3;

            if (param1 == 0 && sigma1 > 0)
                param1 = cvRound(sigma1 * (depth == CV_8U ? 3 : 4) * 2 + 1) | 1;
            if (param2 == 0 && sigma2 > 0)
                param2 = cvRound(sigma2 * (depth == CV_8U ? 3 : 4) * 2 + 1) | 1;
        }

        if (param2 == 0)
            param2 = size.height == 1 ? 1 : param1;
        if (param1 < 1 || (param1 & 1) == 0 || param2 < 1 || (param2 & 1) == 0)
            CV_ERROR(CV_StsOutOfRange,
                     "Both mask width and height must be >=1 and odd");

        if (param1 == 1 && param2 == 1)
        {
            cvConvert(src, dst);
            EXIT;
        }
    }

    if (smooth_type == CV_GAUSSIAN)
    {
        CvSize ksize = {param1, param2};
        float *kx = (float *)cvStackAlloc(ksize.width * sizeof(kx[0]));
        float *ky = (float *)cvStackAlloc(ksize.height * sizeof(ky[0]));
        CvMat KX = cvMat(1, ksize.width, CV_32F, kx);
        CvMat KY = cvMat(1, ksize.height, CV_32F, ky);

        CvSepFilter::init_gaussian_kernel(&KX, sigma1);
        if (ksize.width != ksize.height || fabs(sigma1 - sigma2) > FLT_EPSILON)
            CvSepFilter::init_gaussian_kernel(&KY, sigma2);
        else
            KY.data.fl = kx;

        if (have_ipp && size.width >= param1 * 3 &&
            size.height >= param2 && param1 > 1 && param2 > 1)
        {
            assert("herelsp remove" && 0);
        }

        CV_CALL(gaussian_filter.init(src->cols, src_type, dst_type, &KX, &KY));
        CV_CALL(gaussian_filter.process(src, dst));
    }
    else
    {
        assert("herelsp remove" && 0);
    }

    __END__;

    cvReleaseMat(&temp);
}

/* End of file. */

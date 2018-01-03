#include "_cv.h"

//////////////////////////////////////////////////////////////////////////////////////////

void Smooth(const void *srcarr, void *dstarr, int smooth_type,
            int param1, int param2, double param3, double param4)
{
    SepFilter gaussian_filter;

    Mat *temp = 0;

    VOS_FUNCNAME("Smooth");

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    Size size;
    int src_type, dst_type, depth;
    double sigma1 = 0, sigma2 = 0;
    bool have_ipp = false;

    VOS_CALL(src = GetMat(src, &srcstub, &coi1));
    VOS_CALL(dst = GetMat(dst, &dststub, &coi2));

    if (coi1 != 0 || coi2 != 0)
        VOS_ERROR(VOS_BadCOI, "");

    src_type = VOS_MAT_TYPE(src->type);
    dst_type = VOS_MAT_TYPE(dst->type);
    depth = VOS_MAT_DEPTH(src_type);
    size = GetMatSize(src);

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedSizes, "");

    if (!VOS_ARE_TYPES_EQ(src, dst))
        VOS_ERROR(VOS_StsUnmatchedFormats,
                  "The specified smoothing algorithm requires input and ouput arrays be of the same type");

    sigma1 = param3;
    sigma2 = param4 ? param4 : param3;

    if (param1 == 0 && sigma1 > 0)
        param1 = SysRound(sigma1 * (depth == VOS_8U ? 3 : 4) * 2 + 1) | 1;
    if (param2 == 0 && sigma2 > 0)
        param2 = SysRound(sigma2 * (depth == VOS_8U ? 3 : 4) * 2 + 1) | 1;

    if (param2 == 0)
        param2 = size.height == 1 ? 1 : param1;
    if (param1 < 1 || (param1 & 1) == 0 || param2 < 1 || (param2 & 1) == 0)
        VOS_ERROR(VOS_StsOutOfRange,
                  "Both mask width and height must be >=1 and odd");

    if (param1 == 1 && param2 == 1)
    {
        Convert(src, dst);
        EXIT;
    }

    {
        Size ksize = {param1, param2};
        float *kx = (float *)cvStackAlloc(ksize.width * sizeof(kx[0]));
        float *ky = (float *)cvStackAlloc(ksize.height * sizeof(ky[0]));
        Mat KX = InitMat(1, ksize.width, VOS_32F, kx);
        Mat KY = InitMat(1, ksize.height, VOS_32F, ky);

        SepFilter::init_gaussian_kernel(&KX, sigma1);
        if (ksize.width != ksize.height || fabs(sigma1 - sigma2) > FLT_EPSILON)
            SepFilter::init_gaussian_kernel(&KY, sigma2);
        else
            KY.data.fl = kx;

        if (have_ipp && size.width >= param1 * 3 &&
            size.height >= param2 && param1 > 1 && param2 > 1)
        {
            assert("herelsp remove" && 0);
        }

        VOS_CALL(gaussian_filter.init(src->cols, src_type, dst_type, &KX, &KY));
        VOS_CALL(gaussian_filter.process(src, dst));
    }
    

    __END__;

    ReleaseMat(&temp);
}

/* End of file. */

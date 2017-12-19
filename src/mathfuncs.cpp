
#include "core.precomp.hpp"

namespace cv
{

typedef void (*MathFunc)(const void *src, void *dst, int len);

static void InvSqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    for (; i < len; i++)
        dst[i] = 1 / std::sqrt(src[i]);
}

static void InvSqrt_64f(const double *src, double *dst, int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = 1 / std::sqrt(src[i]);
}

static void Sqrt_32f(const float *src, float *dst, int len)
{
    int i = 0;

    for (; i < len; i++)
        dst[i] = std::sqrt(src[i]);
}

static void Sqrt_64f(const double *src, double *dst, int len)
{
    int i = 0;

    for (; i < len; i++)
        dst[i] = std::sqrt(src[i]);
}

/****************************************************************************************\
*                                    P O W E R                                           *
\****************************************************************************************/

template <typename T, typename WT>
static void
iPow_(const T *src, T *dst, int len, int power)
{
    int i;
    for (i = 0; i < len; i++)
    {
        WT a = 1, b = src[i];
        int p = power;
        while (p > 1)
        {
            if (p & 1)
                a *= b;
            b *= b;
            p >>= 1;
        }

        a *= b;
        dst[i] = saturate_cast<T>(a);
    }
}
static void iPow32s(const int *src, int *dst, int len, int power)
{
    iPow_<int, int>(src, dst, len, power);
}

static void iPow32f(const float *src, float *dst, int len, int power)
{
    iPow_<float, float>(src, dst, len, power);
}

static void iPow64f(const double *src, double *dst, int len, int power)
{
    iPow_<double, double>(src, dst, len, power);
}

typedef void (*IPowFunc)(const uchar *src, uchar *dst, int len, int power);

void pow(InputArray _src, double power, OutputArray _dst)
{
    Mat src = _src.getMat();
    int type = src.type(), depth = src.depth(), cn = src.channels();

    _dst.create(src.dims, src.size, type);
    Mat dst = _dst.getMat();

    int ipower = cvRound(power);
    bool is_ipower = false;
    CV_Assert(depth == CV_32F || depth == CV_64F);

    const Mat *arrays[] = {&src, &dst, 0};
    uchar *ptrs[2];
    NAryMatIterator it(arrays, ptrs);
    int len = (int)(it.size * cn);

    if (fabs(fabs(power) - 0.5) < DBL_EPSILON)
    {
        MathFunc func = power < 0 ? (depth == CV_32F ? (MathFunc)InvSqrt_32f : (MathFunc)InvSqrt_64f) : (depth == CV_32F ? (MathFunc)Sqrt_32f : (MathFunc)Sqrt_64f);

        for (size_t i = 0; i < it.nplanes; i++, ++it)
            func(ptrs[0], ptrs[1], len);
    }
}

}

CV_IMPL void cvPow(const CvArr *srcarr, CvArr *dstarr, double power)
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr);
    CV_Assert(src.type() == dst.type() && src.size == dst.size);
    cv::pow(src, power, dst);
}

/* End of file. */

#include "imgproc.precomp.hpp"
#include <opencv2/imgproc/imgproc.hpp>

/*
 * This file includes the code, contributed by Simon Perreault
 * (the function icvMedianBlur_8u_O1)
 *
 * Constant-time median filtering -- http://nomis80.org/ctmf.html
 * Copyright (C) 2006 Simon Perreault
 *
 * Contact:
 *  Laboratoire de vision et systemes numeriques
 *  Pavillon Adrien-Pouliot
 *  Universite Laval
 *  Sainte-Foy, Quebec, Canada
 *  G1K 7P4
 *
 *  perreaul@gel.ulaval.ca
 */

namespace cv
{

/****************************************************************************************\
                                         Box Filter
\****************************************************************************************/

template <typename T, typename ST>
struct RowSum : public BaseRowFilter
{
    RowSum(int _ksize, int _anchor)
    {
        ksize = _ksize;
        anchor = _anchor;
    }

    void operator()(const uchar *src, uchar *dst, int width, int cn)
    {
        const T *S = (const T *)src;
        ST *D = (ST *)dst;
        int i = 0, k, ksz_cn = ksize * cn;

        width = (width - 1) * cn;
        for (k = 0; k < cn; k++, S++, D++)
        {
            ST s = 0;
            for (i = 0; i < ksz_cn; i += cn)
                s += S[i];
            D[0] = s;
            for (i = 0; i < width; i += cn)
            {
                s += S[i + ksz_cn] - S[i];
                D[i + cn] = s;
            }
        }
    }
};

template <typename ST, typename T>
struct ColumnSum : public BaseColumnFilter
{
    ColumnSum(int _ksize, int _anchor, double _scale)
    {
        ksize = _ksize;
        anchor = _anchor;
        scale = _scale;
        sumCount = 0;
    }

    void reset() { sumCount = 0; }

    void operator()(const uchar **src, uchar *dst, int dststep, int count, int width)
    {
        int i;
        ST *SUM;
        bool haveScale = scale != 1;
        double _scale = scale;

        if (width != (int)sum.size())
        {
            sum.resize(width);
            sumCount = 0;
        }

        SUM = &sum[0];
        if (sumCount == 0)
        {
            for (i = 0; i < width; i++)
                SUM[i] = 0;
            for (; sumCount < ksize - 1; sumCount++, src++)
            {
                const ST *Sp = (const ST *)src[0];
                for (i = 0; i <= width - 2; i += 2)
                {
                    ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
                    SUM[i] = s0;
                    SUM[i + 1] = s1;
                }

                for (; i < width; i++)
                    SUM[i] += Sp[i];
            }
        }
        else
        {
            CV_Assert(sumCount == ksize - 1);
            src += ksize - 1;
        }

        for (; count--; src++)
        {
            const ST *Sp = (const ST *)src[0];
            const ST *Sm = (const ST *)src[1 - ksize];
            T *D = (T *)dst;
            if (haveScale)
            {
                for (i = 0; i <= width - 2; i += 2)
                {
                    ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
                    D[i] = saturate_cast<T>(s0 * _scale);
                    D[i + 1] = saturate_cast<T>(s1 * _scale);
                    s0 -= Sm[i];
                    s1 -= Sm[i + 1];
                    SUM[i] = s0;
                    SUM[i + 1] = s1;
                }

                for (; i < width; i++)
                {
                    ST s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<T>(s0 * _scale);
                    SUM[i] = s0 - Sm[i];
                }
            }
            else
            {
                for (i = 0; i <= width - 2; i += 2)
                {
                    ST s0 = SUM[i] + Sp[i], s1 = SUM[i + 1] + Sp[i + 1];
                    D[i] = saturate_cast<T>(s0);
                    D[i + 1] = saturate_cast<T>(s1);
                    s0 -= Sm[i];
                    s1 -= Sm[i + 1];
                    SUM[i] = s0;
                    SUM[i + 1] = s1;
                }

                for (; i < width; i++)
                {
                    ST s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<T>(s0);
                    SUM[i] = s0 - Sm[i];
                }
            }
            dst += dststep;
        }
    }

    double scale;
    int sumCount;
    vector<ST> sum;
};

template <>
struct ColumnSum<int, uchar> : public BaseColumnFilter
{
    ColumnSum(int _ksize, int _anchor, double _scale)
    {
        ksize = _ksize;
        anchor = _anchor;
        scale = _scale;
        sumCount = 0;
    }

    void reset() { sumCount = 0; }

    void operator()(const uchar **src, uchar *dst, int dststep, int count, int width)
    {
        int i;
        int *SUM;
        bool haveScale = scale != 1;
        double _scale = scale;


        if (width != (int)sum.size())
        {
            sum.resize(width);
            sumCount = 0;
        }

        SUM = &sum[0];
        if (sumCount == 0)
        {
            memset((void *)SUM, 0, width * sizeof(int));
            for (; sumCount < ksize - 1; sumCount++, src++)
            {
                const int *Sp = (const int *)src[0];
                i = 0;
                for (; i < width; i++)
                    SUM[i] += Sp[i];
            }
        }
        else
        {
            CV_Assert(sumCount == ksize - 1);
            src += ksize - 1;
        }

        for (; count--; src++)
        {
            const int *Sp = (const int *)src[0];
            const int *Sm = (const int *)src[1 - ksize];
            uchar *D = (uchar *)dst;
            if (haveScale)
            {
                i = 0;
                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<uchar>(s0 * _scale);
                    SUM[i] = s0 - Sm[i];
                }
            }
            else
            {
                i = 0;

                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<uchar>(s0);
                    SUM[i] = s0 - Sm[i];
                }
            }
            dst += dststep;
        }
    }

    double scale;
    int sumCount;
    vector<int> sum;
};

template <>
struct ColumnSum<int, short> : public BaseColumnFilter
{
    ColumnSum(int _ksize, int _anchor, double _scale)
    {
        ksize = _ksize;
        anchor = _anchor;
        scale = _scale;
        sumCount = 0;
    }

    void reset() { sumCount = 0; }

    void operator()(const uchar **src, uchar *dst, int dststep, int count, int width)
    {
        int i;
        int *SUM;
        bool haveScale = scale != 1;
        double _scale = scale;


        if (width != (int)sum.size())
        {
            sum.resize(width);
            sumCount = 0;
        }
        SUM = &sum[0];
        if (sumCount == 0)
        {
            memset((void *)SUM, 0, width * sizeof(int));
            for (; sumCount < ksize - 1; sumCount++, src++)
            {
                const int *Sp = (const int *)src[0];
                i = 0;
                for (; i < width; i++)
                    SUM[i] += Sp[i];
            }
        }
        else
        {
            CV_Assert(sumCount == ksize - 1);
            src += ksize - 1;
        }

        for (; count--; src++)
        {
            const int *Sp = (const int *)src[0];
            const int *Sm = (const int *)src[1 - ksize];
            short *D = (short *)dst;
            if (haveScale)
            {
                i = 0;
                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<short>(s0 * _scale);
                    SUM[i] = s0 - Sm[i];
                }
            }
            else
            {
                i = 0;

                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<short>(s0);
                    SUM[i] = s0 - Sm[i];
                }
            }
            dst += dststep;
        }
    }

    double scale;
    int sumCount;
    vector<int> sum;
};

template <>
struct ColumnSum<int, ushort> : public BaseColumnFilter
{
    ColumnSum(int _ksize, int _anchor, double _scale)
    {
        ksize = _ksize;
        anchor = _anchor;
        scale = _scale;
        sumCount = 0;
    }

    void reset() { sumCount = 0; }

    void operator()(const uchar **src, uchar *dst, int dststep, int count, int width)
    {
        int i;
        int *SUM;
        bool haveScale = scale != 1;
        double _scale = scale;

        if (width != (int)sum.size())
        {
            sum.resize(width);
            sumCount = 0;
        }
        SUM = &sum[0];
        if (sumCount == 0)
        {
            memset((void *)SUM, 0, width * sizeof(int));
            for (; sumCount < ksize - 1; sumCount++, src++)
            {
                const int *Sp = (const int *)src[0];
                i = 0;
                for (; i < width; i++)
                    SUM[i] += Sp[i];
            }
        }
        else
        {
            CV_Assert(sumCount == ksize - 1);
            src += ksize - 1;
        }

        for (; count--; src++)
        {
            const int *Sp = (const int *)src[0];
            const int *Sm = (const int *)src[1 - ksize];
            ushort *D = (ushort *)dst;
            if (haveScale)
            {
                i = 0;
                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<ushort>(s0 * _scale);
                    SUM[i] = s0 - Sm[i];
                }
            }
            else
            {
                i = 0;

                for (; i < width; i++)
                {
                    int s0 = SUM[i] + Sp[i];
                    D[i] = saturate_cast<ushort>(s0);
                    SUM[i] = s0 - Sm[i];
                }
            }
            dst += dststep;
        }
    }

    double scale;
    int sumCount;
    vector<int> sum;
};
}




/****************************************************************************************\
                                     Gaussian Blur
\****************************************************************************************/

cv::Mat cv::getGaussianKernel(int n, double sigma, int ktype)
{
    const int SMALL_GAUSSIAN_SIZE = 7;
    static const float small_gaussian_tab[][SMALL_GAUSSIAN_SIZE] =
        {
            {1.f},
            {0.25f, 0.5f, 0.25f},
            {0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f},
            {0.03125f, 0.109375f, 0.21875f, 0.28125f, 0.21875f, 0.109375f, 0.03125f}};

    const float *fixed_kernel = n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 ? small_gaussian_tab[n >> 1] : 0;

    CV_Assert(ktype == CV_32F || ktype == CV_64F);
    Mat kernel(n, 1, ktype);
    float *cf = (float *)kernel.data;
    double *cd = (double *)kernel.data;

    double sigmaX = sigma > 0 ? sigma : ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
    double scale2X = -0.5 / (sigmaX * sigmaX);
    double sum = 0;

    int i;
    for (i = 0; i < n; i++)
    {
        double x = i - (n - 1) * 0.5;
        double t = fixed_kernel ? (double)fixed_kernel[i] : std::exp(scale2X * x * x);
        if (ktype == CV_32F)
        {
            cf[i] = (float)t;
            sum += cf[i];
        }
        else
        {
            cd[i] = t;
            sum += cd[i];
        }
    }

    sum = 1. / sum;
    for (i = 0; i < n; i++)
    {
        if (ktype == CV_32F)
            cf[i] = (float)(cf[i] * sum);
        else
            cd[i] *= sum;
    }

    return kernel;
}

cv::Ptr<cv::FilterEngine> cv::createGaussianFilter(int type, Size ksize,
                                                   double sigma1, double sigma2,
                                                   int borderType)
{
    int depth = CV_MAT_DEPTH(type);
    if (sigma2 <= 0)
        sigma2 = sigma1;

    // automatic detection of kernel size from sigma
    if (ksize.width <= 0 && sigma1 > 0)
        ksize.width = cvRound(sigma1 * (depth == CV_8U ? 3 : 4) * 2 + 1) | 1;
    if (ksize.height <= 0 && sigma2 > 0)
        ksize.height = cvRound(sigma2 * (depth == CV_8U ? 3 : 4) * 2 + 1) | 1;

    CV_Assert(ksize.width > 0 && ksize.width % 2 == 1 &&
              ksize.height > 0 && ksize.height % 2 == 1);

    sigma1 = std::max(sigma1, 0.);
    sigma2 = std::max(sigma2, 0.);

    Mat kx = getGaussianKernel(ksize.width, sigma1, std::max(depth, CV_32F));
    Mat ky;
    if (ksize.height == ksize.width && std::abs(sigma1 - sigma2) < DBL_EPSILON)
        ky = kx;
    else
        ky = getGaussianKernel(ksize.height, sigma2, std::max(depth, CV_32F));

    return createSeparableLinearFilter(type, type, kx, ky, Point(-1, -1), 0, borderType);
}

void cv::GaussianBlur(InputArray _src, OutputArray _dst, Size ksize,
                      double sigma1, double sigma2,
                      int borderType)
{
    Mat src = _src.getMat();
    _dst.create(src.size(), src.type());
    Mat dst = _dst.getMat();

    if (borderType != BORDER_CONSTANT && (borderType & BORDER_ISOLATED) != 0)
    {
        if (src.rows == 1)
            ksize.height = 1;
        if (src.cols == 1)
            ksize.width = 1;
    }

    if (ksize.width == 1 && ksize.height == 1)
    {
        src.copyTo(dst);
        return;
    }


    Ptr<FilterEngine> f = createGaussianFilter(src.type(), ksize, sigma1, sigma2, borderType);
    f->apply(src, dst);
}



/****************************************************************************************\
                                   Bilateral Filtering
\****************************************************************************************/

namespace cv
{

class BilateralFilter_8u_Invoker : public ParallelLoopBody
{
  public:
    BilateralFilter_8u_Invoker(Mat &_dest, const Mat &_temp, int _radius, int _maxk,
                               int *_space_ofs, float *_space_weight, float *_color_weight) : temp(&_temp), dest(&_dest), radius(_radius),
                                                                                              maxk(_maxk), space_ofs(_space_ofs), space_weight(_space_weight), color_weight(_color_weight)
    {
    }

    virtual void operator()(const Range &range) const
    {
        int i, j, cn = dest->channels(), k;
        Size size = dest->size();

        for (i = range.start; i < range.end; i++)
        {
            const uchar *sptr = temp->ptr(i + radius) + radius * cn;
            uchar *dptr = dest->ptr(i);

            if (cn == 1)
            {
                for (j = 0; j < size.width; j++)
                {
                    float sum = 0, wsum = 0;
                    int val0 = sptr[j];
                    k = 0;
                    for (; k < maxk; k++)
                    {
                        int val = sptr[j + space_ofs[k]];
                        float w = space_weight[k] * color_weight[std::abs(val - val0)];
                        sum += val * w;
                        wsum += w;
                    }
                    // overflow is not possible here => there is no need to use CV_CAST_8U
                    dptr[j] = (uchar)cvRound(sum / wsum);
                }
            }
            else
            {
                assert(cn == 3);
                for (j = 0; j < size.width * 3; j += 3)
                {
                    float sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                    int b0 = sptr[j], g0 = sptr[j + 1], r0 = sptr[j + 2];
                    k = 0;

                    for (; k < maxk; k++)
                    {
                        const uchar *sptr_k = sptr + j + space_ofs[k];
                        int b = sptr_k[0], g = sptr_k[1], r = sptr_k[2];
                        float w = space_weight[k] * color_weight[std::abs(b - b0) +
                                                                 std::abs(g - g0) + std::abs(r - r0)];
                        sum_b += b * w;
                        sum_g += g * w;
                        sum_r += r * w;
                        wsum += w;
                    }
                    wsum = 1.f / wsum;
                    b0 = cvRound(sum_b * wsum);
                    g0 = cvRound(sum_g * wsum);
                    r0 = cvRound(sum_r * wsum);
                    dptr[j] = (uchar)b0;
                    dptr[j + 1] = (uchar)g0;
                    dptr[j + 2] = (uchar)r0;
                }
            }
        }
    }

  private:
    const Mat *temp;
    Mat *dest;
    int radius, maxk, *space_ofs;
    float *space_weight, *color_weight;
};

#if defined(HAVE_IPP) && (IPP_VERSION_MAJOR >= 7)
class IPPBilateralFilter_8u_Invoker : public ParallelLoopBody
{
  public:
    IPPBilateralFilter_8u_Invoker(Mat &_src, Mat &_dst, double _sigma_color, double _sigma_space, int _radius, bool *_ok) : ParallelLoopBody(), src(_src), dst(_dst), sigma_color(_sigma_color), sigma_space(_sigma_space), radius(_radius), ok(_ok)
    {
        *ok = true;
    }

    virtual void operator()(const Range &range) const
    {
        int d = radius * 2 + 1;
        IppiSize kernel = {d, d};
        IppiSize roi = {dst.cols, range.end - range.start};
        int bufsize = 0;
        ippiFilterBilateralGetBufSize_8u_C1R(ippiFilterBilateralGauss, roi, kernel, &bufsize);
        AutoBuffer<uchar> buf(bufsize);
        IppiFilterBilateralSpec *pSpec = (IppiFilterBilateralSpec *)alignPtr(&buf[0], 32);
        ippiFilterBilateralInit_8u_C1R(ippiFilterBilateralGauss, kernel, (Ipp32f)sigma_color, (Ipp32f)sigma_space, 1, pSpec);
        if (ippiFilterBilateral_8u_C1R(src.ptr<uchar>(range.start) + radius * ((int)src.step[0] + 1), (int)src.step[0], dst.ptr<uchar>(range.start), (int)dst.step[0], roi, kernel, pSpec) < 0)
            *ok = false;
    }

  private:
    Mat &src;
    Mat &dst;
    double sigma_color;
    double sigma_space;
    int radius;
    bool *ok;
    const IPPBilateralFilter_8u_Invoker &operator=(const IPPBilateralFilter_8u_Invoker &);
};
#endif

static void
bilateralFilter_8u(const Mat &src, Mat &dst, int d,
                   double sigma_color, double sigma_space,
                   int borderType)
{

    int cn = src.channels();
    int i, j, maxk, radius;
    Size size = src.size();

    CV_Assert((src.type() == CV_8UC1 || src.type() == CV_8UC3) &&
              src.type() == dst.type() && src.size() == dst.size() &&
              src.data != dst.data);

    if (sigma_color <= 0)
        sigma_color = 1;
    if (sigma_space <= 0)
        sigma_space = 1;

    double gauss_color_coeff = -0.5 / (sigma_color * sigma_color);
    double gauss_space_coeff = -0.5 / (sigma_space * sigma_space);

    if (d <= 0)
        radius = cvRound(sigma_space * 1.5);
    else
        radius = d / 2;
    radius = MAX(radius, 1);
    d = radius * 2 + 1;

    Mat temp;
    copyMakeBorder(src, temp, radius, radius, radius, radius, borderType);

#if defined HAVE_IPP && (IPP_VERSION_MAJOR >= 7)
    if (cn == 1)
    {
        bool ok;
        IPPBilateralFilter_8u_Invoker body(temp, dst, sigma_color * sigma_color, sigma_space * sigma_space, radius, &ok);
        parallel_for_(Range(0, dst.rows), body, dst.total() / (double)(1 << 16));
        if (ok)
            return;
    }
#endif

    vector<float> _color_weight(cn * 256);
    vector<float> _space_weight(d * d);
    vector<int> _space_ofs(d * d);
    float *color_weight = &_color_weight[0];
    float *space_weight = &_space_weight[0];
    int *space_ofs = &_space_ofs[0];

    // initialize color-related bilateral filter coefficients

    for (i = 0; i < 256 * cn; i++)
        color_weight[i] = (float)std::exp(i * i * gauss_color_coeff);

    // initialize space-related bilateral filter coefficients
    for (i = -radius, maxk = 0; i <= radius; i++)
    {
        j = -radius;

        for (; j <= radius; j++)
        {
            double r = std::sqrt((double)i * i + (double)j * j);
            if (r > radius)
                continue;
            space_weight[maxk] = (float)std::exp(r * r * gauss_space_coeff);
            space_ofs[maxk++] = (int)(i * temp.step + j * cn);
        }
    }

    BilateralFilter_8u_Invoker body(dst, temp, radius, maxk, space_ofs, space_weight, color_weight);
    parallel_for_(Range(0, size.height), body, dst.total() / (double)(1 << 16));
}

class BilateralFilter_32f_Invoker : public ParallelLoopBody
{
  public:
    BilateralFilter_32f_Invoker(int _cn, int _radius, int _maxk, int *_space_ofs,
                                const Mat &_temp, Mat &_dest, float _scale_index, float *_space_weight, float *_expLUT) : cn(_cn), radius(_radius), maxk(_maxk), space_ofs(_space_ofs),
                                                                                                                          temp(&_temp), dest(&_dest), scale_index(_scale_index), space_weight(_space_weight), expLUT(_expLUT)
    {
    }

    virtual void operator()(const Range &range) const
    {
        int i, j, k;
        Size size = dest->size();

        for (i = range.start; i < range.end; i++)
        {
            const float *sptr = temp->ptr<float>(i + radius) + radius * cn;
            float *dptr = dest->ptr<float>(i);

            if (cn == 1)
            {
                for (j = 0; j < size.width; j++)
                {
                    float sum = 0, wsum = 0;
                    float val0 = sptr[j];
                    k = 0;

                    for (; k < maxk; k++)
                    {
                        float val = sptr[j + space_ofs[k]];
                        float alpha = (float)(std::abs(val - val0) * scale_index);
                        int idx = cvFloor(alpha);
                        alpha -= idx;
                        float w = space_weight[k] * (expLUT[idx] + alpha * (expLUT[idx + 1] - expLUT[idx]));
                        sum += val * w;
                        wsum += w;
                    }
                    dptr[j] = (float)(sum / wsum);
                }
            }
            else
            {
                assert(cn == 3);
                for (j = 0; j < size.width * 3; j += 3)
                {
                    float sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                    float b0 = sptr[j], g0 = sptr[j + 1], r0 = sptr[j + 2];
                    k = 0;

                    for (; k < maxk; k++)
                    {
                        const float *sptr_k = sptr + j + space_ofs[k];
                        float b = sptr_k[0], g = sptr_k[1], r = sptr_k[2];
                        float alpha = (float)((std::abs(b - b0) +
                                               std::abs(g - g0) + std::abs(r - r0)) *
                                              scale_index);
                        int idx = cvFloor(alpha);
                        alpha -= idx;
                        float w = space_weight[k] * (expLUT[idx] + alpha * (expLUT[idx + 1] - expLUT[idx]));
                        sum_b += b * w;
                        sum_g += g * w;
                        sum_r += r * w;
                        wsum += w;
                    }
                    wsum = 1.f / wsum;
                    b0 = sum_b * wsum;
                    g0 = sum_g * wsum;
                    r0 = sum_r * wsum;
                    dptr[j] = b0;
                    dptr[j + 1] = g0;
                    dptr[j + 2] = r0;
                }
            }
        }
    }

  private:
    int cn, radius, maxk, *space_ofs;
    const Mat *temp;
    Mat *dest;
    float scale_index, *space_weight, *expLUT;
};

static void
bilateralFilter_32f(const Mat &src, Mat &dst, int d,
                    double sigma_color, double sigma_space,
                    int borderType)
{
    int cn = src.channels();
    int i, j, maxk, radius;
    double minValSrc = -1, maxValSrc = 1;
    const int kExpNumBinsPerChannel = 1 << 12;
    int kExpNumBins = 0;
    float lastExpVal = 1.f;
    float len, scale_index;
    Size size = src.size();

    CV_Assert((src.type() == CV_32FC1 || src.type() == CV_32FC3) &&
              src.type() == dst.type() && src.size() == dst.size() &&
              src.data != dst.data);

    if (sigma_color <= 0)
        sigma_color = 1;
    if (sigma_space <= 0)
        sigma_space = 1;

    double gauss_color_coeff = -0.5 / (sigma_color * sigma_color);
    double gauss_space_coeff = -0.5 / (sigma_space * sigma_space);

    if (d <= 0)
        radius = cvRound(sigma_space * 1.5);
    else
        radius = d / 2;
    radius = MAX(radius, 1);
    d = radius * 2 + 1;
    // compute the min/max range for the input image (even if multichannel)

    minMaxLoc(src.reshape(1), &minValSrc, &maxValSrc);
    if (std::abs(minValSrc - maxValSrc) < FLT_EPSILON)
    {
        src.copyTo(dst);
        return;
    }

    // temporary copy of the image with borders for easy processing
    Mat temp;
    copyMakeBorder(src, temp, radius, radius, radius, radius, borderType);
    const double insteadNaNValue = -5. * sigma_color;
    patchNaNs(temp, insteadNaNValue); // this replacement of NaNs makes the assumption that depth values are nonnegative
                                      // TODO: make insteadNaNValue avalible in the outside function interface to control the cases breaking the assumption
    // allocate lookup tables
    vector<float> _space_weight(d * d);
    vector<int> _space_ofs(d * d);
    float *space_weight = &_space_weight[0];
    int *space_ofs = &_space_ofs[0];

    // assign a length which is slightly more than needed
    len = (float)(maxValSrc - minValSrc) * cn;
    kExpNumBins = kExpNumBinsPerChannel * cn;
    vector<float> _expLUT(kExpNumBins + 2);
    float *expLUT = &_expLUT[0];

    scale_index = kExpNumBins / len;

    // initialize the exp LUT
    for (i = 0; i < kExpNumBins + 2; i++)
    {
        if (lastExpVal > 0.f)
        {
            double val = i / scale_index;
            expLUT[i] = (float)std::exp(val * val * gauss_color_coeff);
            lastExpVal = expLUT[i];
        }
        else
            expLUT[i] = 0.f;
    }

    // initialize space-related bilateral filter coefficients
    for (i = -radius, maxk = 0; i <= radius; i++)
        for (j = -radius; j <= radius; j++)
        {
            double r = std::sqrt((double)i * i + (double)j * j);
            if (r > radius)
                continue;
            space_weight[maxk] = (float)std::exp(r * r * gauss_space_coeff);
            space_ofs[maxk++] = (int)(i * (temp.step / sizeof(float)) + j * cn);
        }

    // parallel_for usage

    BilateralFilter_32f_Invoker body(cn, radius, maxk, space_ofs, temp, dst, scale_index, space_weight, expLUT);
    parallel_for_(Range(0, size.height), body, dst.total() / (double)(1 << 16));
}
}

void cv::bilateralFilter(InputArray _src, OutputArray _dst, int d,
                         double sigmaColor, double sigmaSpace,
                         int borderType)
{
    Mat src = _src.getMat();
    _dst.create(src.size(), src.type());
    Mat dst = _dst.getMat();

    if (src.depth() == CV_8U)
        bilateralFilter_8u(src, dst, d, sigmaColor, sigmaSpace, borderType);
    else if (src.depth() == CV_32F)
        bilateralFilter_32f(src, dst, d, sigmaColor, sigmaSpace, borderType);
    else
        CV_Error(CV_StsUnsupportedFormat,
                 "Bilateral filtering is only implemented for 8u and 32f images");
}

/****************************************************************************************\
                                  Adaptive Bilateral Filtering
\****************************************************************************************/

namespace cv
{
#ifndef ABF_CALCVAR
#define ABF_CALCVAR 1
#endif

#ifndef ABF_FIXED_WEIGHT
#define ABF_FIXED_WEIGHT 0
#endif

#ifndef ABF_GAUSSIAN
#define ABF_GAUSSIAN 1
#endif

class adaptiveBilateralFilter_8u_Invoker : public ParallelLoopBody
{
  public:
    adaptiveBilateralFilter_8u_Invoker(Mat &_dest, const Mat &_temp, Size _ksize, double _sigma_space, double _maxSigmaColor, Point _anchor) : temp(&_temp), dest(&_dest), ksize(_ksize), sigma_space(_sigma_space), maxSigma_Color(_maxSigmaColor), anchor(_anchor)
    {
        if (sigma_space <= 0)
            sigma_space = 1;
        CV_Assert((ksize.width & 1) && (ksize.height & 1));
        space_weight.resize(ksize.width * ksize.height);
        double sigma2 = sigma_space * sigma_space;
        int idx = 0;
        int w = ksize.width / 2;
        int h = ksize.height / 2;
        for (int y = -h; y <= h; y++)
            for (int x = -w; x <= w; x++)
            {
#if ABF_GAUSSIAN
                space_weight[idx++] = (float)exp(-0.5 * (x * x + y * y) / sigma2);
#else
                space_weight[idx++] = (float)(sigma2 / (sigma2 + x * x + y * y));
#endif
            }
    }
    virtual void operator()(const Range &range) const
    {
        int cn = dest->channels();
        int anX = anchor.x;

        const uchar *tptr;

        for (int i = range.start; i < range.end; i++)
        {
            int startY = i;
            if (cn == 1)
            {
                float var;
                int currVal;
                int sumVal = 0;
                int sumValSqr = 0;
                int currValCenter;
                int currWRTCenter;
                float weight;
                float totalWeight = 0.;
                float tmpSum = 0.;

                for (int j = 0; j < dest->cols * cn; j += cn)
                {
                    sumVal = 0;
                    sumValSqr = 0;
                    totalWeight = 0.;
                    tmpSum = 0.;

                    // Top row: don't sum the very last element
                    int startLMJ = 0;
                    int endLMJ = ksize.width - 1;
                    int howManyAll = (anX * 2 + 1) * (ksize.width);
#if ABF_CALCVAR
                    for (int x = startLMJ; x < endLMJ; x++)
                    {
                        tptr = temp->ptr(startY + x) + j;
                        for (int y = -anX; y <= anX; y++)
                        {
                            currVal = tptr[cn * (y + anX)];
                            sumVal += currVal;
                            sumValSqr += (currVal * currVal);
                        }
                    }
                    var = ((sumValSqr * howManyAll) - sumVal * sumVal) / ((float)(howManyAll * howManyAll));

                    if (var < 0.01)
                        var = 0.01f;
                    else if (var > (float)(maxSigma_Color * maxSigma_Color))
                        var = (float)(maxSigma_Color * maxSigma_Color);

#else
                    var = maxSigmaColor * maxSigmaColor;
#endif
                    startLMJ = 0;
                    endLMJ = ksize.width;
                    tptr = temp->ptr(startY + (startLMJ + endLMJ) / 2);
                    currValCenter = tptr[j + cn * anX];
                    for (int x = startLMJ; x < endLMJ; x++)
                    {
                        tptr = temp->ptr(startY + x) + j;
                        for (int y = -anX; y <= anX; y++)
                        {
#if ABF_FIXED_WEIGHT
                            weight = 1.0;
#else
                            currVal = tptr[cn * (y + anX)];
                            currWRTCenter = currVal - currValCenter;

#if ABF_GAUSSIAN
                            weight = exp(-0.5f * currWRTCenter * currWRTCenter / var) * space_weight[x * ksize.width + y + anX];
#else
                            weight = var / (var + (currWRTCenter * currWRTCenter)) * space_weight[x * ksize.width + y + anX];
#endif

#endif
                            tmpSum += ((float)tptr[cn * (y + anX)] * weight);
                            totalWeight += weight;
                        }
                    }
                    tmpSum /= totalWeight;

                    dest->at<uchar>(startY, j) = static_cast<uchar>(tmpSum);
                }
            }
            else
            {
                assert(cn == 3);
                float var_b, var_g, var_r;
                int currVal_b, currVal_g, currVal_r;
                int sumVal_b = 0, sumVal_g = 0, sumVal_r = 0;
                int sumValSqr_b = 0, sumValSqr_g = 0, sumValSqr_r = 0;
                int currValCenter_b = 0, currValCenter_g = 0, currValCenter_r = 0;
                int currWRTCenter_b, currWRTCenter_g, currWRTCenter_r;
                float weight_b, weight_g, weight_r;
                float totalWeight_b = 0., totalWeight_g = 0., totalWeight_r = 0.;
                float tmpSum_b = 0., tmpSum_g = 0., tmpSum_r = 0.;

                for (int j = 0; j < dest->cols * cn; j += cn)
                {
                    sumVal_b = 0, sumVal_g = 0, sumVal_r = 0;
                    sumValSqr_b = 0, sumValSqr_g = 0, sumValSqr_r = 0;
                    totalWeight_b = 0., totalWeight_g = 0., totalWeight_r = 0.;
                    tmpSum_b = 0., tmpSum_g = 0., tmpSum_r = 0.;

                    // Top row: don't sum the very last element
                    int startLMJ = 0;
                    int endLMJ = ksize.width - 1;
                    int howManyAll = (anX * 2 + 1) * (ksize.width);
#if ABF_CALCVAR
                    float max_var = (float)(maxSigma_Color * maxSigma_Color);
                    for (int x = startLMJ; x < endLMJ; x++)
                    {
                        tptr = temp->ptr(startY + x) + j;
                        for (int y = -anX; y <= anX; y++)
                        {
                            currVal_b = tptr[cn * (y + anX)], currVal_g = tptr[cn * (y + anX) + 1], currVal_r = tptr[cn * (y + anX) + 2];
                            sumVal_b += currVal_b;
                            sumVal_g += currVal_g;
                            sumVal_r += currVal_r;
                            sumValSqr_b += (currVal_b * currVal_b);
                            sumValSqr_g += (currVal_g * currVal_g);
                            sumValSqr_r += (currVal_r * currVal_r);
                        }
                    }
                    var_b = ((sumValSqr_b * howManyAll) - sumVal_b * sumVal_b) / ((float)(howManyAll * howManyAll));
                    var_g = ((sumValSqr_g * howManyAll) - sumVal_g * sumVal_g) / ((float)(howManyAll * howManyAll));
                    var_r = ((sumValSqr_r * howManyAll) - sumVal_r * sumVal_r) / ((float)(howManyAll * howManyAll));

                    if (var_b < 0.01)
                        var_b = 0.01f;
                    else if (var_b > max_var)
                        var_b = (float)(max_var);

                    if (var_g < 0.01)
                        var_g = 0.01f;
                    else if (var_g > max_var)
                        var_g = (float)(max_var);

                    if (var_r < 0.01)
                        var_r = 0.01f;
                    else if (var_r > max_var)
                        var_r = (float)(max_var);

#else
                    var_b = maxSigma_Color * maxSigma_Color;
                    var_g = maxSigma_Color * maxSigma_Color;
                    var_r = maxSigma_Color * maxSigma_Color;
#endif
                    startLMJ = 0;
                    endLMJ = ksize.width;
                    tptr = temp->ptr(startY + (startLMJ + endLMJ) / 2) + j;
                    currValCenter_b = tptr[cn * anX], currValCenter_g = tptr[cn * anX + 1], currValCenter_r = tptr[cn * anX + 2];
                    for (int x = startLMJ; x < endLMJ; x++)
                    {
                        tptr = temp->ptr(startY + x) + j;
                        for (int y = -anX; y <= anX; y++)
                        {
#if ABF_FIXED_WEIGHT
                            weight_b = 1.0;
                            weight_g = 1.0;
                            weight_r = 1.0;
#else
                            currVal_b = tptr[cn * (y + anX)];
                            currVal_g = tptr[cn * (y + anX) + 1];
                            currVal_r = tptr[cn * (y + anX) + 2];
                            currWRTCenter_b = currVal_b - currValCenter_b;
                            currWRTCenter_g = currVal_g - currValCenter_g;
                            currWRTCenter_r = currVal_r - currValCenter_r;

                            float cur_spw = space_weight[x * ksize.width + y + anX];

#if ABF_GAUSSIAN
                            weight_b = exp(-0.5f * currWRTCenter_b * currWRTCenter_b / var_b) * cur_spw;
                            weight_g = exp(-0.5f * currWRTCenter_g * currWRTCenter_g / var_g) * cur_spw;
                            weight_r = exp(-0.5f * currWRTCenter_r * currWRTCenter_r / var_r) * cur_spw;
#else
                            weight_b = var_b / (var_b + (currWRTCenter_b * currWRTCenter_b)) * cur_spw;
                            weight_g = var_g / (var_g + (currWRTCenter_g * currWRTCenter_g)) * cur_spw;
                            weight_r = var_r / (var_r + (currWRTCenter_r * currWRTCenter_r)) * cur_spw;
#endif
#endif
                            tmpSum_b += ((float)tptr[cn * (y + anX)] * weight_b);
                            tmpSum_g += ((float)tptr[cn * (y + anX) + 1] * weight_g);
                            tmpSum_r += ((float)tptr[cn * (y + anX) + 2] * weight_r);
                            totalWeight_b += weight_b, totalWeight_g += weight_g, totalWeight_r += weight_r;
                        }
                    }
                    tmpSum_b /= totalWeight_b;
                    tmpSum_g /= totalWeight_g;
                    tmpSum_r /= totalWeight_r;

                    dest->at<uchar>(startY, j) = static_cast<uchar>(tmpSum_b);
                    dest->at<uchar>(startY, j + 1) = static_cast<uchar>(tmpSum_g);
                    dest->at<uchar>(startY, j + 2) = static_cast<uchar>(tmpSum_r);
                }
            }
        }
    }

  private:
    const Mat *temp;
    Mat *dest;
    Size ksize;
    double sigma_space;
    double maxSigma_Color;
    Point anchor;
    vector<float> space_weight;
};
static void adaptiveBilateralFilter_8u(const Mat &src, Mat &dst, Size ksize, double sigmaSpace, double maxSigmaColor, Point anchor, int borderType)
{
    Size size = src.size();

    CV_Assert((src.type() == CV_8UC1 || src.type() == CV_8UC3) &&
              src.type() == dst.type() && src.size() == dst.size() &&
              src.data != dst.data);
    Mat temp;
    copyMakeBorder(src, temp, anchor.x, anchor.y, anchor.x, anchor.y, borderType);

    adaptiveBilateralFilter_8u_Invoker body(dst, temp, ksize, sigmaSpace, maxSigmaColor, anchor);
    parallel_for_(Range(0, size.height), body, dst.total() / (double)(1 << 16));
}
}
void cv::adaptiveBilateralFilter(InputArray _src, OutputArray _dst, Size ksize,
                                 double sigmaSpace, double maxSigmaColor, Point anchor, int borderType)
{
    Mat src = _src.getMat();
    _dst.create(src.size(), src.type());
    Mat dst = _dst.getMat();

    CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3);

    anchor = normalizeAnchor(anchor, ksize);
    if (src.depth() == CV_8U)
        adaptiveBilateralFilter_8u(src, dst, ksize, sigmaSpace, maxSigmaColor, anchor, borderType);
    else
        CV_Error(CV_StsUnsupportedFormat,
                 "Adaptive Bilateral filtering is only implemented for 8u images");
}

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvSmooth(const void *srcarr, void *dstarr, int smooth_type,
         int param1, int param2, double param3, double param4)
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst0 = cv::cvarrToMat(dstarr), dst = dst0;

    CV_Assert(dst.size() == src.size() &&
              (smooth_type == CV_BLUR_NO_SCALE || dst.type() == src.type()));

    if (param2 <= 0)
        param2 = param1;

    if (smooth_type == CV_GAUSSIAN)
        cv::GaussianBlur(src, dst, cv::Size(param1, param2), param3, param4, cv::BORDER_REPLICATE);
    else
        cv::bilateralFilter(src, dst, param1, param3, param4, cv::BORDER_REPLICATE);

    if (dst.data != dst0.data)
        CV_Error(CV_StsUnmatchedFormats, "The destination image does not have the proper type");
}

/* End of file. */

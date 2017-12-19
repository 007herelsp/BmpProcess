#include "imgproc.precomp.hpp"
#include <opencv2/imgproc/imgproc.hpp>

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
		assert("herelsp remove" && 0);
       // cv::bilateralFilter(src, dst, param1, param3, param4, cv::BORDER_REPLICATE);

    if (dst.data != dst0.data)
        CV_Error(CV_StsUnmatchedFormats, "The destination image does not have the proper type");
}

/* End of file. */

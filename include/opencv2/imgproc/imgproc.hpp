#ifndef __OPENCV_IMGPROC_HPP__
#define __OPENCV_IMGPROC_HPP__

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/types_c.h"

#ifdef __cplusplus

/*! \namespace cv
 Namespace where all the C++ OpenCV functionality resides
 */
namespace cv
{

//! various border interpolation methods
enum { BORDER_REPLICATE=IPL_BORDER_REPLICATE, BORDER_CONSTANT=IPL_BORDER_CONSTANT,
       BORDER_REFLECT=IPL_BORDER_REFLECT, BORDER_WRAP=IPL_BORDER_WRAP,
       BORDER_REFLECT_101=IPL_BORDER_REFLECT_101, BORDER_REFLECT101=BORDER_REFLECT_101,
       BORDER_TRANSPARENT=IPL_BORDER_TRANSPARENT,
       BORDER_DEFAULT=BORDER_REFLECT_101, BORDER_ISOLATED=16 };

//! 1D interpolation function: returns coordinate of the "donor" pixel for the specified location p.
CV_EXPORTS_W int borderInterpolate( int p, int len, int borderType );

/*!
 The Base Class for 1D or Row-wise Filters

 This is the base class for linear or non-linear filters that process 1D data.
 In particular, such filters are used for the "horizontal" filtering parts in separable filters.

 Several functions in OpenCV return Ptr<BaseRowFilter> for the specific types of filters,
 and those pointers can be used directly or within cv::FilterEngine.
*/
class CV_EXPORTS BaseRowFilter
{
public:
    //! the default constructor
    BaseRowFilter();
    //! the destructor
    virtual ~BaseRowFilter();
    //! the filtering operator. Must be overridden in the derived classes. The horizontal border interpolation is done outside of the class.
    virtual void operator()(const uchar* src, uchar* dst,
                            int width, int cn) = 0;
    int ksize, anchor;
};


/*!
 The Base Class for Column-wise Filters

 This is the base class for linear or non-linear filters that process columns of 2D arrays.
 Such filters are used for the "vertical" filtering parts in separable filters.

 Several functions in OpenCV return Ptr<BaseColumnFilter> for the specific types of filters,
 and those pointers can be used directly or within cv::FilterEngine.

 Unlike cv::BaseRowFilter, cv::BaseColumnFilter may have some context information,
 i.e. box filter keeps the sliding sum of elements. To reset the state BaseColumnFilter::reset()
 must be called (e.g. the method is called by cv::FilterEngine)
 */
class CV_EXPORTS BaseColumnFilter
{
public:
    //! the default constructor
    BaseColumnFilter();
    //! the destructor
    virtual ~BaseColumnFilter();
    //! the filtering operator. Must be overridden in the derived classes. The vertical border interpolation is done outside of the class.
    virtual void operator()(const uchar** src, uchar* dst, int dststep,
                            int dstcount, int width) = 0;
    //! resets the internal buffers, if any
    virtual void reset();
    int ksize, anchor;
};

/*!
 The Base Class for Non-Separable 2D Filters.

 This is the base class for linear or non-linear 2D filters.

 Several functions in OpenCV return Ptr<BaseFilter> for the specific types of filters,
 and those pointers can be used directly or within cv::FilterEngine.

 Similar to cv::BaseColumnFilter, the class may have some context information,
 that should be reset using BaseFilter::reset() method before processing the new array.
*/
class CV_EXPORTS BaseFilter
{
public:
    //! the default constructor
    BaseFilter();
    //! the destructor
    virtual ~BaseFilter();
    //! the filtering operator. The horizontal and the vertical border interpolation is done outside of the class.
    virtual void operator()(const uchar** src, uchar* dst, int dststep,
                            int dstcount, int width, int cn) = 0;
    //! resets the internal buffers, if any
    virtual void reset();
    Size ksize;
    Point anchor;
};

class CV_EXPORTS FilterEngine
{
public:
    //! the default constructor
    FilterEngine();
    //! the full constructor. Either _filter2D or both _rowFilter and _columnFilter must be non-empty.
    FilterEngine(const Ptr<BaseFilter>& _filter2D,
                 const Ptr<BaseRowFilter>& _rowFilter,
                 const Ptr<BaseColumnFilter>& _columnFilter,
                 int srcType, int dstType, int bufType,
                 int _rowBorderType=BORDER_REPLICATE,
                 int _columnBorderType=-1,
                 const Scalar& _borderValue=Scalar());
    //! the destructor
    virtual ~FilterEngine();
    //! reinitializes the engine. The previously assigned filters are released.
    void init(const Ptr<BaseFilter>& _filter2D,
              const Ptr<BaseRowFilter>& _rowFilter,
              const Ptr<BaseColumnFilter>& _columnFilter,
              int srcType, int dstType, int bufType,
              int _rowBorderType=BORDER_REPLICATE, int _columnBorderType=-1,
              const Scalar& _borderValue=Scalar());
    //! starts filtering of the specified ROI of an image of size wholeSize.
    virtual int start(Size wholeSize, Rect roi, int maxBufRows=-1);
    //! starts filtering of the specified ROI of the specified image.
    virtual int start(const Mat& src, const Rect& srcRoi=Rect(0,0,-1,-1),
                      bool isolated=false, int maxBufRows=-1);
    //! processes the next srcCount rows of the image.
    virtual int proceed(const uchar* src, int srcStep, int srcCount,
                        uchar* dst, int dstStep);
    //! applies filter to the specified ROI of the image. if srcRoi=(0,0,-1,-1), the whole image is filtered.
    virtual void apply( const Mat& src, Mat& dst,
                        const Rect& srcRoi=Rect(0,0,-1,-1),
                        Point dstOfs=Point(0,0),
                        bool isolated=false);
    //! returns true if the filter is separable
    bool isSeparable() const { return (const BaseFilter*)filter2D == 0; }
    //! returns the number
    int remainingInputRows() const;
    int remainingOutputRows() const;

    int srcType, dstType, bufType;
    Size ksize;
    Point anchor;
    int maxWidth;
    Size wholeSize;
    Rect roi;
    int dx1, dx2;
    int rowBorderType, columnBorderType;
    vector<int> borderTab;
    int borderElemSize;
    vector<uchar> ringBuf;
    vector<uchar> srcRow;
    vector<uchar> constBorderValue;
    vector<uchar> constBorderRow;
    int bufStep, startY, startY0, endY, rowCount, dstY;
    vector<uchar*> rows;

    Ptr<BaseFilter> filter2D;
    Ptr<BaseRowFilter> rowFilter;
    Ptr<BaseColumnFilter> columnFilter;
};

//! type of the kernel
enum { KERNEL_GENERAL=0, KERNEL_SYMMETRICAL=1, KERNEL_ASYMMETRICAL=2,
       KERNEL_SMOOTH=4, KERNEL_INTEGER=8 };

//! returns type (one of KERNEL_*) of 1D or 2D kernel specified by its coefficients.
CV_EXPORTS int getKernelType(InputArray kernel, Point anchor);

//! returns the primitive row filter with the specified kernel
CV_EXPORTS Ptr<BaseRowFilter> getLinearRowFilter(int srcType, int bufType,
                                            InputArray kernel, int anchor,
                                            int symmetryType);

//! returns the primitive column filter with the specified kernel
CV_EXPORTS Ptr<BaseColumnFilter> getLinearColumnFilter(int bufType, int dstType,
                                            InputArray kernel, int anchor,
                                            int symmetryType, double delta=0,
                                            int bits=0);

//! returns 2D filter with the specified kernel
CV_EXPORTS Ptr<BaseFilter> getLinearFilter(int srcType, int dstType,
                                           InputArray kernel,
                                           Point anchor=Point(-1,-1),
                                           double delta=0, int bits=0);

//! returns the separable linear filter engine
CV_EXPORTS Ptr<FilterEngine> createSeparableLinearFilter(int srcType, int dstType,
                          InputArray rowKernel, InputArray columnKernel,
                          Point anchor=Point(-1,-1), double delta=0,
                          int rowBorderType=BORDER_DEFAULT,
                          int columnBorderType=-1,
                          const Scalar& borderValue=Scalar());

//! returns the non-separable linear filter engine
CV_EXPORTS Ptr<FilterEngine> createLinearFilter(int srcType, int dstType,
                 InputArray kernel, Point _anchor=Point(-1,-1),
                 double delta=0, int rowBorderType=BORDER_DEFAULT,
                 int columnBorderType=-1, const Scalar& borderValue=Scalar());

//! returns the Gaussian kernel with the specified parameters
CV_EXPORTS_W Mat getGaussianKernel( int ksize, double sigma, int ktype=CV_64F );

//! returns the Gaussian filter engine
CV_EXPORTS Ptr<FilterEngine> createGaussianFilter( int type, Size ksize,
                                    double sigma1, double sigma2=0,
                                    int borderType=BORDER_DEFAULT);
//! initializes kernels of the generalized Sobel operator
CV_EXPORTS_W void getDerivKernels( OutputArray kx, OutputArray ky,
                                   int dx, int dy, int ksize,
                                   bool normalize=false, int ktype=CV_32F );


//! returns the Gabor kernel with the specified parameters
CV_EXPORTS_W Mat getGaborKernel( Size ksize, double sigma, double theta, double lambd,
                                 double gamma, double psi=CV_PI*0.5, int ktype=CV_64F );

//! type of morphological operation
enum { MORPH_ERODE=CV_MOP_ERODE, MORPH_DILATE=CV_MOP_DILATE,
       MORPH_OPEN=CV_MOP_OPEN, MORPH_CLOSE=CV_MOP_CLOSE,
       MORPH_GRADIENT=CV_MOP_GRADIENT, MORPH_TOPHAT=CV_MOP_TOPHAT,
       MORPH_BLACKHAT=CV_MOP_BLACKHAT, MORPH_HITMISS };

//! returns horizontal 1D morphological filter
CV_EXPORTS Ptr<BaseRowFilter> getMorphologyRowFilter(int op, int type, int ksize, int anchor=-1);
//! returns vertical 1D morphological filter
CV_EXPORTS Ptr<BaseColumnFilter> getMorphologyColumnFilter(int op, int type, int ksize, int anchor=-1);
//! returns 2D morphological filter
CV_EXPORTS Ptr<BaseFilter> getMorphologyFilter(int op, int type, InputArray kernel,
                                               Point anchor=Point(-1,-1));

//! returns "magic" border value for erosion and dilation. It is automatically transformed to Scalar::all(-DBL_MAX) for dilation.
static inline Scalar morphologyDefaultBorderValue() { return Scalar::all(DBL_MAX); }

//! returns morphological filter engine. Only MORPH_ERODE and MORPH_DILATE are supported.
CV_EXPORTS Ptr<FilterEngine> createMorphologyFilter(int op, int type, InputArray kernel,
                    Point anchor=Point(-1,-1), int rowBorderType=BORDER_CONSTANT,
                    int columnBorderType=-1,
                    const Scalar& borderValue=morphologyDefaultBorderValue());

//! shape of the structuring element
enum { MORPH_RECT=0, MORPH_CROSS=1, MORPH_ELLIPSE=2 };
//! returns structuring element of the specified shape and size
CV_EXPORTS_W Mat getStructuringElement(int shape, Size ksize, Point anchor=Point(-1,-1));

template<> CV_EXPORTS void Ptr<IplConvKernel>::delete_obj();

//! copies 2D array to a larger destination array with extrapolation of the outer part of src using the specified border mode
CV_EXPORTS_W void copyMakeBorder( InputArray src, OutputArray dst,
                                int top, int bottom, int left, int right,
                                int borderType, const Scalar& value=Scalar() );

//! smooths the image using median filter.
CV_EXPORTS_W void medianBlur( InputArray src, OutputArray dst, int ksize );
//! smooths the image using Gaussian filter.
CV_EXPORTS_W void GaussianBlur( InputArray src,
                                               OutputArray dst, Size ksize,
                                               double sigmaX, double sigmaY=0,
                                               int borderType=BORDER_DEFAULT );
//! smooths the image using bilateral filter
CV_EXPORTS_W void bilateralFilter( InputArray src, OutputArray dst, int d,
                                   double sigmaColor, double sigmaSpace,
                                   int borderType=BORDER_DEFAULT );
//! smooths the image using adaptive bilateral filter
CV_EXPORTS_W void adaptiveBilateralFilter( InputArray src, OutputArray dst, Size ksize,
                                           double sigmaSpace, double maxSigmaColor = 20.0, Point anchor=Point(-1, -1),
                                           int borderType=BORDER_DEFAULT );



//! applies non-separable 2D linear filter to the image
CV_EXPORTS_W void filter2D( InputArray src, OutputArray dst, int ddepth,
                            InputArray kernel, Point anchor=Point(-1,-1),
                            double delta=0, int borderType=BORDER_DEFAULT );

//! applies separable 2D linear filter to the image
CV_EXPORTS_W void sepFilter2D( InputArray src, OutputArray dst, int ddepth,
                               InputArray kernelX, InputArray kernelY,
                               Point anchor=Point(-1,-1),
                               double delta=0, int borderType=BORDER_DEFAULT );

//! applies generalized Sobel operator to the image
CV_EXPORTS_W void Sobel( InputArray src, OutputArray dst, int ddepth,
                         int dx, int dy, int ksize=3,
                         double scale=1, double delta=0,
                         int borderType=BORDER_DEFAULT );




//! applies Canny edge detector and produces the edge map.
CV_EXPORTS_W void Canny( InputArray image, OutputArray edges,
                         double threshold1, double threshold2,
                         int apertureSize=3, bool L2gradient=false );





//! erodes the image (applies the local minimum operator)
CV_EXPORTS_W void erode( InputArray src, OutputArray dst, InputArray kernel,
                         Point anchor=Point(-1,-1), int iterations=1,
                         int borderType=BORDER_CONSTANT,
                         const Scalar& borderValue=morphologyDefaultBorderValue() );

//! dilates the image (applies the local maximum operator)
CV_EXPORTS_W void dilate( InputArray src, OutputArray dst, InputArray kernel,
                          Point anchor=Point(-1,-1), int iterations=1,
                          int borderType=BORDER_CONSTANT,
                          const Scalar& borderValue=morphologyDefaultBorderValue() );



//! interpolation algorithm
enum
{
    INTER_NEAREST=CV_INTER_NN, //!< nearest neighbor interpolation
    INTER_LINEAR=CV_INTER_LINEAR, //!< bilinear interpolation
    INTER_CUBIC=CV_INTER_CUBIC, //!< bicubic interpolation
    INTER_AREA=CV_INTER_AREA, //!< area-based (or super) interpolation
    INTER_LANCZOS4=CV_INTER_LANCZOS4, //!< Lanczos interpolation over 8x8 neighborhood
    INTER_MAX=7,
    WARP_INVERSE_MAP=CV_WARP_INVERSE_MAP
};

//! resizes the image
CV_EXPORTS_W void resize( InputArray src, OutputArray dst,
                          Size dsize, double fx=0, double fy=0,
                          int interpolation=INTER_LINEAR );



//! warps the image using perspective transformation
CV_EXPORTS_W void warpPerspective( InputArray src, OutputArray dst,
                                   InputArray M, Size dsize,
                                   int flags=INTER_LINEAR,
                                   int borderMode=BORDER_CONSTANT,
                                   const Scalar& borderValue=Scalar());

enum
{
    INTER_BITS=5, INTER_BITS2=INTER_BITS*2,
    INTER_TAB_SIZE=(1<<INTER_BITS),
    INTER_TAB_SIZE2=INTER_TAB_SIZE*INTER_TAB_SIZE
};

//! warps the image using the precomputed maps. The maps are stored in either floating-point or integer fixed-point format
CV_EXPORTS_W void remap( InputArray src, OutputArray dst,
                         InputArray map1, InputArray map2,
                         int interpolation, int borderMode=BORDER_CONSTANT,
                         const Scalar& borderValue=Scalar());




//! returns 3x3 perspective transformation for the corresponding 4 point pairs.
CV_EXPORTS Mat getPerspectiveTransform( const Point2f src[], const Point2f dst[] );
//! returns 2x3 affine transformation for the corresponding 3 point pairs.
CV_EXPORTS Mat getAffineTransform( const Point2f src[], const Point2f dst[] );


CV_EXPORTS_W Mat getPerspectiveTransform( InputArray src, InputArray dst );
CV_EXPORTS_W Mat getAffineTransform( InputArray src, InputArray dst );



//! computes the integral image
CV_EXPORTS_W void integral( InputArray src, OutputArray sum, int sdepth=-1 );

//! computes the integral image and integral for the squared image
CV_EXPORTS_AS(integral2) void integral( InputArray src, OutputArray sum,
                                        OutputArray sqsum, int sdepth=-1 );
//! computes the integral image, integral for the squared image and the tilted integral image
CV_EXPORTS_AS(integral3) void integral( InputArray src, OutputArray sum,
                                        OutputArray sqsum, OutputArray tilted,
                                        int sdepth=-1 );





//! type of the threshold operation
enum { THRESH_BINARY=CV_THRESH_BINARY, THRESH_BINARY_INV=CV_THRESH_BINARY_INV,
       THRESH_TRUNC=CV_THRESH_TRUNC, THRESH_TOZERO=CV_THRESH_TOZERO,
       THRESH_TOZERO_INV=CV_THRESH_TOZERO_INV, THRESH_MASK=CV_THRESH_MASK,
       THRESH_OTSU=CV_THRESH_OTSU };

//! applies fixed threshold to the image
CV_EXPORTS_W double threshold( InputArray src, OutputArray dst,
                               double thresh, double maxval, int type );

enum
{
    COLOR_BGR2BGRA    =0,
    COLOR_RGB2RGBA    =COLOR_BGR2BGRA,

    COLOR_BGRA2BGR    =1,
    COLOR_RGBA2RGB    =COLOR_BGRA2BGR,

    COLOR_BGR2RGBA    =2,
    COLOR_RGB2BGRA    =COLOR_BGR2RGBA,

    COLOR_RGBA2BGR    =3,
    COLOR_BGRA2RGB    =COLOR_RGBA2BGR,

    COLOR_BGR2RGB     =4,
    COLOR_RGB2BGR     =COLOR_BGR2RGB,

    COLOR_BGRA2RGBA   =5,
    COLOR_RGBA2BGRA   =COLOR_BGRA2RGBA,

    COLOR_BGR2GRAY    =6,
    COLOR_RGB2GRAY    =7,
    COLOR_GRAY2BGR    =8,
    COLOR_GRAY2RGB    =COLOR_GRAY2BGR,
    COLOR_GRAY2BGRA   =9,
    COLOR_GRAY2RGBA   =COLOR_GRAY2BGRA,
    COLOR_BGRA2GRAY   =10,
    COLOR_RGBA2GRAY   =11,

    COLOR_BGR2BGR565  =12,
    COLOR_RGB2BGR565  =13,
    COLOR_BGR5652BGR  =14,
    COLOR_BGR5652RGB  =15,
    COLOR_BGRA2BGR565 =16,
    COLOR_RGBA2BGR565 =17,
    COLOR_BGR5652BGRA =18,
    COLOR_BGR5652RGBA =19,

    COLOR_GRAY2BGR565 =20,
    COLOR_BGR5652GRAY =21,

    COLOR_BGR2BGR555  =22,
    COLOR_RGB2BGR555  =23,
    COLOR_BGR5552BGR  =24,
    COLOR_BGR5552RGB  =25,
    COLOR_BGRA2BGR555 =26,
    COLOR_RGBA2BGR555 =27,
    COLOR_BGR5552BGRA =28,
    COLOR_BGR5552RGBA =29,

    COLOR_GRAY2BGR555 =30,
    COLOR_BGR5552GRAY =31,

    COLOR_BGR2XYZ     =32,
    COLOR_RGB2XYZ     =33,
    COLOR_XYZ2BGR     =34,
    COLOR_XYZ2RGB     =35,

    COLOR_BGR2YCrCb   =36,
    COLOR_RGB2YCrCb   =37,
    COLOR_YCrCb2BGR   =38,
    COLOR_YCrCb2RGB   =39,

    COLOR_BGR2HSV     =40,
    COLOR_RGB2HSV     =41,

    COLOR_BGR2Lab     =44,
    COLOR_RGB2Lab     =45,

    COLOR_BayerBG2BGR =46,
    COLOR_BayerGB2BGR =47,
    COLOR_BayerRG2BGR =48,
    COLOR_BayerGR2BGR =49,

    COLOR_BayerBG2RGB =COLOR_BayerRG2BGR,
    COLOR_BayerGB2RGB =COLOR_BayerGR2BGR,
    COLOR_BayerRG2RGB =COLOR_BayerBG2BGR,
    COLOR_BayerGR2RGB =COLOR_BayerGB2BGR,

    COLOR_BGR2Luv     =50,
    COLOR_RGB2Luv     =51,
    COLOR_BGR2HLS     =52,
    COLOR_RGB2HLS     =53,

    COLOR_HSV2BGR     =54,
    COLOR_HSV2RGB     =55,

    COLOR_Lab2BGR     =56,
    COLOR_Lab2RGB     =57,
    COLOR_Luv2BGR     =58,
    COLOR_Luv2RGB     =59,
    COLOR_HLS2BGR     =60,
    COLOR_HLS2RGB     =61,

    COLOR_BayerBG2BGR_VNG =62,
    COLOR_BayerGB2BGR_VNG =63,
    COLOR_BayerRG2BGR_VNG =64,
    COLOR_BayerGR2BGR_VNG =65,

    COLOR_BayerBG2RGB_VNG =COLOR_BayerRG2BGR_VNG,
    COLOR_BayerGB2RGB_VNG =COLOR_BayerGR2BGR_VNG,
    COLOR_BayerRG2RGB_VNG =COLOR_BayerBG2BGR_VNG,
    COLOR_BayerGR2RGB_VNG =COLOR_BayerGB2BGR_VNG,

    COLOR_BGR2HSV_FULL = 66,
    COLOR_RGB2HSV_FULL = 67,
    COLOR_BGR2HLS_FULL = 68,
    COLOR_RGB2HLS_FULL = 69,

    COLOR_HSV2BGR_FULL = 70,
    COLOR_HSV2RGB_FULL = 71,
    COLOR_HLS2BGR_FULL = 72,
    COLOR_HLS2RGB_FULL = 73,

    COLOR_LBGR2Lab     = 74,
    COLOR_LRGB2Lab     = 75,
    COLOR_LBGR2Luv     = 76,
    COLOR_LRGB2Luv     = 77,

    COLOR_Lab2LBGR     = 78,
    COLOR_Lab2LRGB     = 79,
    COLOR_Luv2LBGR     = 80,
    COLOR_Luv2LRGB     = 81,

    COLOR_BGR2YUV      = 82,
    COLOR_RGB2YUV      = 83,
    COLOR_YUV2BGR      = 84,
    COLOR_YUV2RGB      = 85,

    COLOR_BayerBG2GRAY = 86,
    COLOR_BayerGB2GRAY = 87,
    COLOR_BayerRG2GRAY = 88,
    COLOR_BayerGR2GRAY = 89,

    //YUV 4:2:0 formats family
    COLOR_YUV2RGB_NV12 = 90,
    COLOR_YUV2BGR_NV12 = 91,
    COLOR_YUV2RGB_NV21 = 92,
    COLOR_YUV2BGR_NV21 = 93,
    COLOR_YUV420sp2RGB = COLOR_YUV2RGB_NV21,
    COLOR_YUV420sp2BGR = COLOR_YUV2BGR_NV21,

    COLOR_YUV2RGBA_NV12 = 94,
    COLOR_YUV2BGRA_NV12 = 95,
    COLOR_YUV2RGBA_NV21 = 96,
    COLOR_YUV2BGRA_NV21 = 97,
    COLOR_YUV420sp2RGBA = COLOR_YUV2RGBA_NV21,
    COLOR_YUV420sp2BGRA = COLOR_YUV2BGRA_NV21,

    COLOR_YUV2RGB_YV12 = 98,
    COLOR_YUV2BGR_YV12 = 99,
    COLOR_YUV2RGB_IYUV = 100,
    COLOR_YUV2BGR_IYUV = 101,
    COLOR_YUV2RGB_I420 = COLOR_YUV2RGB_IYUV,
    COLOR_YUV2BGR_I420 = COLOR_YUV2BGR_IYUV,
    COLOR_YUV420p2RGB = COLOR_YUV2RGB_YV12,
    COLOR_YUV420p2BGR = COLOR_YUV2BGR_YV12,

    COLOR_YUV2RGBA_YV12 = 102,
    COLOR_YUV2BGRA_YV12 = 103,
    COLOR_YUV2RGBA_IYUV = 104,
    COLOR_YUV2BGRA_IYUV = 105,
    COLOR_YUV2RGBA_I420 = COLOR_YUV2RGBA_IYUV,
    COLOR_YUV2BGRA_I420 = COLOR_YUV2BGRA_IYUV,
    COLOR_YUV420p2RGBA = COLOR_YUV2RGBA_YV12,
    COLOR_YUV420p2BGRA = COLOR_YUV2BGRA_YV12,

    COLOR_YUV2GRAY_420 = 106,
    COLOR_YUV2GRAY_NV21 = COLOR_YUV2GRAY_420,
    COLOR_YUV2GRAY_NV12 = COLOR_YUV2GRAY_420,
    COLOR_YUV2GRAY_YV12 = COLOR_YUV2GRAY_420,
    COLOR_YUV2GRAY_IYUV = COLOR_YUV2GRAY_420,
    COLOR_YUV2GRAY_I420 = COLOR_YUV2GRAY_420,
    COLOR_YUV420sp2GRAY = COLOR_YUV2GRAY_420,
    COLOR_YUV420p2GRAY = COLOR_YUV2GRAY_420,

    //YUV 4:2:2 formats family
    COLOR_YUV2RGB_UYVY = 107,
    COLOR_YUV2BGR_UYVY = 108,
    //COLOR_YUV2RGB_VYUY = 109,
    //COLOR_YUV2BGR_VYUY = 110,
    COLOR_YUV2RGB_Y422 = COLOR_YUV2RGB_UYVY,
    COLOR_YUV2BGR_Y422 = COLOR_YUV2BGR_UYVY,
    COLOR_YUV2RGB_UYNV = COLOR_YUV2RGB_UYVY,
    COLOR_YUV2BGR_UYNV = COLOR_YUV2BGR_UYVY,

    COLOR_YUV2RGBA_UYVY = 111,
    COLOR_YUV2BGRA_UYVY = 112,
    //COLOR_YUV2RGBA_VYUY = 113,
    //COLOR_YUV2BGRA_VYUY = 114,
    COLOR_YUV2RGBA_Y422 = COLOR_YUV2RGBA_UYVY,
    COLOR_YUV2BGRA_Y422 = COLOR_YUV2BGRA_UYVY,
    COLOR_YUV2RGBA_UYNV = COLOR_YUV2RGBA_UYVY,
    COLOR_YUV2BGRA_UYNV = COLOR_YUV2BGRA_UYVY,

    COLOR_YUV2RGB_YUY2 = 115,
    COLOR_YUV2BGR_YUY2 = 116,
    COLOR_YUV2RGB_YVYU = 117,
    COLOR_YUV2BGR_YVYU = 118,
    COLOR_YUV2RGB_YUYV = COLOR_YUV2RGB_YUY2,
    COLOR_YUV2BGR_YUYV = COLOR_YUV2BGR_YUY2,
    COLOR_YUV2RGB_YUNV = COLOR_YUV2RGB_YUY2,
    COLOR_YUV2BGR_YUNV = COLOR_YUV2BGR_YUY2,

    COLOR_YUV2RGBA_YUY2 = 119,
    COLOR_YUV2BGRA_YUY2 = 120,
    COLOR_YUV2RGBA_YVYU = 121,
    COLOR_YUV2BGRA_YVYU = 122,
    COLOR_YUV2RGBA_YUYV = COLOR_YUV2RGBA_YUY2,
    COLOR_YUV2BGRA_YUYV = COLOR_YUV2BGRA_YUY2,
    COLOR_YUV2RGBA_YUNV = COLOR_YUV2RGBA_YUY2,
    COLOR_YUV2BGRA_YUNV = COLOR_YUV2BGRA_YUY2,

    COLOR_YUV2GRAY_UYVY = 123,
    COLOR_YUV2GRAY_YUY2 = 124,
    //COLOR_YUV2GRAY_VYUY = COLOR_YUV2GRAY_UYVY,
    COLOR_YUV2GRAY_Y422 = COLOR_YUV2GRAY_UYVY,
    COLOR_YUV2GRAY_UYNV = COLOR_YUV2GRAY_UYVY,
    COLOR_YUV2GRAY_YVYU = COLOR_YUV2GRAY_YUY2,
    COLOR_YUV2GRAY_YUYV = COLOR_YUV2GRAY_YUY2,
    COLOR_YUV2GRAY_YUNV = COLOR_YUV2GRAY_YUY2,

    // alpha premultiplication
    COLOR_RGBA2mRGBA = 125,
    COLOR_mRGBA2RGBA = 126,

    COLOR_RGB2YUV_I420 = 127,
    COLOR_BGR2YUV_I420 = 128,
    COLOR_RGB2YUV_IYUV = COLOR_RGB2YUV_I420,
    COLOR_BGR2YUV_IYUV = COLOR_BGR2YUV_I420,

    COLOR_RGBA2YUV_I420 = 129,
    COLOR_BGRA2YUV_I420 = 130,
    COLOR_RGBA2YUV_IYUV = COLOR_RGBA2YUV_I420,
    COLOR_BGRA2YUV_IYUV = COLOR_BGRA2YUV_I420,
    COLOR_RGB2YUV_YV12  = 131,
    COLOR_BGR2YUV_YV12  = 132,
    COLOR_RGBA2YUV_YV12 = 133,
    COLOR_BGRA2YUV_YV12 = 134,

    COLOR_COLORCVT_MAX  = 135
};


//! converts image from one color space to another
CV_EXPORTS_W void cvtColor( InputArray src, OutputArray dst, int code, int dstCn=0 );


//! type of the template matching operation
enum { TM_SQDIFF=0, TM_SQDIFF_NORMED=1, TM_CCORR=2, TM_CCORR_NORMED=3, TM_CCOEFF=4, TM_CCOEFF_NORMED=5 };



//! mode of the contour retrieval algorithm
enum
{
    RETR_EXTERNAL=CV_RETR_EXTERNAL, //!< retrieve only the most external (top-level) contours
    RETR_LIST=CV_RETR_LIST, //!< retrieve all the contours without any hierarchical information
    RETR_CCOMP=CV_RETR_CCOMP, //!< retrieve the connected components (that can possibly be nested)
    RETR_TREE=CV_RETR_TREE, //!< retrieve all the contours and the whole hierarchy
    RETR_FLOODFILL=CV_RETR_FLOODFILL
};

//! the contour approximation algorithm
enum
{
    CHAIN_APPROX_NONE=CV_CHAIN_APPROX_NONE,
    CHAIN_APPROX_SIMPLE=CV_CHAIN_APPROX_SIMPLE,
    CHAIN_APPROX_TC89_L1=CV_CHAIN_APPROX_TC89_L1,
    CHAIN_APPROX_TC89_KCOS=CV_CHAIN_APPROX_TC89_KCOS
};

//! retrieves contours and the hierarchical information from black-n-white image.
CV_EXPORTS_W void findContours( InputOutputArray image, OutputArrayOfArrays contours,
                              OutputArray hierarchy, int mode,
                              int method, Point offset=Point());

//! retrieves contours from black-n-white image.
CV_EXPORTS void findContours( InputOutputArray image, OutputArrayOfArrays contours,
                              int mode, int method, Point offset=Point());



//! approximates contour or a curve using Douglas-Peucker algorithm
CV_EXPORTS_W void approxPolyDP( InputArray curve,
                                OutputArray approxCurve,
                                double epsilon, bool closed );

//! computes the contour perimeter (closed=true) or a curve length
CV_EXPORTS_W double arcLength( InputArray curve, bool closed );
//! computes the bounding rectangle for a contour
CV_EXPORTS_W Rect boundingRect( InputArray points );
//! computes the contour area
CV_EXPORTS_W double contourArea( InputArray contour, bool oriented=false );
//! computes the minimal rotated rectangle for a set of points
CV_EXPORTS_W RotatedRect minAreaRect( InputArray points );
//! computes the minimal enclosing circle for a set of points
CV_EXPORTS_W void minEnclosingCircle( InputArray points,
                                      CV_OUT Point2f& center, CV_OUT float& radius );
//! matches two contours using one of the available algorithms
CV_EXPORTS_W double matchShapes( InputArray contour1, InputArray contour2,
                                 int method, double parameter );
//! computes convex hull for a set of 2D points.
CV_EXPORTS_W void convexHull( InputArray points, OutputArray hull,
                              bool clockwise=false, bool returnPoints=true );
//! computes the contour convexity defects
CV_EXPORTS_W void convexityDefects( InputArray contour, InputArray convexhull, OutputArray convexityDefects );

//! returns true if the contour is convex. Does not support contours with self-intersection
CV_EXPORTS_W bool isContourConvex( InputArray contour );



//! fits ellipse to the set of 2D points
CV_EXPORTS_W RotatedRect fitEllipse( InputArray points );

//! fits line to the set of 2D points using M-estimator algorithm
CV_EXPORTS_W void fitLine( InputArray points, OutputArray line, int distType,
                           double param, double reps, double aeps );
//! checks if the point is inside the contour. Optionally computes the signed distance from the point to the contour boundary
CV_EXPORTS_W double pointPolygonTest( InputArray contour, Point2f pt, bool measureDist );


class CV_EXPORTS_W Subdiv2D
{
public:
    enum
    {
        PTLOC_ERROR = -2,
        PTLOC_OUTSIDE_RECT = -1,
        PTLOC_INSIDE = 0,
        PTLOC_VERTEX = 1,
        PTLOC_ON_EDGE = 2
    };

    enum
    {
        NEXT_AROUND_ORG   = 0x00,
        NEXT_AROUND_DST   = 0x22,
        PREV_AROUND_ORG   = 0x11,
        PREV_AROUND_DST   = 0x33,
        NEXT_AROUND_LEFT  = 0x13,
        NEXT_AROUND_RIGHT = 0x31,
        PREV_AROUND_LEFT  = 0x20,
        PREV_AROUND_RIGHT = 0x02
    };

    CV_WRAP Subdiv2D();
    CV_WRAP Subdiv2D(Rect rect);
    CV_WRAP void initDelaunay(Rect rect);

    CV_WRAP int insert(Point2f pt);
    CV_WRAP void insert(const vector<Point2f>& ptvec);
    CV_WRAP int locate(Point2f pt, CV_OUT int& edge, CV_OUT int& vertex);

    CV_WRAP int findNearest(Point2f pt, CV_OUT Point2f* nearestPt=0);
    CV_WRAP void getEdgeList(CV_OUT vector<Vec4f>& edgeList) const;
    CV_WRAP void getTriangleList(CV_OUT vector<Vec6f>& triangleList) const;
    CV_WRAP void getVoronoiFacetList(const vector<int>& idx, CV_OUT vector<vector<Point2f> >& facetList,
                                     CV_OUT vector<Point2f>& facetCenters);

    CV_WRAP Point2f getVertex(int vertex, CV_OUT int* firstEdge=0) const;

    CV_WRAP int getEdge( int edge, int nextEdgeType ) const;
    CV_WRAP int nextEdge(int edge) const;
    CV_WRAP int rotateEdge(int edge, int rotate) const;
    CV_WRAP int symEdge(int edge) const;
    CV_WRAP int edgeOrg(int edge, CV_OUT Point2f* orgpt=0) const;
    CV_WRAP int edgeDst(int edge, CV_OUT Point2f* dstpt=0) const;

protected:
    int newEdge();
    void deleteEdge(int edge);
    int newPoint(Point2f pt, bool isvirtual, int firstEdge=0);
    void deletePoint(int vtx);
    void setEdgePoints( int edge, int orgPt, int dstPt );
    void splice( int edgeA, int edgeB );
    int connectEdges( int edgeA, int edgeB );
    void swapEdges( int edge );
    int isRightOf(Point2f pt, int edge) const;
    void calcVoronoi();
    void clearVoronoi();
    void checkSubdiv() const;

    struct CV_EXPORTS Vertex
    {
        Vertex();
        Vertex(Point2f pt, bool _isvirtual, int _firstEdge=0);
        bool isvirtual() const;
        bool isfree() const;
        int firstEdge;
        int type;
        Point2f pt;
    };
    struct CV_EXPORTS QuadEdge
    {
        QuadEdge();
        QuadEdge(int edgeidx);
        bool isfree() const;
        int next[4];
        int pt[4];
    };

    vector<Vertex> vtx;
    vector<QuadEdge> qedges;
    int freeQEdge;
    int freePoint;
    bool validGeometry;

    int recentEdge;
    Point2f topLeft;
    Point2f bottomRight;
};

}

#endif /* __cplusplus */

#endif

/* End of file. */

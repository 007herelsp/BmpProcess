#ifndef __OPENCV_IMGPROC_HPP__
#define __OPENCV_IMGPROC_HPP__

#include "core.hpp"
#include "types_c.h"

#ifdef __cplusplus

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
    FilterEngine(const Ptr<BaseRowFilter>& _rowFilter,
                 const Ptr<BaseColumnFilter>& _columnFilter,
                 int srcType, int dstType, int bufType,
                 int _rowBorderType=BORDER_REPLICATE,
                 int _columnBorderType=-1,
                 const Scalar& _borderValue=Scalar());
    //! the destructor
    virtual ~FilterEngine();
    //! reinitializes the engine. The previously assigned filters are released.
    void init(const Ptr<BaseRowFilter>& _rowFilter,
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
    bool isSeparable() const { return true; }
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



//! returns the separable linear filter engine
CV_EXPORTS Ptr<FilterEngine> createSeparableLinearFilter(int srcType, int dstType,
                          InputArray rowKernel, InputArray columnKernel,
                          Point anchor=Point(-1,-1), double delta=0,
                          int rowBorderType=BORDER_DEFAULT,
                          int columnBorderType=-1,
                          const Scalar& borderValue=Scalar());



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



//! smooths the image using Gaussian filter.
CV_EXPORTS_W void GaussianBlur( InputArray src,
                                               OutputArray dst, Size ksize,
                                               double sigmaX, double sigmaY=0,
                                               int borderType=BORDER_DEFAULT );



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

//! type of the threshold operation
enum { THRESH_BINARY=CV_THRESH_BINARY, THRESH_BINARY_INV=CV_THRESH_BINARY_INV,
       THRESH_TRUNC=CV_THRESH_TRUNC, THRESH_TOZERO=CV_THRESH_TOZERO,
       THRESH_TOZERO_INV=CV_THRESH_TOZERO_INV, THRESH_MASK=CV_THRESH_MASK,
       THRESH_OTSU=CV_THRESH_OTSU };

//! applies fixed threshold to the image
CV_EXPORTS_W double threshold( InputArray src, OutputArray dst,
                               double thresh, double maxval, int type );


//! converts image from one color space to another
CV_EXPORTS_W void cvtColor( InputArray src, OutputArray dst, int code, int dstCn=0 );


}

#endif /* __cplusplus */

#endif

/* End of file. */

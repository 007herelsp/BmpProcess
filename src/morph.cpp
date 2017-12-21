 

#include "imgproc.precomp.hpp"
#include <limits.h>
#include <stdio.h>

/****************************************************************************************\
                     Basic Morphological Operations: Erosion & Dilation
\****************************************************************************************/

namespace cv
{

template<typename T> struct MinOp
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator ()(const T a, const T b) const { return std::min(a, b); }
};

template<typename T> struct MaxOp
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator ()(const T a, const T b) const { return std::max(a, b); }
};

#undef CV_MIN_8U
#undef CV_MAX_8U
#define CV_MIN_8U(a,b)       ((a) - CV_FAST_CAST_8U((a) - (b)))
#define CV_MAX_8U(a,b)       ((a) + CV_FAST_CAST_8U((b) - (a)))


struct MorphRowNoVec
{
    MorphRowNoVec(int, int) {}
    int operator()(const uchar*, uchar*, int, int) const { return 0; }
};

struct MorphColumnNoVec
{
    MorphColumnNoVec(int, int) {}
    int operator()(const uchar**, uchar*, int, int, int) const { return 0; }
};

struct MorphNoVec
{
    int operator()(uchar**, int, uchar*, int) const { return 0; }
};


typedef MorphRowNoVec ErodeRowVec8u;
typedef MorphRowNoVec DilateRowVec8u;

typedef MorphColumnNoVec ErodeColumnVec8u;
typedef MorphColumnNoVec DilateColumnVec8u;

typedef MorphRowNoVec ErodeRowVec16u;
typedef MorphRowNoVec DilateRowVec16u;
typedef MorphRowNoVec ErodeRowVec16s;
typedef MorphRowNoVec DilateRowVec16s;
typedef MorphRowNoVec ErodeRowVec32f;
typedef MorphRowNoVec DilateRowVec32f;

typedef MorphColumnNoVec ErodeColumnVec16u;
typedef MorphColumnNoVec DilateColumnVec16u;
typedef MorphColumnNoVec ErodeColumnVec16s;
typedef MorphColumnNoVec DilateColumnVec16s;
typedef MorphColumnNoVec ErodeColumnVec32f;
typedef MorphColumnNoVec DilateColumnVec32f;

typedef MorphNoVec ErodeVec8u;
typedef MorphNoVec DilateVec8u;
typedef MorphNoVec ErodeVec16u;
typedef MorphNoVec DilateVec16u;
typedef MorphNoVec ErodeVec16s;
typedef MorphNoVec DilateVec16s;
typedef MorphNoVec ErodeVec32f;
typedef MorphNoVec DilateVec32f;


typedef MorphRowNoVec ErodeRowVec64f;
typedef MorphRowNoVec DilateRowVec64f;
typedef MorphColumnNoVec ErodeColumnVec64f;
typedef MorphColumnNoVec DilateColumnVec64f;
typedef MorphNoVec ErodeVec64f;
typedef MorphNoVec DilateVec64f;


template<class Op, class VecOp> struct MorphRowFilter : public BaseRowFilter
{
    typedef typename Op::rtype T;

    MorphRowFilter( int _ksize, int _anchor ) : vecOp(_ksize, _anchor)
    {
        ksize = _ksize;
        anchor = _anchor;
    }

    void operator()(const uchar* src, uchar* dst, int width, int cn)
    {
        int i, j, k, _ksize = ksize*cn;
        const T* S = (const T*)src;
        Op op;
        T* D = (T*)dst;

        if( _ksize == cn )
        {
            for( i = 0; i < width*cn; i++ )
                D[i] = S[i];
            return;
        }

        int i0 = vecOp(src, dst, width, cn);
        width *= cn;

        for( k = 0; k < cn; k++, S++, D++ )
        {
            for( i = i0; i <= width - cn*2; i += cn*2 )
            {
                const T* s = S + i;
                T m = s[cn];
                for( j = cn*2; j < _ksize; j += cn )
                    m = op(m, s[j]);
                D[i] = op(m, s[0]);
                D[i+cn] = op(m, s[j]);
            }

            for( ; i < width; i += cn )
            {
                const T* s = S + i;
                T m = s[0];
                for( j = cn; j < _ksize; j += cn )
                    m = op(m, s[j]);
                D[i] = m;
            }
        }
    }

    VecOp vecOp;
};


template<class Op, class VecOp> struct MorphColumnFilter : public BaseColumnFilter
{
    typedef typename Op::rtype T;

    MorphColumnFilter( int _ksize, int _anchor ) : vecOp(_ksize, _anchor)
    {
        ksize = _ksize;
        anchor = _anchor;
    }

    void operator()(const uchar** _src, uchar* dst, int dststep, int count, int width)
    {
        int i, k, _ksize = ksize;
        const T** src = (const T**)_src;
        T* D = (T*)dst;
        Op op;

        int i0 = vecOp(_src, dst, dststep, count, width);
        dststep /= sizeof(D[0]);

        for( ; _ksize > 1 && count > 1; count -= 2, D += dststep*2, src += 2 )
        {
            i = i0;
            for( ; i < width; i++ )
            {
                T s0 = src[1][i];

                for( k = 2; k < _ksize; k++ )
                    s0 = op(s0, src[k][i]);

                D[i] = op(s0, src[0][i]);
                D[i+dststep] = op(s0, src[k][i]);
            }
        }

        for( ; count > 0; count--, D += dststep, src++ )
        {
            i = i0;
            for( ; i < width; i++ )
            {
                T s0 = src[0][i];
                for( k = 1; k < _ksize; k++ )
                    s0 = op(s0, src[k][i]);
                D[i] = s0;
            }
        }
    }

    VecOp vecOp;
};


template<class Op, class VecOp> struct MorphFilter : BaseFilter
{
    typedef typename Op::rtype T;

    MorphFilter( const Mat& _kernel, Point _anchor )
    {
        anchor = _anchor;
        ksize = _kernel.size();
        CV_Assert( _kernel.type() == CV_8U );

        vector<uchar> coeffs; // we do not really the values of non-zero
        // kernel elements, just their locations
        preprocess2DKernel( _kernel, coords, coeffs );
        ptrs.resize( coords.size() );
    }

    void operator()(const uchar** src, uchar* dst, int dststep, int count, int width, int cn)
    {
        const Point* pt = &coords[0];
        const T** kp = (const T**)&ptrs[0];
        int i, k, nz = (int)coords.size();
        Op op;

        width *= cn;
        for( ; count > 0; count--, dst += dststep, src++ )
        {
            T* D = (T*)dst;

            for( k = 0; k < nz; k++ )
                kp[k] = (const T*)src[pt[k].y] + pt[k].x*cn;

            i = vecOp(&ptrs[0], nz, dst, width);
            for( ; i < width; i++ )
            {
                T s0 = kp[0][i];
                for( k = 1; k < nz; k++ )
                    s0 = op(s0, kp[k][i]);
                D[i] = s0;
            }
        }
    }

    vector<Point> coords;
    vector<uchar*> ptrs;
    VecOp vecOp;
};

}

/////////////////////////////////// External Interface /////////////////////////////////////

cv::Ptr<cv::BaseRowFilter> cv::getMorphologyRowFilter(int op, int type, int ksize, int anchor)
{
    int depth = CV_MAT_DEPTH(type);
    if( anchor < 0 )
        anchor = ksize/2;
    CV_Assert( op == MORPH_ERODE || op == MORPH_DILATE );
    if( op == MORPH_ERODE )
    {
        if( depth == CV_8U )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MinOp<uchar>,
                                      ErodeRowVec8u>(ksize, anchor));
        if( depth == CV_16U )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MinOp<ushort>,
                                      ErodeRowVec16u>(ksize, anchor));
        if( depth == CV_16S )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MinOp<short>,
                                      ErodeRowVec16s>(ksize, anchor));
        if( depth == CV_32F )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MinOp<float>,
                                      ErodeRowVec32f>(ksize, anchor));
        if( depth == CV_64F )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MinOp<double>,
                                      ErodeRowVec64f>(ksize, anchor));
    }
    else
    {
        if( depth == CV_8U )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MaxOp<uchar>,
                                      DilateRowVec8u>(ksize, anchor));
        if( depth == CV_16U )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MaxOp<ushort>,
                                      DilateRowVec16u>(ksize, anchor));
        if( depth == CV_16S )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MaxOp<short>,
                                      DilateRowVec16s>(ksize, anchor));
        if( depth == CV_32F )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MaxOp<float>,
                                      DilateRowVec32f>(ksize, anchor));
        if( depth == CV_64F )
            return Ptr<BaseRowFilter>(new MorphRowFilter<MaxOp<double>,
                                      DilateRowVec64f>(ksize, anchor));
    }

    CV_Error_( CV_StsNotImplemented, ("Unsupported data type (=%d)", type));
    return Ptr<BaseRowFilter>(0);
}

cv::Ptr<cv::BaseColumnFilter> cv::getMorphologyColumnFilter(int op, int type, int ksize, int anchor)
{
    int depth = CV_MAT_DEPTH(type);
    if( anchor < 0 )
        anchor = ksize/2;
    CV_Assert( op == MORPH_ERODE || op == MORPH_DILATE );
    if( op == MORPH_ERODE )
    {
        if( depth == CV_8U )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MinOp<uchar>,
                                         ErodeColumnVec8u>(ksize, anchor));
        if( depth == CV_16U )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MinOp<ushort>,
                                         ErodeColumnVec16u>(ksize, anchor));
        if( depth == CV_16S )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MinOp<short>,
                                         ErodeColumnVec16s>(ksize, anchor));
        if( depth == CV_32F )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MinOp<float>,
                                         ErodeColumnVec32f>(ksize, anchor));
        if( depth == CV_64F )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MinOp<double>,
                                         ErodeColumnVec64f>(ksize, anchor));
    }
    else
    {
        if( depth == CV_8U )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MaxOp<uchar>,
                                         DilateColumnVec8u>(ksize, anchor));
        if( depth == CV_16U )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MaxOp<ushort>,
                                         DilateColumnVec16u>(ksize, anchor));
        if( depth == CV_16S )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MaxOp<short>,
                                         DilateColumnVec16s>(ksize, anchor));
        if( depth == CV_32F )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MaxOp<float>,
                                         DilateColumnVec32f>(ksize, anchor));
        if( depth == CV_64F )
            return Ptr<BaseColumnFilter>(new MorphColumnFilter<MaxOp<double>,
                                         DilateColumnVec64f>(ksize, anchor));
    }

    CV_Error_( CV_StsNotImplemented, ("Unsupported data type (=%d)", type));
    return Ptr<BaseColumnFilter>(0);
}


cv::Ptr<cv::BaseFilter> cv::getMorphologyFilter(int op, int type, InputArray _kernel, Point anchor)
{
    Mat kernel = _kernel.getMat();
    int depth = CV_MAT_DEPTH(type);
    anchor = normalizeAnchor(anchor, kernel.size());
    CV_Assert( op == MORPH_ERODE || op == MORPH_DILATE );
    if( op == MORPH_ERODE )
    {
        if( depth == CV_8U )
            return Ptr<BaseFilter>(new MorphFilter<MinOp<uchar>, ErodeVec8u>(kernel, anchor));
        if( depth == CV_16U )
            return Ptr<BaseFilter>(new MorphFilter<MinOp<ushort>, ErodeVec16u>(kernel, anchor));
        if( depth == CV_16S )
            return Ptr<BaseFilter>(new MorphFilter<MinOp<short>, ErodeVec16s>(kernel, anchor));
        if( depth == CV_32F )
            return Ptr<BaseFilter>(new MorphFilter<MinOp<float>, ErodeVec32f>(kernel, anchor));
        if( depth == CV_64F )
            return Ptr<BaseFilter>(new MorphFilter<MinOp<double>, ErodeVec64f>(kernel, anchor));
    }
    else
    {
        if( depth == CV_8U )
            return Ptr<BaseFilter>(new MorphFilter<MaxOp<uchar>, DilateVec8u>(kernel, anchor));
        if( depth == CV_16U )
            return Ptr<BaseFilter>(new MorphFilter<MaxOp<ushort>, DilateVec16u>(kernel, anchor));
        if( depth == CV_16S )
            return Ptr<BaseFilter>(new MorphFilter<MaxOp<short>, DilateVec16s>(kernel, anchor));
        if( depth == CV_32F )
            return Ptr<BaseFilter>(new MorphFilter<MaxOp<float>, DilateVec32f>(kernel, anchor));
        if( depth == CV_64F )
            return Ptr<BaseFilter>(new MorphFilter<MaxOp<double>, DilateVec64f>(kernel, anchor));
    }

    CV_Error_( CV_StsNotImplemented, ("Unsupported data type (=%d)", type));
    return Ptr<BaseFilter>(0);
}


cv::Ptr<cv::FilterEngine> cv::createMorphologyFilter( int op, int type, InputArray _kernel,
                                                      Point anchor, int _rowBorderType, int _columnBorderType,
                                                      const Scalar& _borderValue )
{
    Mat kernel = _kernel.getMat();
    anchor = normalizeAnchor(anchor, kernel.size());

    Ptr<BaseRowFilter> rowFilter;
    Ptr<BaseColumnFilter> columnFilter;
    Ptr<BaseFilter> filter2D;

    if( countNonZero(kernel) == kernel.rows*kernel.cols )
    {
        // rectangular structuring element
        rowFilter = getMorphologyRowFilter(op, type, kernel.cols, anchor.x);
        columnFilter = getMorphologyColumnFilter(op, type, kernel.rows, anchor.y);
    }
    else
        filter2D = getMorphologyFilter(op, type, kernel, anchor);

    Scalar borderValue = _borderValue;
    if( (_rowBorderType == BORDER_CONSTANT || _columnBorderType == BORDER_CONSTANT) &&
            borderValue == morphologyDefaultBorderValue() )
    {
        int depth = CV_MAT_DEPTH(type);
        CV_Assert( depth == CV_8U || depth == CV_16U || depth == CV_16S ||
                   depth == CV_32F || depth == CV_64F );
        if( op == MORPH_ERODE )
            borderValue = Scalar::all( depth == CV_8U ? (double)UCHAR_MAX :
                                       depth == CV_16U ? (double)USHRT_MAX :
                                       depth == CV_16S ? (double)SHRT_MAX :
                                       depth == CV_32F ? (double)FLT_MAX : DBL_MAX);
        else
            borderValue = Scalar::all( depth == CV_8U || depth == CV_16U ?
                                           0. :
                                       depth == CV_16S ? (double)SHRT_MIN :
                                       depth == CV_32F ? (double)-FLT_MAX : -DBL_MAX);
    }

    return Ptr<FilterEngine>(new FilterEngine(filter2D, rowFilter, columnFilter,
                                              type, type, type, _rowBorderType, _columnBorderType, borderValue ));
}


cv::Mat cv::getStructuringElement(int shape, Size ksize, Point anchor)
{
    int i, j;
    int r = 0, c = 0;
    double inv_r2 = 0;

    CV_Assert( shape == MORPH_RECT || shape == MORPH_CROSS || shape == MORPH_ELLIPSE );

    anchor = normalizeAnchor(anchor, ksize);

    if( ksize == Size(1,1) )
        shape = MORPH_RECT;

    if( shape == MORPH_ELLIPSE )
    {
        r = ksize.height/2;
        c = ksize.width/2;
        inv_r2 = r ? 1./((double)r*r) : 0;
    }

    Mat elem(ksize, CV_8U);

    for( i = 0; i < ksize.height; i++ )
    {
        uchar* ptr = elem.data + i*elem.step;
        int j1 = 0, j2 = 0;

        if( shape == MORPH_RECT || (shape == MORPH_CROSS && i == anchor.y) )
            j2 = ksize.width;
        else if( shape == MORPH_CROSS )
            j1 = anchor.x, j2 = j1 + 1;
        else
        {
            int dy = i - r;
            if( std::abs(dy) <= r )
            {
                int dx = saturate_cast<int>(c*std::sqrt((r*r - dy*dy)*inv_r2));
                j1 = std::max( c - dx, 0 );
                j2 = std::min( c + dx + 1, ksize.width );
            }
        }

        for( j = 0; j < j1; j++ )
            ptr[j] = 0;
        for( ; j < j2; j++ )
            ptr[j] = 1;
        for( ; j < ksize.width; j++ )
            ptr[j] = 0;
    }

    return elem;
}

namespace cv
{

class MorphologyRunner : public ParallelLoopBody
{
public:
    MorphologyRunner(Mat _src, Mat _dst, int _nStripes, int _iterations,
                     int _op, Mat _kernel, Point _anchor,
                     int _rowBorderType, int _columnBorderType, const Scalar& _borderValue) :
        borderValue(_borderValue)
    {
        src = _src;
        dst = _dst;

        nStripes = _nStripes;
        iterations = _iterations;

        op = _op;
        kernel = _kernel;
        anchor = _anchor;
        rowBorderType = _rowBorderType;
        columnBorderType = _columnBorderType;
    }

    void operator () ( const Range& range ) const
    {
        int row0 = min(cvRound(range.start * src.rows / nStripes), src.rows);
        int row1 = min(cvRound(range.end * src.rows / nStripes), src.rows);

        /*if(0)
            printf("Size = (%d, %d), range[%d,%d), row0 = %d, row1 = %d\n",
                   src.rows, src.cols, range.start, range.end, row0, row1);*/

        Mat srcStripe = src.rowRange(row0, row1);
        Mat dstStripe = dst.rowRange(row0, row1);




        Ptr<FilterEngine> f = createMorphologyFilter(op, src.type(), kernel, anchor,
                                                     rowBorderType, columnBorderType, borderValue );

        f->apply( srcStripe, dstStripe );
        for( int i = 1; i < iterations; i++ )
            f->apply( dstStripe, dstStripe );
    }

private:
    Mat src;
    Mat dst;
    int nStripes;
    int iterations;

    int op;
    Mat kernel;
    Point anchor;
    int rowBorderType;
    int columnBorderType;
    Scalar borderValue;
};


static void morphOp( int op, InputArray _src, OutputArray _dst,
                     InputArray _kernel,
                     Point anchor, int iterations,
                     int borderType, const Scalar& borderValue )
{
    Mat kernel = _kernel.getMat();
    Size ksize = kernel.data ? kernel.size() : Size(3,3);
    anchor = normalizeAnchor(anchor, ksize);

    CV_Assert( anchor.inside(Rect(0, 0, ksize.width, ksize.height)) );


    Mat src = _src.getMat();

    _dst.create( src.size(), src.type() );
    Mat dst = _dst.getMat();

    if( iterations == 0 || kernel.rows*kernel.cols == 1 )
    {
        src.copyTo(dst);
        return;
    }

    if( !kernel.data )
    {
        kernel = getStructuringElement(MORPH_RECT, Size(1+iterations*2,1+iterations*2));
        anchor = Point(iterations, iterations);
        iterations = 1;
    }
    else if( iterations > 1 && countNonZero(kernel) == kernel.rows*kernel.cols )
    {
        anchor = Point(anchor.x*iterations, anchor.y*iterations);
        kernel = getStructuringElement(MORPH_RECT,
                                       Size(ksize.width + (iterations-1)*(ksize.width-1),
                                            ksize.height + (iterations-1)*(ksize.height-1)),
                                       anchor);
        iterations = 1;
    }

    int nStripes = 1;


    parallel_for_(Range(0, nStripes),
                  MorphologyRunner(src, dst, nStripes, iterations, op, kernel, anchor, borderType, borderType, borderValue));

    //Ptr<FilterEngine> f = createMorphologyFilter(op, src.type(),
    //                                             kernel, anchor, borderType, borderType, borderValue );

    //f->apply( src, dst );
    //for( int i = 1; i < iterations; i++ )
    //    f->apply( dst, dst );
}

template<> void Ptr<IplConvKernel>::delete_obj()
{ cvReleaseStructuringElement(&obj); }

}

void cv::erode( InputArray src, OutputArray dst, InputArray kernel,
                Point anchor, int iterations,
                int borderType, const Scalar& borderValue )
{
    morphOp( MORPH_ERODE, src, dst, kernel, anchor, iterations, borderType, borderValue );
}


void cv::dilate( InputArray src, OutputArray dst, InputArray kernel,
                 Point anchor, int iterations,
                 int borderType, const Scalar& borderValue )
{
    morphOp( MORPH_DILATE, src, dst, kernel, anchor, iterations, borderType, borderValue );
}


CV_IMPL IplConvKernel *
cvCreateStructuringElementEx( int cols, int rows,
                              int anchorX, int anchorY,
                              int shape, int *values )
{
    cv::Size ksize = cv::Size(cols, rows);
    cv::Point anchor = cv::Point(anchorX, anchorY);
    CV_Assert( cols > 0 && rows > 0 && anchor.inside(cv::Rect(0,0,cols,rows)) &&
               (shape != CV_SHAPE_CUSTOM || values != 0));

    int i, size = rows * cols;
    int element_size = sizeof(IplConvKernel) + size*sizeof(int);
    IplConvKernel *element = (IplConvKernel*)cvAlloc(element_size + 32);

    element->nCols = cols;
    element->nRows = rows;
    element->anchorX = anchorX;
    element->anchorY = anchorY;
    element->nShiftR = shape < CV_SHAPE_ELLIPSE ? shape : CV_SHAPE_CUSTOM;
    element->values = (int*)(element + 1);

    if( shape == CV_SHAPE_CUSTOM )
    {
        for( i = 0; i < size; i++ )
            element->values[i] = values[i];
    }
    else
    {
        cv::Mat elem = cv::getStructuringElement(shape, ksize, anchor);
        for( i = 0; i < size; i++ )
            element->values[i] = elem.data[i];
    }

    return element;
}


CV_IMPL void
cvReleaseStructuringElement( IplConvKernel ** element )
{
    if( !element )
        CV_Error( CV_StsNullPtr, "" );
    cvFree( element );
}


static void convertConvKernel( const IplConvKernel* src, cv::Mat& dst, cv::Point& anchor )
{
    if(!src)
    {
        anchor = cv::Point(1,1);
        dst.release();
        return;
    }
    anchor = cv::Point(src->anchorX, src->anchorY);
    dst.create(src->nRows, src->nCols, CV_8U);

    int i, size = src->nRows*src->nCols;
    for( i = 0; i < size; i++ )
        dst.data[i] = (uchar)(src->values[i] != 0);
}


CV_IMPL void
cvErode( const CvArr* srcarr, CvArr* dstarr, IplConvKernel* element, int iterations )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr), kernel;
    CV_Assert( src.size() == dst.size() && src.type() == dst.type() );
    cv::Point anchor;
    convertConvKernel( element, kernel, anchor );
    cv::erode( src, dst, kernel, anchor, iterations, cv::BORDER_REPLICATE );
}


CV_IMPL void
cvDilate( const CvArr* srcarr, CvArr* dstarr, IplConvKernel* element, int iterations )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr), kernel;
    CV_Assert( src.size() == dst.size() && src.type() == dst.type() );
    cv::Point anchor;
    convertConvKernel( element, kernel, anchor );
    cv::dilate( src, dst, kernel, anchor, iterations, cv::BORDER_REPLICATE );
}


/* End of file. */

#ifndef __OPENCV_CORE_HPP__
#define __OPENCV_CORE_HPP__

#include "opencv2/core/types_c.h"


#ifdef __cplusplus

#ifndef SKIP_INCLUDES
#include <limits.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <complex>
#include <map>
#include <new>
#include <string>
#include <vector>
#include <sstream>
#endif // SKIP_INCLUDES

/*! \namespace cv
    Namespace where all the C++ OpenCV functionality resides
*/
namespace cv {

#undef abs
#undef min
#undef max
#undef Complex

using std::vector;
using std::string;
using std::ptrdiff_t;

template<typename _Tp> class Size_;
template<typename _Tp> class Point_;
template<typename _Tp> class Rect_;
template<typename _Tp, int cn> class Vec;
template<typename _Tp, int m, int n> class Matx;

typedef std::string String;

class Mat;


CV_EXPORTS string format( const char* fmt, ... );


// matrix decomposition types
enum { DECOMP_LU=0, DECOMP_SVD=1, DECOMP_EIG=2, DECOMP_CHOLESKY=3, DECOMP_QR=4, DECOMP_NORMAL=16 };
enum { NORM_INF=1, NORM_L1=2, NORM_L2=4, NORM_L2SQR=5, NORM_HAMMING=6, NORM_HAMMING2=7, NORM_TYPE_MASK=7, NORM_RELATIVE=8, NORM_MINMAX=32 };
enum { CMP_EQ=0, CMP_GT=1, CMP_GE=2, CMP_LT=3, CMP_LE=4, CMP_NE=5 };
enum { GEMM_1_T=1, GEMM_2_T=2, GEMM_3_T=4 };
enum { DFT_INVERSE=1, DFT_SCALE=2, DFT_ROWS=4, DFT_COMPLEX_OUTPUT=16, DFT_REAL_OUTPUT=32,
    DCT_INVERSE = DFT_INVERSE, DCT_ROWS=DFT_ROWS };


/*!
 The standard OpenCV exception class.
 Instances of the class are thrown by various functions and methods in the case of critical errors.
 */
class CV_EXPORTS Exception : public std::exception
{
public:
    /*!
     Default constructor
     */
    Exception();
    /*!
     Full constructor. Normally the constuctor is not called explicitly.
     Instead, the macros CV_Error(), CV_Error_() and CV_Assert() are used.
    */
    Exception(int _code, const string& _err, const string& _func, const string& _file, int _line);
    virtual ~Exception() throw();

    /*!
     \return the error description and the context as a text string.
    */
    virtual const char *what() const throw();
    void formatMessage();

    string msg; ///< the formatted error message

    int code; ///< error code @see CVStatus
    string err; ///< error description
    string func; ///< function name. Available only when the compiler supports getting it
    string file; ///< source file name where the error has occured
    int line; ///< line number in the source file where the error has occured
};


//! Signals an error and raises the exception.

/*!
  By default the function prints information about the error to stderr,
  then it either stops if setBreakOnError() had been called before or raises the exception.
  It is possible to alternate error processing by using redirectError().

  \param exc the exception raisen.
 */
CV_EXPORTS void error( const Exception& exc );



//! Sets the new error handler and the optional user data.




#if defined __GNUC__
#define CV_Func __func__
#elif defined _MSC_VER
#define CV_Func __FUNCTION__
#else
#define CV_Func ""
#endif

#define CV_Error( code, msg ) cv::error( cv::Exception(code, msg, CV_Func, __FILE__, __LINE__) )
#define CV_Error_( code, args ) cv::error( cv::Exception(code, cv::format args, CV_Func, __FILE__, __LINE__) )
#define CV_Assert( expr ) if(!!(expr)) ; else cv::error( cv::Exception(CV_StsAssert, #expr, CV_Func, __FILE__, __LINE__) )

#ifdef _DEBUG
#define CV_DbgAssert(expr) CV_Assert(expr)
#else
#define CV_DbgAssert(expr)
#endif



/*!
  Allocates memory buffer

  This is specialized OpenCV memory allocation function that returns properly aligned memory buffers.
  The usage is identical to malloc(). The allocated buffers must be freed with cv::fastFree().
  If there is not enough memory, the function calls cv::error(), which raises an exception.

  \param bufSize buffer size in bytes
  \return the allocated memory buffer.
*/
CV_EXPORTS void* fastMalloc(size_t bufSize);

/*!
  Frees the memory allocated with cv::fastMalloc

  This is the corresponding deallocation function for cv::fastMalloc().
  When ptr==NULL, the function has no effect.
*/
CV_EXPORTS void fastFree(void* ptr);

template<typename _Tp> static inline _Tp* allocate(size_t n)
{
    return new _Tp[n];
}

template<typename _Tp> static inline void deallocate(_Tp* ptr, size_t)
{
    delete[] ptr;
}

/*!
  Aligns pointer by the certain number of bytes

  This small inline function aligns the pointer by the certian number of bytes by shifting
  it forward by 0 or a positive offset.
*/
template<typename _Tp> static inline _Tp* alignPtr(_Tp* ptr, int n=(int)sizeof(_Tp))
{
    return (_Tp*)(((size_t)ptr + n-1) & -n);
}

/*!
  Aligns buffer size by the certain number of bytes

  This small inline function aligns a buffer size by the certian number of bytes by enlarging it.
*/
static inline size_t alignSize(size_t sz, int n)
{
    assert((n & (n - 1)) == 0); // n is a power of 2
    return (sz + n-1) & -n;
}


/////////////////////// Vec (used as element of multi-channel images /////////////////////

/*!
  A helper class for cv::DataType

  The class is specialized for each fundamental numerical data type supported by OpenCV.
  It provides DataDepth<T>::value constant.
*/
template<typename _Tp> class DataDepth { public: enum { value = -1, fmt = 0 }; };

template<> class DataDepth<bool> { public: enum { value = CV_8U, fmt=(int)'u' }; };
template<> class DataDepth<uchar> { public: enum { value = CV_8U, fmt=(int)'u' }; };
template<> class DataDepth<schar> { public: enum { value = CV_8S, fmt=(int)'c' }; };
template<> class DataDepth<char> { public: enum { value = CV_8S, fmt=(int)'c' }; };
template<> class DataDepth<ushort> { public: enum { value = CV_16U, fmt=(int)'w' }; };
template<> class DataDepth<short> { public: enum { value = CV_16S, fmt=(int)'s' }; };
template<> class DataDepth<int> { public: enum { value = CV_32S, fmt=(int)'i' }; };
// this is temporary solution to support 32-bit unsigned integers
template<> class DataDepth<unsigned> { public: enum { value = CV_32S, fmt=(int)'i' }; };
template<> class DataDepth<float> { public: enum { value = CV_32F, fmt=(int)'f' }; };
template<> class DataDepth<double> { public: enum { value = CV_64F, fmt=(int)'d' }; };
template<typename _Tp> class DataDepth<_Tp*> { public: enum { value = CV_USRTYPE1, fmt=(int)'r' }; };


////////////////////////////// Small Matrix ///////////////////////////

/*!
 A short numerical vector.

 This template class represents short numerical vectors (of 1, 2, 3, 4 ... elements)
 on which you can perform basic arithmetical operations, access individual elements using [] operator etc.
 The vectors are allocated on stack, as opposite to std::valarray, std::vector, cv::Mat etc.,
 which elements are dynamically allocated in the heap.

 The template takes 2 parameters:
 -# _Tp element type
 -# cn the number of elements

 In addition to the universal notation like Vec<float, 3>, you can use shorter aliases
 for the most popular specialized variants of Vec, e.g. Vec3f ~ Vec<float, 3>.
 */

struct CV_EXPORTS Matx_AddOp {};
struct CV_EXPORTS Matx_SubOp {};
struct CV_EXPORTS Matx_ScaleOp {};
struct CV_EXPORTS Matx_MulOp {};
struct CV_EXPORTS Matx_MatMulOp {};
struct CV_EXPORTS Matx_TOp {};

template<typename _Tp, int m, int n> class Matx
{
public:
    typedef _Tp value_type;
    typedef Matx<_Tp, (m < n ? m : n), 1> diag_type;
    typedef Matx<_Tp, m, n> mat_type;
    enum { depth = DataDepth<_Tp>::value, rows = m, cols = n, channels = rows*cols,
           type = CV_MAKETYPE(depth, channels) };

    //! default constructor
    Matx();

    Matx(_Tp v0); //!< 1x1 matrix
    Matx(_Tp v0, _Tp v1); //!< 1x2 or 2x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2); //!< 1x3 or 3x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3); //!< 1x4, 2x2 or 4x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4); //!< 1x5 or 5x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5); //!< 1x6, 2x3, 3x2 or 6x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6); //!< 1x7 or 7x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7); //!< 1x8, 2x4, 4x2 or 8x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7, _Tp v8); //!< 1x9, 3x3 or 9x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7, _Tp v8, _Tp v9); //!< 1x10, 2x5 or 5x2 or 10x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3,
         _Tp v4, _Tp v5, _Tp v6, _Tp v7,
         _Tp v8, _Tp v9, _Tp v10, _Tp v11); //!< 1x12, 2x6, 3x4, 4x3, 6x2 or 12x1 matrix
    Matx(_Tp v0, _Tp v1, _Tp v2, _Tp v3,
         _Tp v4, _Tp v5, _Tp v6, _Tp v7,
         _Tp v8, _Tp v9, _Tp v10, _Tp v11,
         _Tp v12, _Tp v13, _Tp v14, _Tp v15); //!< 1x16, 4x4 or 16x1 matrix
    explicit Matx(const _Tp* vals); //!< initialize from a plain array

    static Matx all(_Tp alpha);
    static Matx zeros();
    static Matx ones();
    static Matx eye();

    //! dot product computed with the default precision

    //! dot product computed in double-precision arithmetics

    //! conversion to another data type
    template<typename T2> operator Matx<T2, m, n>() const;

    //! change the matrix shape

    //! extract part of the matrix
    template<int m1, int n1> Matx<_Tp, m1, n1> get_minor(int i, int j) const;

    //! extract the matrix row
    Matx<_Tp, 1, n> row(int i) const;

    //! extract the matrix column
    Matx<_Tp, m, 1> col(int i) const;

    //! extract the matrix diagonal

    //! transpose the matrix
    Matx<_Tp, n, m> t() const;

    //! invert matrix the matrix
    Matx<_Tp, n, m> inv(int method=DECOMP_LU) const;

    //! solve linear system
    template<int l> Matx<_Tp, n, l> solve(const Matx<_Tp, m, l>& rhs, int flags=DECOMP_LU) const;
    Vec<_Tp, n> solve(const Vec<_Tp, m>& rhs, int method) const;

    //! element access
    const _Tp& operator ()(int i, int j) const;
    _Tp& operator ()(int i, int j);

    //! 1D element access
    const _Tp& operator ()(int i) const;
    _Tp& operator ()(int i);

    Matx(const Matx<_Tp, m, n>& a, const Matx<_Tp, m, n>& b, Matx_AddOp);
    Matx(const Matx<_Tp, m, n>& a, const Matx<_Tp, m, n>& b, Matx_SubOp);
    template<typename _T2> Matx(const Matx<_Tp, m, n>& a, _T2 alpha, Matx_ScaleOp);
    Matx(const Matx<_Tp, m, n>& a, const Matx<_Tp, m, n>& b, Matx_MulOp);
    template<int l> Matx(const Matx<_Tp, m, l>& a, const Matx<_Tp, l, n>& b, Matx_MatMulOp);
    Matx(const Matx<_Tp, n, m>& a, Matx_TOp);

    _Tp val[m*n]; //< matrix elements
};


typedef Matx<float, 1, 2> Matx12f;
typedef Matx<double, 1, 2> Matx12d;
typedef Matx<float, 1, 3> Matx13f;
typedef Matx<double, 1, 3> Matx13d;
typedef Matx<float, 1, 4> Matx14f;
typedef Matx<double, 1, 4> Matx14d;
typedef Matx<float, 1, 6> Matx16f;
typedef Matx<double, 1, 6> Matx16d;

typedef Matx<float, 2, 1> Matx21f;
typedef Matx<double, 2, 1> Matx21d;
typedef Matx<float, 3, 1> Matx31f;
typedef Matx<double, 3, 1> Matx31d;
typedef Matx<float, 4, 1> Matx41f;
typedef Matx<double, 4, 1> Matx41d;
typedef Matx<float, 6, 1> Matx61f;
typedef Matx<double, 6, 1> Matx61d;

typedef Matx<float, 2, 2> Matx22f;
typedef Matx<double, 2, 2> Matx22d;
typedef Matx<float, 2, 3> Matx23f;
typedef Matx<double, 2, 3> Matx23d;
typedef Matx<float, 3, 2> Matx32f;
typedef Matx<double, 3, 2> Matx32d;

typedef Matx<float, 3, 3> Matx33f;
typedef Matx<double, 3, 3> Matx33d;

typedef Matx<float, 3, 4> Matx34f;
typedef Matx<double, 3, 4> Matx34d;
typedef Matx<float, 4, 3> Matx43f;
typedef Matx<double, 4, 3> Matx43d;

typedef Matx<float, 4, 4> Matx44f;
typedef Matx<double, 4, 4> Matx44d;
typedef Matx<float, 6, 6> Matx66f;
typedef Matx<double, 6, 6> Matx66d;


/*!
  A short numerical vector.

  This template class represents short numerical vectors (of 1, 2, 3, 4 ... elements)
  on which you can perform basic arithmetical operations, access individual elements using [] operator etc.
  The vectors are allocated on stack, as opposite to std::valarray, std::vector, cv::Mat etc.,
  which elements are dynamically allocated in the heap.

  The template takes 2 parameters:
  -# _Tp element type
  -# cn the number of elements

  In addition to the universal notation like Vec<float, 3>, you can use shorter aliases
  for the most popular specialized variants of Vec, e.g. Vec3f ~ Vec<float, 3>.
*/
template<typename _Tp, int cn> class Vec : public Matx<_Tp, cn, 1>
{
public:
    typedef _Tp value_type;
    enum { depth = DataDepth<_Tp>::value, channels = cn, type = CV_MAKETYPE(depth, channels) };

    //! default constructor
    Vec();

    Vec(_Tp v0); //!< 1-element vector constructor
    Vec(_Tp v0, _Tp v1); //!< 2-element vector constructor
   
    explicit Vec(const _Tp* values);

    Vec(const Vec<_Tp, cn>& v);

   

    //! per-element multiplication

    //! conjugation (makes sense for complex numbers and quaternions)

    /*!
      cross product of the two 3D vectors.

      For other dimensionalities the exception is raised
    */
    //! conversion to another data type
    template<typename T2> operator Vec<T2, cn>() const;
    //! conversion to 4-element CvScalar.

    /*! element access */
    const _Tp& operator [](int i) const;
    _Tp& operator[](int i);
    const _Tp& operator ()(int i) const;
    _Tp& operator ()(int i);

 
};


/* \typedef

   Shorter aliases for the most popular specializations of Vec<T,n>
*/
typedef Vec<uchar, 2> Vec2b;
typedef Vec<uchar, 3> Vec3b;
typedef Vec<uchar, 4> Vec4b;

typedef Vec<short, 3> Vec3s;


typedef Vec<int, 2> Vec2i;
typedef Vec<int, 3> Vec3i;
typedef Vec<int, 4> Vec4i;
typedef Vec<int, 6> Vec6i;
typedef Vec<int, 8> Vec8i;





//////////////////////////////// Point_ ////////////////////////////////

/*!
  template 2D point class.

  The class defines a point in 2D space. Data type of the point coordinates is specified
  as a template parameter. There are a few shorter aliases available for user convenience.
  See cv::Point, cv::Point2i, cv::Point2f and cv::Point2d.
*/
template<typename _Tp> class Point_
{
public:
    typedef _Tp value_type;

    // various constructors
    Point_();
    Point_(_Tp _x, _Tp _y);
   

    Point_& operator = (const Point_& pt);
    bool inside(const Rect_<_Tp>& r) const;

    _Tp x, y; //< the point coordinates
};


//////////////////////////////// Size_ ////////////////////////////////

/*!
  The 2D size class

  The class represents the size of a 2D rectangle, image size, matrix size etc.
  Normally, cv::Size ~ cv::Size_<int> is used.
*/
template<typename _Tp> class Size_
{
public:
    typedef _Tp value_type;

    //! various constructors
    Size_();
    Size_(_Tp _width, _Tp _height);


    Size_& operator = (const Size_& sz);
    //! the area (width*height)
    _Tp area() const;

    //! conversion of another data type.


    //! conversion to the old-style OpenCV types
    operator CvSize() const;

    _Tp width, height; // the width and the height
};

//////////////////////////////// Rect_ ////////////////////////////////

/*!
  The 2D up-right rectangle class

  The class represents a 2D rectangle with coordinates of the specified data type.
  Normally, cv::Rect ~ cv::Rect_<int> is used.
*/
template<typename _Tp> class Rect_
{
public:
    typedef _Tp value_type;

    //! various constructors
    Rect_();
    Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height);
    Rect_(const Rect_& r);


    Rect_& operator = ( const Rect_& r );
    

    //! size (width, height) of the rectangle
    Size_<_Tp> size() const;
    //! area (width*height) of the rectangle
    _Tp area() const;

    //! conversion to another data type
    template<typename _Tp2> operator Rect_<_Tp2>() const;
    //! conversion to the old-style CvRect
    operator CvRect() const;

    //! checks whether the rectangle contains the point
    bool contains(const Point_<_Tp>& pt) const;

    _Tp x, y, width, height; //< the top-left corner, as well as width and height of the rectangle
};


typedef Point_<int> Point2i;
typedef Point_<int64> Point2l;
typedef Point2i Point;
typedef Size_<int> Size2i;
typedef Size_<int64> Size2l;
typedef Size_<double> Size2d;
typedef Size2i Size;
typedef Rect_<int> Rect;
typedef Point_<float> Point2f;
typedef Point_<double> Point2d;
typedef Size_<float> Size2f;


//////////////////////////////// Scalar_ ///////////////////////////////

/*!
   The template scalar class.

   This is partially specialized cv::Vec class with the number of elements = 4, i.e. a short vector of four elements.
   Normally, cv::Scalar ~ cv::Scalar_<double> is used.
*/
template<typename _Tp> class Scalar_ : public Vec<_Tp, 4>
{
public:
    //! various constructors
    Scalar_();
    Scalar_(_Tp v0, _Tp v1, _Tp v2=0, _Tp v3=0);
    Scalar_(const CvScalar& s);
    Scalar_(_Tp v0);

    //! returns a scalar with all elements set to v0
    static Scalar_<_Tp> all(_Tp v0);
    //! conversion to the old-style CvScalar

    //! conversion to another data type
    template<typename T2> operator Scalar_<T2>() const;

    //! per-element product

    // returns (v0, -v1, -v2, -v3)

    // returns true iff v1 == v2 == v3 == 0
    bool isReal() const;
};

typedef Scalar_<double> Scalar;

CV_EXPORTS void scalarToRawData(const Scalar& s, void* buf, int type, int unroll_to=0);

//////////////////////////////// Range /////////////////////////////////

/*!
   The 2D range class

   This is the class used to specify a continuous subsequence, i.e. part of a contour, or a column span in a matrix.
*/
class CV_EXPORTS Range
{
public:
    Range();
    Range(int _start, int _end);
    Range(const CvSlice& slice);
    int size() const;
    bool empty() const;
    static Range all();
    operator CvSlice() const;

    int start, end;
};

/////////////////////////////// DataType ////////////////////////////////


template<typename _Tp> class DataType
{
public:
    typedef _Tp value_type;
    typedef value_type work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 1, depth = -1, channels = 1, fmt=0,
        type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<bool>
{
public:
    typedef bool value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<uchar>
{
public:
    typedef uchar value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<schar>
{
public:
    typedef schar value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<char>
{
public:
    typedef schar value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<ushort>
{
public:
    typedef ushort value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<short>
{
public:
    typedef short value_type;
    typedef int work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<int>
{
public:
    typedef int value_type;
    typedef value_type work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<float>
{
public:
    typedef float value_type;
    typedef value_type work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<> class DataType<double>
{
public:
    typedef double value_type;
    typedef value_type work_type;
    typedef value_type channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 1,
           fmt=DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<typename _Tp, int m, int n> class DataType<Matx<_Tp, m, n> >
{
public:
    typedef Matx<_Tp, m, n> value_type;
    typedef Matx<typename DataType<_Tp>::work_type, m, n> work_type;
    typedef _Tp channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = m*n,
        fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
        type = CV_MAKETYPE(depth, channels) };
};

template<typename _Tp, int cn> class DataType<Vec<_Tp, cn> >
{
public:
    typedef Vec<_Tp, cn> value_type;
    typedef Vec<typename DataType<_Tp>::work_type, cn> work_type;
    typedef _Tp channel_type;
    typedef value_type vec_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = cn,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
};

template<typename _Tp> class DataType<std::complex<_Tp> >
{
public:
    typedef std::complex<_Tp> value_type;
    typedef value_type work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 2,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};


template<typename _Tp> class DataType<Point_<_Tp> >
{
public:
    typedef Point_<_Tp> value_type;
    typedef Point_<typename DataType<_Tp>::work_type> work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 2,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};

template<typename _Tp> class DataType<Size_<_Tp> >
{
public:
    typedef Size_<_Tp> value_type;
    typedef Size_<typename DataType<_Tp>::work_type> work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 2,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};

template<typename _Tp> class DataType<Rect_<_Tp> >
{
public:
    typedef Rect_<_Tp> value_type;
    typedef Rect_<typename DataType<_Tp>::work_type> work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 4,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};

template<typename _Tp> class DataType<Scalar_<_Tp> >
{
public:
    typedef Scalar_<_Tp> value_type;
    typedef Scalar_<typename DataType<_Tp>::work_type> work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 4,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};

template<> class DataType<Range>
{
public:
    typedef Range value_type;
    typedef value_type work_type;
    typedef int channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 2,
           fmt = ((channels-1)<<8) + DataDepth<channel_type>::fmt,
           type = CV_MAKETYPE(depth, channels) };
    typedef Vec<channel_type, channels> vec_type;
};

//////////////////// generic_type ref-counting pointer class for C/C++ objects ////////////////////////


template<typename _Tp> class Ptr
{
public:
    //! empty constructor
    Ptr();
    //! take ownership of the pointer. The associated reference counter is allocated and set to 1
    Ptr(_Tp* _obj);
    //! calls release()
    ~Ptr();
    //! copy constructor. Copies the members and calls addref()
    Ptr(const Ptr& ptr);
    template<typename _Tp2> Ptr(const Ptr<_Tp2>& ptr);
    //! copy operator. Calls ptr.addref() and release() before copying the members
    Ptr& operator = (const Ptr& ptr);
    //! increments the reference counter
    void addref();
    //! decrements the reference counter. If it reaches 0, delete_obj() is called
    void release();
    //! deletes the object. Override if needed
    void delete_obj();
    //! returns true iff obj==NULL
    bool empty() const;

    //! cast pointer to another type
    template<typename _Tp2> Ptr<_Tp2> ptr();
    template<typename _Tp2> const Ptr<_Tp2> ptr() const;

    //! helper operators making "Ptr<T> ptr" use very similar to "T* ptr".
    _Tp* operator -> ();
    const _Tp* operator -> () const;

    operator _Tp* ();
    operator const _Tp*() const;

    _Tp* obj; //< the object pointer.
    int* refcount; //< the associated reference counter
};

//////////////////////// Input/Output Array Arguments /////////////////////////////////

/*!
 Proxy datatype for passing Mat's and vector<>'s as input parameters
 */
class CV_EXPORTS _InputArray
{
public:
    enum {
        KIND_SHIFT = 16,
        FIXED_TYPE = 0x8000 << KIND_SHIFT,
        FIXED_SIZE = 0x4000 << KIND_SHIFT,
        KIND_MASK = ~(FIXED_TYPE|FIXED_SIZE) - (1 << KIND_SHIFT) + 1,

        NONE              = 0 << KIND_SHIFT,
        MAT               = 1 << KIND_SHIFT,
        MATX              = 2 << KIND_SHIFT,
        STD_VECTOR        = 3 << KIND_SHIFT,
        STD_VECTOR_VECTOR = 4 << KIND_SHIFT,
        STD_VECTOR_MAT    = 5 << KIND_SHIFT,
        EXPR              = 6 << KIND_SHIFT,
        OPENGL_BUFFER     = 7 << KIND_SHIFT,
        OPENGL_TEXTURE    = 8 << KIND_SHIFT,
        GPU_MAT           = 9 << KIND_SHIFT,
        OCL_MAT           =10 << KIND_SHIFT
    };
    _InputArray();

    _InputArray(const Mat& m);
    template<typename _Tp> _InputArray(const _Tp* vec, int n);
    template<typename _Tp> _InputArray(const vector<_Tp>& vec);
    template<typename _Tp> _InputArray(const vector<vector<_Tp> >& vec);
    _InputArray(const vector<Mat>& vec);
    template<typename _Tp, int m, int n> _InputArray(const Matx<_Tp, m, n>& matx);
    _InputArray(const Scalar& s);
    _InputArray(const double& val);
    virtual Mat getMat(int i=-1) const;



    virtual int kind() const;
    virtual Size size(int i=-1) const;
    virtual size_t total(int i=-1) const;
    virtual int type(int i=-1) const;
    virtual int depth(int i=-1) const;
    virtual int channels(int i=-1) const;
    virtual bool empty() const;

#ifdef OPENCV_CAN_BREAK_BINARY_COMPATIBILITY
    virtual ~_InputArray();
#endif

    int flags;
    void* obj;
    Size sz;
};


enum
{
    DEPTH_MASK_8U = 1 << CV_8U,
    DEPTH_MASK_8S = 1 << CV_8S,
    DEPTH_MASK_16U = 1 << CV_16U,
    DEPTH_MASK_16S = 1 << CV_16S,
    DEPTH_MASK_32S = 1 << CV_32S,
    DEPTH_MASK_32F = 1 << CV_32F,
    DEPTH_MASK_64F = 1 << CV_64F,
    DEPTH_MASK_ALL = (DEPTH_MASK_64F<<1)-1,
    DEPTH_MASK_ALL_BUT_8S = DEPTH_MASK_ALL & ~DEPTH_MASK_8S,
    DEPTH_MASK_FLT = DEPTH_MASK_32F + DEPTH_MASK_64F
};


/*!
 Proxy datatype for passing Mat's and vector<>'s as input parameters
 */
class CV_EXPORTS _OutputArray : public _InputArray
{
public:
    _OutputArray();

    _OutputArray(Mat& m);
    template<typename _Tp> _OutputArray(vector<_Tp>& vec);
    template<typename _Tp> _OutputArray(vector<vector<_Tp> >& vec);
    _OutputArray(vector<Mat>& vec);
    template<typename _Tp, int m, int n> _OutputArray(Matx<_Tp, m, n>& matx);
    template<typename _Tp> _OutputArray(_Tp* vec, int n);

    _OutputArray(const Mat& m);
    template<typename _Tp> _OutputArray(const vector<_Tp>& vec);
    template<typename _Tp> _OutputArray(const vector<vector<_Tp> >& vec);
    _OutputArray(const vector<Mat>& vec);
    template<typename _Tp, int m, int n> _OutputArray(const Matx<_Tp, m, n>& matx);
    template<typename _Tp> _OutputArray(const _Tp* vec, int n);


    virtual bool fixedSize() const;
    virtual bool fixedType() const;
    virtual bool needed() const;
    virtual Mat& getMatRef(int i=-1) const;


    virtual void create(Size sz, int type, int i=-1, bool allowTransposed=false, int fixedDepthMask=0) const;
    virtual void create(int rows, int cols, int type, int i=-1, bool allowTransposed=false, int fixedDepthMask=0) const;
    virtual void create(int dims, const int* size, int type, int i=-1, bool allowTransposed=false, int fixedDepthMask=0) const;
    virtual void release() const;
    virtual void clear() const;

#ifdef OPENCV_CAN_BREAK_BINARY_COMPATIBILITY
    virtual ~_OutputArray();
#endif
};

typedef const _InputArray& InputArray;
typedef InputArray InputArrayOfArrays;
typedef const _OutputArray& OutputArray;
typedef OutputArray InputOutputArray;

CV_EXPORTS OutputArray noArray();

/////////////////////////////////////// Mat ///////////////////////////////////////////

enum { MAGIC_MASK=0xFFFF0000, TYPE_MASK=0x00000FFF, DEPTH_MASK=7 };

static inline size_t getElemSize(int type) { return CV_ELEM_SIZE(type); }

/*!
   Custom array allocator

*/
class CV_EXPORTS MatAllocator
{
public:
    MatAllocator() {}
    virtual ~MatAllocator() {}
    virtual void allocate(int dims, const int* sizes, int type, int*& refcount,
                          uchar*& datastart, uchar*& data, size_t* step) = 0;
    virtual void deallocate(int* refcount, uchar* datastart, uchar* data) = 0;
};


class CV_EXPORTS Mat
{
public:
    //! default constructor
    Mat();
    //! constructs 2D matrix of the specified size and type
    // (_type is CV_8UC1, CV_64FC3, CV_32SC(12) etc.)
    Mat(int rows, int cols, int type);
    Mat(Size size, int type);
    //! constucts 2D matrix and fills it with the specified value _s.
    Mat(int rows, int cols, int type, const Scalar& s);
    Mat(Size size, int type, const Scalar& s);

    //! constructs n-dimensional matrix
    Mat(int ndims, const int* sizes, int type);
    Mat(int ndims, const int* sizes, int type, const Scalar& s);

    //! copy constructor
    Mat(const Mat& m);
    //! constructor for matrix headers pointing to user-allocated data
    Mat(int rows, int cols, int type, void* data, size_t step=AUTO_STEP);
    Mat(Size size, int type, void* data, size_t step=AUTO_STEP);
    Mat(int ndims, const int* sizes, int type, void* data, const size_t* steps=0);

    //! creates a matrix header for a part of the bigger matrix
    Mat(const Mat& m, const Range& rowRange, const Range& colRange=Range::all());
    Mat(const Mat& m, const Rect& roi);
    Mat(const Mat& m, const Range* ranges);
    //! converts old-style CvMat to the new matrix; the data is not copied by default
    Mat(const CvMat* m, bool copyData=false);
    //! converts old-style IplImage to the new matrix; the data is not copied by default
    Mat(const IplImage* img, bool copyData=false);
    //! builds matrix from std::vector with or without copying the data
    template<typename _Tp> explicit Mat(const vector<_Tp>& vec, bool copyData=false);
    //! builds matrix from cv::Vec; the data is copied by default
    template<typename _Tp, int n> explicit Mat(const Vec<_Tp, n>& vec, bool copyData=true);
    //! builds matrix from cv::Matx; the data is copied by default
    template<typename _Tp, int m, int n> explicit Mat(const Matx<_Tp, m, n>& mtx, bool copyData=true);
    //! builds matrix from a 2D point
    template<typename _Tp> explicit Mat(const Point_<_Tp>& pt, bool copyData=true);
    //! builds matrix from a 3D point
    //! builds matrix from comma initializer



    //! destructor - calls release()
    ~Mat();
    //! assignment operators
    Mat& operator = (const Mat& m);

    //! returns a new matrix header for the specified row
    Mat row(int y) const;
    //! returns a new matrix header for the specified column
    Mat col(int x) const;
    //! ... for the specified row span
    Mat rowRange(int startrow, int endrow) const;
    Mat rowRange(const Range& r) const;
    //! ... for the specified column span
    Mat colRange(const Range& r) const;
    //! ... for the specified diagonal
    // (d=0 - the main diagonal,
    //  >0 - a diagonal from the lower half,
    //  <0 - a diagonal from the upper half)
    //! constructs a square diagonal matrix which main diagonal is vector "d"

    //! returns deep copy of the matrix, i.e. the data is copied
    Mat clone() const;
    //! copies the matrix content to "m".
    // It calls m.create(this->size(), this->type()).
    void copyTo( OutputArray m ) const;
    //! copies those matrix elements to "m" that are marked with non-zero mask elements.
    //! converts matrix to another datatype with optional scalng. See cvConvertScale.
    void convertTo( OutputArray m, int rtype, double alpha=1, double beta=0 ) const;

    void assignTo( Mat& m, int type=-1 ) const;

    //! sets every matrix element to s
    Mat& operator = (const Scalar& s);
    //! sets some of the matrix elements to s, according to the mask
    Mat& setTo(InputArray value, InputArray mask=noArray());

    //! allocates new matrix data unless the matrix already has specified size and type.
    // previous data is unreferenced if needed.
    void create(int rows, int cols, int type);
    void create(Size size, int type);
    void create(int ndims, const int* sizes, int type);

    //! increases the reference counter; use with care to avoid memleaks
    void addref();
    //! decreases reference counter;
    // deallocates the data when reference counter reaches 0.
    void release();

    //! deallocates the matrix data
    void deallocate();
    //! internal use function; properly re-allocates _size, _step arrays
    void copySize(const Mat& m);

    //! reserves enough space to fit sz hyper-planes
    void reserve(size_t sz);
    //! resizes matrix to the specified number of hyper-planes
    void resize(size_t sz);
    //! resizes matrix to the specified number of hyper-planes; initializes the newly added elements
    void resize(size_t sz, const Scalar& s);
    //! internal function
    //! adds element to the end of 1d matrix (or possibly multiple elements when _Tp=Mat)
    //! removes several hyper-planes from bottom of the matrix

    //! locates matrix header within a parent matrix. See below
    void locateROI( Size& wholeSize, Point& ofs ) const;
    //! moves/resizes the current matrix ROI inside the parent matrix.
    Mat& adjustROI( int dtop, int dbottom, int dleft, int dright );
    //! extracts a rectangular sub-matrix
    // (this is a generalized form of row, rowRange etc.)
    Mat operator()( Range rowRange, Range colRange ) const;
    Mat operator()( const Rect& roi ) const;
    Mat operator()( const Range* ranges ) const;

   

    //! returns true iff the matrix data is continuous
    // (i.e. when there are no gaps between successive rows).
    // similar to CV_IS_MAT_CONT(cvmat->type)
    bool isContinuous() const;

    //! returns true if the matrix is a submatrix of another matrix
    bool isSubmatrix() const;

    //! returns element size in bytes,
    // similar to CV_ELEM_SIZE(cvmat->type)
    size_t elemSize() const;
    //! returns the size of element channel in bytes.
    size_t elemSize1() const;
    //! returns element type, similar to CV_MAT_TYPE(cvmat->type)
    int type() const;
    //! returns element type, similar to CV_MAT_DEPTH(cvmat->type)
    int depth() const;
    //! returns element type, similar to CV_MAT_CN(cvmat->type)
    int channels() const;
    //! returns step/elemSize1()
    size_t step1(int i=0) const;
    //! returns true if matrix data is NULL
    bool empty() const;
    //! returns the total number of matrix elements
    size_t total() const;

    //! returns N if the matrix is 1-channel (N x ptdim) or ptdim-channel (1 x N) or (N x 1); negative number otherwise

    //! returns pointer to i0-th submatrix along the dimension #0
    uchar* ptr(int i0=0);
    const uchar* ptr(int i0=0) const;

    //! returns pointer to (i0,i1) submatrix along the dimensions #0 and #1
    uchar* ptr(int i0, int i1);
    const uchar* ptr(int i0, int i1) const;

    //! returns pointer to (i0,i1,i3) submatrix along the dimensions #0, #1, #2
    uchar* ptr(int i0, int i1, int i2);
    const uchar* ptr(int i0, int i1, int i2) const;

    //! returns pointer to the matrix element
    uchar* ptr(const int* idx);
    //! returns read-only pointer to the matrix element
    const uchar* ptr(const int* idx) const;

    template<int n> uchar* ptr(const Vec<int, n>& idx);
    template<int n> const uchar* ptr(const Vec<int, n>& idx) const;

    //! template version of the above method
    template<typename _Tp> _Tp* ptr(int i0=0);
    template<typename _Tp> const _Tp* ptr(int i0=0) const;

    template<typename _Tp> _Tp* ptr(int i0, int i1);
    template<typename _Tp> const _Tp* ptr(int i0, int i1) const;

    template<typename _Tp> _Tp* ptr(int i0, int i1, int i2);
    template<typename _Tp> const _Tp* ptr(int i0, int i1, int i2) const;

    template<typename _Tp> _Tp* ptr(const int* idx);
    template<typename _Tp> const _Tp* ptr(const int* idx) const;

    template<typename _Tp, int n> _Tp* ptr(const Vec<int, n>& idx);
    template<typename _Tp, int n> const _Tp* ptr(const Vec<int, n>& idx) const;

    //! the same as above, with the pointer dereferencing
    template<typename _Tp> _Tp& at(int i0=0);
    template<typename _Tp> const _Tp& at(int i0=0) const;

    template<typename _Tp> _Tp& at(int i0, int i1);
    template<typename _Tp> const _Tp& at(int i0, int i1) const;

    template<typename _Tp> _Tp& at(int i0, int i1, int i2);
    template<typename _Tp> const _Tp& at(int i0, int i1, int i2) const;

    template<typename _Tp> _Tp& at(const int* idx);
    template<typename _Tp> const _Tp& at(const int* idx) const;

    template<typename _Tp, int n> _Tp& at(const Vec<int, n>& idx);
    template<typename _Tp, int n> const _Tp& at(const Vec<int, n>& idx) const;

    //! special versions for 2D arrays (especially convenient for referencing image pixels)
    template<typename _Tp> _Tp& at(Point pt);
    template<typename _Tp> const _Tp& at(Point pt) const;

    //! template methods for iteration over matrix elements.
    // the iterators take care of skipping gaps in the end of rows (if any)

    enum { MAGIC_VAL=0x42FF0000, AUTO_STEP=0, CONTINUOUS_FLAG=CV_MAT_CONT_FLAG, SUBMATRIX_FLAG=CV_SUBMAT_FLAG };

    /*! includes several bit-fields:
         - the magic signature
         - continuity flag
         - depth
         - number of channels
     */
    int flags;
    //! the matrix dimensionality, >= 2
    int dims;
    //! the number of rows and columns or (-1, -1) when the matrix has more than 2 dimensions
    int rows, cols;
    //! pointer to the data
    uchar* data;

    //! pointer to the reference counter;
    // when matrix points to user-allocated data, the pointer is NULL
    int* refcount;

    //! helper fields used in locateROI and adjustROI
    uchar* datastart;
    uchar* dataend;
    uchar* datalimit;

    //! custom allocator
    MatAllocator* allocator;

    struct CV_EXPORTS MSize
    {
        MSize(int* _p);
        Size operator()() const;
        const int& operator[](int i) const;
        int& operator[](int i);
        operator const int*() const;
        bool operator == (const MSize& sz) const;
        bool operator != (const MSize& sz) const;

        int* p;
    };

    struct CV_EXPORTS MStep
    {
        MStep();
        MStep(size_t s);
        const size_t& operator[](int i) const;
        size_t& operator[](int i);
        operator size_t() const;
        MStep& operator = (size_t s);

        size_t* p;
        size_t buf[2];
    protected:
        MStep& operator = (const MStep&);
    };

    MSize size;
    MStep step;

protected:
    void initEmpty();
};


/*!
   Random Number Generator

   The class implements RNG using Multiply-with-Carry algorithm
*/
class CV_EXPORTS RNG
{
public:
    enum { UNIFORM=0, NORMAL=1 };

    RNG();
    RNG(uint64 state);
    //! updates the state and returns the next 32-bit unsigned integer random number
    unsigned next();

    operator uchar();
    operator schar();
    operator ushort();
    operator short();
    operator unsigned();
    //! returns a random integer sampled uniformly from [0, N).
    unsigned operator ()(unsigned N);
    unsigned operator ()();
    operator int();
    operator float();
    operator double();

    //! returns Gaussian random variate with mean zero.

    uint64 state;
};




typedef void (*BinaryFunc)(const uchar* src1, size_t step1,
                           const uchar* src2, size_t step2,
                           uchar* dst, size_t step, Size sz,
                           void*);

CV_EXPORTS BinaryFunc getConvertFunc(int sdepth, int ddepth);
CV_EXPORTS BinaryFunc getConvertScaleFunc(int sdepth, int ddepth);
CV_EXPORTS BinaryFunc getCopyMaskFunc(size_t esz);

//! swaps two matrices
CV_EXPORTS void swap(Mat& a, Mat& b);

//! converts array (CvMat or IplImage) to cv::Mat
CV_EXPORTS Mat cvarrToMat(const CvArr* arr, bool copyData=false,
                          bool allowND=true, int coiMode=0);




//! computes sum of array elements
//! computes the number of nonzero array elements
CV_EXPORTS_W int countNonZero( InputArray src );

//! computes per-element minimum of two arrays (dst = min(src1, src2))
CV_EXPORTS_W void min(InputArray src1, InputArray src2, OutputArray dst);
//! computes per-element maximum of two arrays (dst = max(src1, src2))
CV_EXPORTS_W void max(InputArray src1, InputArray src2, OutputArray dst);

//! computes per-element minimum of two arrays (dst = min(src1, src2))
CV_EXPORTS void min(const Mat& src1, const Mat& src2, Mat& dst);
//! computes per-element minimum of array and scalar (dst = min(src1, src2))
CV_EXPORTS void min(const Mat& src1, double src2, Mat& dst);
//! computes per-element maximum of two arrays (dst = max(src1, src2))
CV_EXPORTS void max(const Mat& src1, const Mat& src2, Mat& dst);
//! computes per-element maximum of array and scalar (dst = max(src1, src2))
CV_EXPORTS void max(const Mat& src1, double src2, Mat& dst);

//! computes square root of each matrix element (dst = src**0.5)
CV_EXPORTS_W void sqrt(InputArray src, OutputArray dst);
//! raises the input matrix elements to the specified power (b = a**power)
CV_EXPORTS_W void pow(InputArray src, double power, OutputArray dst);
//! computes exponent of each matrix element (dst = e**src)
//! computes natural logarithm of absolute value of each matrix element: dst = log(abs(src))
//! computes cube root of the argument
//! computes the angle in degrees (0..360) of the vector (x,y)



//! transposes the matrix
CV_EXPORTS_W void transpose(InputArray src, OutputArray dst);

//! computes inverse or pseudo-inverse matrix
CV_EXPORTS_W double invert(InputArray src, OutputArray dst, int flags=DECOMP_LU);
//! solves linear system or a least-square problem
CV_EXPORTS_W bool solve(InputArray src1, InputArray src2,
                        OutputArray dst, int flags=DECOMP_LU);


template<typename _Tp, size_t fixed_size=4096/sizeof(_Tp)+8> class AutoBuffer
{
public:
    typedef _Tp value_type;
    enum { buffer_padding = (int)((16 + sizeof(_Tp) - 1)/sizeof(_Tp)) };

    //! the default contructor
    AutoBuffer();
    //! constructor taking the real buffer size
    AutoBuffer(size_t _size);
    //! destructor. calls deallocate()
    ~AutoBuffer();

    //! allocates the new buffer of size _size. if the _size is small enough, stack-allocated buffer is used
    void allocate(size_t _size);
    //! deallocates the buffer if it was dynamically allocated
    void deallocate();
    //! returns pointer to the real buffer, stack-allocated or head-allocated
    operator _Tp* ();
    //! returns read-only pointer to the real buffer, stack-allocated or head-allocated
    operator const _Tp* () const;

    //! returns number of allocated elements
    size_t getSize() const;

protected:
    //! pointer to the real buffer, can point to buf if the buffer is small enough
    _Tp* ptr;
    //! size of the real buffer
    size_t size;
    //! pre-allocated buffer
    _Tp buf[fixed_size+buffer_padding];
};

/////////////////////////// multi-dimensional dense matrix //////////////////////////

class CV_EXPORTS NAryMatIterator
{
public:
    //! the default constructor
    NAryMatIterator();
    //! the full constructor taking arbitrary number of n-dim matrices
    NAryMatIterator(const Mat** arrays, uchar** ptrs, int narrays=-1);
    //! the full constructor taking arbitrary number of n-dim matrices
    NAryMatIterator(const Mat** arrays, Mat* planes, int narrays=-1);
    //! the separate iterator initialization method
    void init(const Mat** arrays, Mat* planes, uchar** ptrs, int narrays=-1);

    //! proceeds to the next plane of every iterated matrix
    NAryMatIterator& operator ++();
    //! proceeds to the next plane of every iterated matrix (postfix increment operator)
    NAryMatIterator operator ++(int);

    //! the iterated arrays
    const Mat** arrays;
    //! the current planes
    Mat* planes;
    //! data pointers
    uchar** ptrs;
    //! the number of arrays
    int narrays;
    //! the number of hyper-planes that the iterator steps through
    size_t nplanes;
    //! the size of each segment (in elements)
    size_t size;
protected:
    int iterdepth;
    size_t idx;
};


typedef Ptr<CvMemStorage> MemStorage;




/////////////////////////////// Parallel Primitives //////////////////////////////////

// a base body class
class CV_EXPORTS ParallelLoopBody
{
public:
    virtual ~ParallelLoopBody();
    virtual void operator() (const Range& range) const = 0;
};

CV_EXPORTS void parallel_for_(const Range& range, const ParallelLoopBody& body, double nstripes=-1.);
}

#endif // __cplusplus

#include "opencv2/core/operations.hpp"
#include "opencv2/core/mat.hpp"

#endif /*__OPENCV_CORE_HPP__*/

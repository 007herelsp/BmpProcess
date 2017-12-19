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




/*!
  The STL-compilant memory Allocator based on cv::fastMalloc() and cv::fastFree()
*/
template<typename _Tp> class Allocator
{
public:
    typedef _Tp value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    template<typename U> class rebind { typedef Allocator<U> other; };

    explicit Allocator() {}
    ~Allocator() {}
    explicit Allocator(Allocator const&) {}
    template<typename U>
    explicit Allocator(Allocator<U> const&) {}

    // address
    pointer address(reference r) { return &r; }
    const_pointer address(const_reference r) { return &r; }

    pointer allocate(size_type count, const void* =0)
    { return reinterpret_cast<pointer>(fastMalloc(count * sizeof (_Tp))); }

    void deallocate(pointer p, size_type) {fastFree(p); }

    size_type max_size() const
    { return max(static_cast<_Tp>(-1)/sizeof(_Tp), 1); }

    void construct(pointer p, const _Tp& v) { new(static_cast<void*>(p)) _Tp(v); }
    void destroy(pointer p) { p->~_Tp(); }
};

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
    static Matx diag(const diag_type& d);
    static Matx randu(_Tp a, _Tp b);
    static Matx randn(_Tp a, _Tp b);

    //! dot product computed with the default precision
    _Tp dot(const Matx<_Tp, m, n>& v) const;

    //! dot product computed in double-precision arithmetics
    double ddot(const Matx<_Tp, m, n>& v) const;

    //! conversion to another data type
    template<typename T2> operator Matx<T2, m, n>() const;

    //! change the matrix shape
    template<int m1, int n1> Matx<_Tp, m1, n1> reshape() const;

    //! extract part of the matrix
    template<int m1, int n1> Matx<_Tp, m1, n1> get_minor(int i, int j) const;

    //! extract the matrix row
    Matx<_Tp, 1, n> row(int i) const;

    //! extract the matrix column
    Matx<_Tp, m, 1> col(int i) const;

    //! extract the matrix diagonal
    diag_type diag() const;

    //! transpose the matrix
    Matx<_Tp, n, m> t() const;

    //! invert matrix the matrix
    Matx<_Tp, n, m> inv(int method=DECOMP_LU) const;

    //! solve linear system
    template<int l> Matx<_Tp, n, l> solve(const Matx<_Tp, m, l>& rhs, int flags=DECOMP_LU) const;
    Vec<_Tp, n> solve(const Vec<_Tp, m>& rhs, int method) const;

    //! multiply two matrices element-wise
    Matx<_Tp, m, n> mul(const Matx<_Tp, m, n>& a) const;

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
    Vec(_Tp v0, _Tp v1, _Tp v2); //!< 3-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3); //!< 4-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4); //!< 5-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5); //!< 6-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6); //!< 7-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7); //!< 8-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7, _Tp v8); //!< 9-element vector constructor
    Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3, _Tp v4, _Tp v5, _Tp v6, _Tp v7, _Tp v8, _Tp v9); //!< 10-element vector constructor
    explicit Vec(const _Tp* values);

    Vec(const Vec<_Tp, cn>& v);

    static Vec all(_Tp alpha);

    //! per-element multiplication
    Vec mul(const Vec<_Tp, cn>& v) const;

    //! conjugation (makes sense for complex numbers and quaternions)
    Vec conj() const;

    /*!
      cross product of the two 3D vectors.

      For other dimensionalities the exception is raised
    */
    Vec cross(const Vec& v) const;
    //! conversion to another data type
    template<typename T2> operator Vec<T2, cn>() const;
    //! conversion to 4-element CvScalar.
    operator CvScalar() const;

    /*! element access */
    const _Tp& operator [](int i) const;
    _Tp& operator[](int i);
    const _Tp& operator ()(int i) const;
    _Tp& operator ()(int i);

    Vec(const Matx<_Tp, cn, 1>& a, const Matx<_Tp, cn, 1>& b, Matx_AddOp);
    Vec(const Matx<_Tp, cn, 1>& a, const Matx<_Tp, cn, 1>& b, Matx_SubOp);
    template<typename _T2> Vec(const Matx<_Tp, cn, 1>& a, _T2 alpha, Matx_ScaleOp);
};


/* \typedef

   Shorter aliases for the most popular specializations of Vec<T,n>
*/
typedef Vec<uchar, 2> Vec2b;
typedef Vec<uchar, 3> Vec3b;
typedef Vec<uchar, 4> Vec4b;

typedef Vec<short, 2> Vec2s;
typedef Vec<short, 3> Vec3s;
typedef Vec<short, 4> Vec4s;

typedef Vec<ushort, 2> Vec2w;
typedef Vec<ushort, 3> Vec3w;
typedef Vec<ushort, 4> Vec4w;

typedef Vec<int, 2> Vec2i;
typedef Vec<int, 3> Vec3i;
typedef Vec<int, 4> Vec4i;
typedef Vec<int, 6> Vec6i;
typedef Vec<int, 8> Vec8i;

typedef Vec<float, 2> Vec2f;
typedef Vec<float, 3> Vec3f;
typedef Vec<float, 4> Vec4f;
typedef Vec<float, 6> Vec6f;

typedef Vec<double, 2> Vec2d;
typedef Vec<double, 3> Vec3d;
typedef Vec<double, 4> Vec4d;
typedef Vec<double, 6> Vec6d;


//////////////////////////////// Complex //////////////////////////////

/*!
  A complex number class.

  The template class is similar and compatible with std::complex, however it provides slightly
  more convenient access to the real and imaginary parts using through the simple field access, as opposite
  to std::complex::real() and std::complex::imag().
*/
template<typename _Tp> class Complex
{
public:

    //! constructors
    Complex();
    Complex( _Tp _re, _Tp _im=0 );
    Complex( const std::complex<_Tp>& c );

    //! conversion to another data type
    template<typename T2> operator Complex<T2>() const;
    //! conjugation
    Complex conj() const;
    //! conversion to std::complex
    operator std::complex<_Tp>() const;

    _Tp re, im; //< the real and the imaginary parts
};


typedef Complex<float> Complexf;
typedef Complex<double> Complexd;


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
    Point_(const Point_& pt);
    Point_(const CvPoint& pt);
    Point_(const CvPoint2D32f& pt);
    Point_(const Size_<_Tp>& sz);
    Point_(const Vec<_Tp, 2>& v);

    Point_& operator = (const Point_& pt);
    //! conversion to another data type
    template<typename _Tp2> operator Point_<_Tp2>() const;

    //! conversion to the old-style C structures
    operator CvPoint() const;
    operator CvPoint2D32f() const;
    operator Vec<_Tp, 2>() const;

    //! dot product
    _Tp dot(const Point_& pt) const;
    //! dot product computed in double-precision arithmetics
    double ddot(const Point_& pt) const;
    //! cross-product
    double cross(const Point_& pt) const;
    //! checks whether the point is inside the specified rectangle
    bool inside(const Rect_<_Tp>& r) const;

    _Tp x, y; //< the point coordinates
};

/*!
  template 3D point class.

  The class defines a point in 3D space. Data type of the point coordinates is specified
  as a template parameter.

  \see cv::Point3i, cv::Point3f and cv::Point3d
*/
template<typename _Tp> class Point3_
{
public:
    typedef _Tp value_type;

    // various constructors
    Point3_();
    Point3_(_Tp _x, _Tp _y, _Tp _z);
    Point3_(const Point3_& pt);
    explicit Point3_(const Point_<_Tp>& pt);
    Point3_(const CvPoint3D32f& pt);
    Point3_(const Vec<_Tp, 3>& v);

    Point3_& operator = (const Point3_& pt);
    //! conversion to another data type
    template<typename _Tp2> operator Point3_<_Tp2>() const;
    //! conversion to the old-style CvPoint...
    operator CvPoint3D32f() const;
    //! conversion to cv::Vec<>
    operator Vec<_Tp, 3>() const;

    //! dot product
    _Tp dot(const Point3_& pt) const;
    //! dot product computed in double-precision arithmetics
    double ddot(const Point3_& pt) const;
    //! cross product of the 2 3D points
    Point3_ cross(const Point3_& pt) const;

    _Tp x, y, z; //< the point coordinates
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
    Size_(const Size_& sz);
    Size_(const CvSize& sz);
    Size_(const CvSize2D32f& sz);
    Size_(const Point_<_Tp>& pt);

    Size_& operator = (const Size_& sz);
    //! the area (width*height)
    _Tp area() const;

    //! conversion of another data type.
    template<typename _Tp2> operator Size_<_Tp2>() const;

    //! conversion to the old-style OpenCV types
    operator CvSize() const;
    operator CvSize2D32f() const;

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
    Rect_(const CvRect& r);
    Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz);
    Rect_(const Point_<_Tp>& pt1, const Point_<_Tp>& pt2);

    Rect_& operator = ( const Rect_& r );
    //! the top-left corner
    Point_<_Tp> tl() const;
    //! the bottom-right corner
    Point_<_Tp> br() const;

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
typedef Point3_<int> Point3i;
typedef Point3_<float> Point3f;
typedef Point3_<double> Point3d;


/*!
  The rotated 2D rectangle.

  The class represents rotated (i.e. not up-right) rectangles on a plane.
  Each rectangle is described by the center point (mass center), length of each side
  (represented by cv::Size2f structure) and the rotation angle in degrees.
*/
class CV_EXPORTS RotatedRect
{
public:
    //! various constructors
    RotatedRect();
    RotatedRect(const Point2f& center, const Size2f& size, float angle);
    RotatedRect(const CvBox2D& box);

    //! returns 4 vertices of the rectangle
    void points(Point2f pts[]) const;
    //! returns the minimal up-right integer rectangle containing the rotated rectangle
    Rect boundingRect() const;
    //! returns the minimal (exact) floating point rectangle containing the rotated rectangle, not intended for use with images
    Rect_<float> boundingRect2f() const;
    //! conversion to the old-style CvBox2D structure
    operator CvBox2D() const;

    Point2f center; //< the rectangle mass center
    Size2f size;    //< width and height of the rectangle
    float angle;    //< the rotation angle. When the angle is 0, 90, 180, 270 etc., the rectangle becomes an up-right rectangle.
};

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
    operator CvScalar() const;

    //! conversion to another data type
    template<typename T2> operator Scalar_<T2>() const;

    //! per-element product
    Scalar_<_Tp> mul(const Scalar_<_Tp>& t, double scale=1 ) const;

    // returns (v0, -v1, -v2, -v3)
    Scalar_<_Tp> conj() const;

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

/*!
   Informative template class for OpenCV "scalars".

   The class is specialized for each primitive numerical type supported by OpenCV (such as unsigned char or float),
   as well as for more complex types, like cv::Complex<>, std::complex<>, cv::Vec<> etc.
   The common property of all such types (called "scalars", do not confuse it with cv::Scalar_)
   is that each of them is basically a tuple of numbers of the same type. Each "scalar" can be represented
   by the depth id (CV_8U ... CV_64F) and the number of channels.
   OpenCV matrices, 2D or nD, dense or sparse, can store "scalars",
   as long as the number of channels does not exceed CV_CN_MAX.
*/
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

template<typename _Tp> class DataType<Complex<_Tp> >
{
public:
    typedef Complex<_Tp> value_type;
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

template<typename _Tp> class DataType<Point3_<_Tp> >
{
public:
    typedef Point3_<_Tp> value_type;
    typedef Point3_<typename DataType<_Tp>::work_type> work_type;
    typedef _Tp channel_type;
    enum { generic_type = 0, depth = DataDepth<channel_type>::value, channels = 3,
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

/*!
  Smart pointer to dynamically allocated objects.

  This is template pointer-wrapping class that stores the associated reference counter along with the
  object pointer. The class is similar to std::smart_ptr<> from the recent addons to the C++ standard,
  but is shorter to write :) and self-contained (i.e. does add any dependency on the compiler or an external library).

  Basically, you can use "Ptr<MyObjectType> ptr" (or faster "const Ptr<MyObjectType>& ptr" for read-only access)
  everywhere instead of "MyObjectType* ptr", where MyObjectType is some C structure or a C++ class.
  To make it all work, you need to specialize Ptr<>::delete_obj(), like:

  \code
  template<> void Ptr<MyObjectType>::delete_obj() { call_destructor_func(obj); }
  \endcode

  \note{if MyObjectType is a C++ class with a destructor, you do not need to specialize delete_obj(),
  since the default implementation calls "delete obj;"}

  \note{Another good property of the class is that the operations on the reference counter are atomic,
  i.e. it is safe to use the class in multi-threaded applications}
*/
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

template<typename T>
Ptr<T> makePtr();

template<typename T, typename A1>
Ptr<T> makePtr(const A1& a1);

template<typename T, typename A1, typename A2>
Ptr<T> makePtr(const A1& a1, const A2& a2);

template<typename T, typename A1, typename A2, typename A3>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3);

template<typename T, typename A1, typename A2, typename A3, typename A4>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9);

template<typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
Ptr<T> makePtr(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8, const A9& a9, const A10& a10);

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
typedef OutputArray OutputArrayOfArrays;
typedef OutputArray InputOutputArray;
typedef OutputArray InputOutputArrayOfArrays;

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
    template<typename _Tp> explicit Mat(const Point3_<_Tp>& pt, bool copyData=true);
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
    Mat colRange(int startcol, int endcol) const;
    Mat colRange(const Range& r) const;
    //! ... for the specified diagonal
    // (d=0 - the main diagonal,
    //  >0 - a diagonal from the lower half,
    //  <0 - a diagonal from the upper half)
    Mat diag(int d=0) const;
    //! constructs a square diagonal matrix which main diagonal is vector "d"
    static Mat diag(const Mat& d);

    //! returns deep copy of the matrix, i.e. the data is copied
    Mat clone() const;
    //! copies the matrix content to "m".
    // It calls m.create(this->size(), this->type()).
    void copyTo( OutputArray m ) const;
    //! copies those matrix elements to "m" that are marked with non-zero mask elements.
    void copyTo( OutputArray m, InputArray mask ) const;
    //! converts matrix to another datatype with optional scalng. See cvConvertScale.
    void convertTo( OutputArray m, int rtype, double alpha=1, double beta=0 ) const;

    void assignTo( Mat& m, int type=-1 ) const;

    //! sets every matrix element to s
    Mat& operator = (const Scalar& s);
    //! sets some of the matrix elements to s, according to the mask
    Mat& setTo(InputArray value, InputArray mask=noArray());
    //! creates alternative matrix header for the same data, with different
    // number of channels and/or different number of rows. see cvReshape.
    Mat reshape(int cn, int rows=0) const;
    Mat reshape(int cn, int newndims, const int* newsz) const;

    //! matrix transposition by means of matrix expressions
    //! matrix inversion by means of matrix expressions
    //! per-element matrix multiplication by means of matrix expressions

    //! computes cross-product of 2 3D vectors
    Mat cross(InputArray m) const;
    //! computes dot-product
    double dot(InputArray m) const;

    //! Matlab-style matrix initialization

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
    void push_back_(const void* elem);
    //! adds element to the end of 1d matrix (or possibly multiple elements when _Tp=Mat)
    template<typename _Tp> void push_back(const _Tp& elem);
    void push_back(const Mat& m);
    //! removes several hyper-planes from bottom of the matrix
    void pop_back(size_t nelems=1);

    //! locates matrix header within a parent matrix. See below
    void locateROI( Size& wholeSize, Point& ofs ) const;
    //! moves/resizes the current matrix ROI inside the parent matrix.
    Mat& adjustROI( int dtop, int dbottom, int dleft, int dright );
    //! extracts a rectangular sub-matrix
    // (this is a generalized form of row, rowRange etc.)
    Mat operator()( Range rowRange, Range colRange ) const;
    Mat operator()( const Rect& roi ) const;
    Mat operator()( const Range* ranges ) const;

    //! converts header to CvMat; no data is copied
    operator CvMat() const;
    //! converts header to IplImage; no data is copied
    operator IplImage() const;

    template<typename _Tp> operator vector<_Tp>() const;
    template<typename _Tp, int n> operator Vec<_Tp, n>() const;
    template<typename _Tp, int m, int n> operator Matx<_Tp, m, n>() const;

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
    int checkVector(int elemChannels, int depth=-1, bool requireContinuous=true) const;

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
    //! returns uniformly distributed integer random number from [a,b) range
    int uniform(int a, int b);
    //! returns uniformly distributed floating-point random number from [a,b) range
    float uniform(float a, float b);
    //! returns uniformly distributed double-precision floating-point random number from [a,b) range
    double uniform(double a, double b);
    void fill( InputOutputArray mat, int distType, InputArray a, InputArray b, bool saturateRange=false );
    //! returns Gaussian random variate with mean zero.
    double gaussian(double sigma);

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
//! extracts Channel of Interest from CvMat or IplImage and makes cv::Mat out of it.
CV_EXPORTS void extractImageCOI(const CvArr* arr, OutputArray coiimg, int coi=-1);
//! inserts single-channel cv::Mat into a multi-channel CvMat or IplImage
CV_EXPORTS void insertImageCOI(InputArray coiimg, CvArr* arr, int coi=-1);

//! adds one matrix to another (dst = src1 + src2)
CV_EXPORTS_W void add(InputArray src1, InputArray src2, OutputArray dst,
                      InputArray mask=noArray(), int dtype=-1);
//! subtracts one matrix from another (dst = src1 - src2)
CV_EXPORTS_W void subtract(InputArray src1, InputArray src2, OutputArray dst,
                           InputArray mask=noArray(), int dtype=-1);

//! computes element-wise weighted product of the two arrays (dst = scale*src1*src2)
CV_EXPORTS_W void multiply(InputArray src1, InputArray src2,
                           OutputArray dst, double scale=1, int dtype=-1);

//! computes element-wise weighted quotient of the two arrays (dst = scale*src1/src2)
CV_EXPORTS_W void divide(InputArray src1, InputArray src2, OutputArray dst,
                         double scale=1, int dtype=-1);

//! computes element-wise weighted reciprocal of an array (dst = scale/src2)
CV_EXPORTS_W void divide(double scale, InputArray src2,
                         OutputArray dst, int dtype=-1);

//! computes sum of array elements
CV_EXPORTS_AS(sumElems) Scalar sum(InputArray src);
//! computes the number of nonzero array elements
CV_EXPORTS_W int countNonZero( InputArray src );


//! computes mean value of selected array elements
CV_EXPORTS_W Scalar mean(InputArray src, InputArray mask=noArray());

//! computes norm of the selected array part
CV_EXPORTS_W double norm(InputArray src1, int normType=NORM_L2, InputArray mask=noArray());
//! computes norm of selected part of the difference between two arrays
CV_EXPORTS_W double norm(InputArray src1, InputArray src2,
                         int normType=NORM_L2, InputArray mask=noArray());


//! copies selected channels from the input arrays to the selected channels of the output arrays
CV_EXPORTS void mixChannels(const Mat* src, size_t nsrcs, Mat* dst, size_t ndsts,
                            const int* fromTo, size_t npairs);
CV_EXPORTS void mixChannels(const vector<Mat>& src, vector<Mat>& dst,
                            const int* fromTo, size_t npairs);
CV_EXPORTS_W void mixChannels(InputArrayOfArrays src, InputArrayOfArrays dst,
                              const vector<int>& fromTo);

//! extracts a single channel from src (coi is 0-based index)
CV_EXPORTS_W void extractChannel(InputArray src, OutputArray dst, int coi);

//! inserts a single channel to dst (coi is 0-based index)
CV_EXPORTS_W void insertChannel(InputArray src, InputOutputArray dst, int coi);

//! reverses the order of the rows, columns or both in a matrix
CV_EXPORTS_W void flip(InputArray src, OutputArray dst, int flipCode);

//! replicates the input matrix the specified number of times in the horizontal and/or vertical direction
CV_EXPORTS_W void repeat(InputArray src, int ny, int nx, OutputArray dst);
CV_EXPORTS Mat repeat(const Mat& src, int ny, int nx);




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
CV_EXPORTS_W void exp(InputArray src, OutputArray dst);
//! computes natural logarithm of absolute value of each matrix element: dst = log(abs(src))
//! computes cube root of the argument
//! computes the angle in degrees (0..360) of the vector (x,y)

CV_EXPORTS void exp(const float* src, float* dst, int n);

//! converts NaN's to the given number

//! implements generalized matrix product algorithm GEMM from BLAS
CV_EXPORTS_W void gemm(InputArray src1, InputArray src2, double alpha,
                       InputArray src3, double beta, OutputArray dst, int flags=0);
//! multiplies matrix by its transposition from the left or from the right
CV_EXPORTS_W void mulTransposed( InputArray src, OutputArray dst, bool aTa,
                                 InputArray delta=noArray(),
                                 double scale=1, int dtype=-1 );
//! transposes the matrix
CV_EXPORTS_W void transpose(InputArray src, OutputArray dst);
//! performs affine transformation of each element of multi-channel input matrix
CV_EXPORTS_W void transform(InputArray src, OutputArray dst, InputArray m );
//! performs perspective transformation of each element of multi-channel input matrix
CV_EXPORTS_W void perspectiveTransform(InputArray src, OutputArray dst, InputArray m );

//! extends the symmetrical matrix from the lower half or from the upper half
CV_EXPORTS_W void completeSymm(InputOutputArray mtx, bool lowerToUpper=false);
//! initializes scaled identity matrix

//! computes inverse or pseudo-inverse matrix
CV_EXPORTS_W double invert(InputArray src, OutputArray dst, int flags=DECOMP_LU);
//! solves linear system or a least-square problem
CV_EXPORTS_W bool solve(InputArray src1, InputArray src2,
                        OutputArray dst, int flags=DECOMP_LU);

enum
{
    SORT_EVERY_ROW=0,
    SORT_EVERY_COLUMN=1,
    SORT_ASCENDING=0,
    SORT_DESCENDING=16
};

//! sorts independently each matrix row or each matrix column
CV_EXPORTS_W void sort(InputArray src, OutputArray dst, int flags);
//! sorts independently each matrix row or each matrix column
CV_EXPORTS_W void sortIdx(InputArray src, OutputArray dst, int flags);




enum
{
    COVAR_SCRAMBLED=0,
    COVAR_NORMAL=1,
    COVAR_USE_AVG=2,
    COVAR_SCALE=4,
    COVAR_ROWS=8,
    COVAR_COLS=16
};


//! performs forward or inverse 1D or 2D Discrete Cosine Transformation

//! computes element-wise product of the two Fourier spectrums. The second spectrum can optionally be conjugated before the multiplication
CV_EXPORTS_W void mulSpectrums(InputArray a, InputArray b, OutputArray c,
                               int flags, bool conjB=false);
//! computes the minimal vector size vecsize1 >= vecsize so that the dft() of the vector of length vecsize1 can be computed efficiently
CV_EXPORTS_W int getOptimalDFTSize(int vecsize);

/*!
 Various k-Means flags
*/
enum
{
    KMEANS_RANDOM_CENTERS=0, // Chooses random centers for k-Means initialization
    KMEANS_PP_CENTERS=2,     // Uses k-Means++ algorithm for initialization
    KMEANS_USE_INITIAL_LABELS=1 // Uses the user-provided labels for K-Means initialization
};


//! returns the thread-local Random number generator
CV_EXPORTS RNG& theRNG();

//! sets state of the thread-local Random number generator
CV_EXPORTS_W void setRNGSeed(int seed);

//! returns the next unifomly-distributed random number of the specified type
template<typename _Tp> static inline _Tp randu() { return (_Tp)theRNG(); }

//! fills array with uniformly-distributed random numbers from the range [low, high)
CV_EXPORTS_W void randu(InputOutputArray dst, InputArray low, InputArray high);

//! fills array with normally-distributed random numbers with the specified mean and the standard deviation
CV_EXPORTS_W void randn(InputOutputArray dst, InputArray mean, InputArray stddev);

//! shuffles the input array elements
CV_EXPORTS void randShuffle(InputOutputArray dst, double iterFactor=1., RNG* rng=0);
CV_EXPORTS_AS(randShuffle) void randShuffle_(InputOutputArray dst, double iterFactor=1.);

//! draws the line segment (pt1, pt2) in the image
CV_EXPORTS_W void line(CV_IN_OUT Mat& img, Point pt1, Point pt2, const Scalar& color,
                     int thickness=1, int lineType=8, int shift=0);

//! draws an arrow from pt1 to pt2 in the image
CV_EXPORTS_W void arrowedLine(CV_IN_OUT Mat& img, Point pt1, Point pt2, const Scalar& color,
                     int thickness=1, int line_type=8, int shift=0, double tipLength=0.1);

//! draws the rectangle outline or a solid rectangle with the opposite corners pt1 and pt2 in the image
CV_EXPORTS_W void rectangle(CV_IN_OUT Mat& img, Point pt1, Point pt2,
                          const Scalar& color, int thickness=1,
                          int lineType=8, int shift=0);

//! draws the rectangle outline or a solid rectangle covering rec in the image
CV_EXPORTS void rectangle(CV_IN_OUT Mat& img, Rect rec,
                          const Scalar& color, int thickness=1,
                          int lineType=8, int shift=0);

//! draws the circle outline or a solid circle in the image
CV_EXPORTS_W void circle(CV_IN_OUT Mat& img, Point center, int radius,
                       const Scalar& color, int thickness=1,
                       int lineType=8, int shift=0);

//! draws an elliptic arc, ellipse sector or a rotated ellipse in the image
CV_EXPORTS_W void ellipse(CV_IN_OUT Mat& img, Point center, Size axes,
                        double angle, double startAngle, double endAngle,
                        const Scalar& color, int thickness=1,
                        int lineType=8, int shift=0);

//! draws a rotated ellipse in the image
CV_EXPORTS_W void ellipse(CV_IN_OUT Mat& img, const RotatedRect& box, const Scalar& color,
                        int thickness=1, int lineType=8);

/* ----------------------------------------------------------------------------------------- */
/* ADDING A SET OF PREDEFINED MARKERS WHICH COULD BE USED TO HIGHLIGHT POSITIONS IN AN IMAGE */
/* ----------------------------------------------------------------------------------------- */

//! Possible set of marker types used for the drawMarker function
enum MarkerTypes
{
    MARKER_CROSS = 0,           // A crosshair marker shape
    MARKER_TILTED_CROSS = 1,    // A 45 degree tilted crosshair marker shape
    MARKER_STAR = 2,            // A star marker shape, combination of cross and tilted cross
    MARKER_DIAMOND = 3,         // A diamond marker shape
    MARKER_SQUARE = 4,          // A square marker shape
    MARKER_TRIANGLE_UP = 5,     // An upwards pointing triangle marker shape
    MARKER_TRIANGLE_DOWN = 6    // A downwards pointing triangle marker shape
};

/** @brief Draws a marker on a predefined position in an image.

The function drawMarker draws a marker on a given position in the image. For the moment several
marker types are supported (`MARKER_CROSS`, `MARKER_TILTED_CROSS`, `MARKER_STAR`, `MARKER_DIAMOND`, `MARKER_SQUARE`,
`MARKER_TRIANGLE_UP` and `MARKER_TRIANGLE_DOWN`).

@param img Image.
@param position The point where the crosshair is positioned.
@param markerType The specific type of marker you want to use, see
@param color Line color.
@param thickness Line thickness.
@param line_type Type of the line, see cv::LineTypes
@param markerSize The length of the marker axis [default = 20 pixels]
 */
CV_EXPORTS_W void drawMarker(CV_IN_OUT Mat& img, Point position, const Scalar& color,
                             int markerType = MARKER_CROSS, int markerSize=20, int thickness=1,
                             int line_type=8);

/* ----------------------------------------------------------------------------------------- */
/* END OF MARKER SECTION */
/* ----------------------------------------------------------------------------------------- */

//! draws a filled convex polygon in the image
CV_EXPORTS void fillConvexPoly(Mat& img, const Point* pts, int npts,
                               const Scalar& color, int lineType=8,
                               int shift=0);
CV_EXPORTS_W void fillConvexPoly(InputOutputArray img, InputArray points,
                                 const Scalar& color, int lineType=8,
                                 int shift=0);

//! fills an area bounded by one or more polygons
CV_EXPORTS void fillPoly(Mat& img, const Point** pts,
                         const int* npts, int ncontours,
                         const Scalar& color, int lineType=8, int shift=0,
                         Point offset=Point() );

CV_EXPORTS_W void fillPoly(InputOutputArray img, InputArrayOfArrays pts,
                           const Scalar& color, int lineType=8, int shift=0,
                           Point offset=Point() );

//! draws one or more polygonal curves
CV_EXPORTS void polylines(Mat& img, const Point** pts, const int* npts,
                          int ncontours, bool isClosed, const Scalar& color,
                          int thickness=1, int lineType=8, int shift=0 );

CV_EXPORTS_W void polylines(InputOutputArray img, InputArrayOfArrays pts,
                            bool isClosed, const Scalar& color,
                            int thickness=1, int lineType=8, int shift=0 );

//! clips the line segment by the rectangle Rect(0, 0, imgSize.width, imgSize.height)
CV_EXPORTS bool clipLine(Size imgSize, CV_IN_OUT Point& pt1, CV_IN_OUT Point& pt2);
CV_EXPORTS bool clipLine(Size2l imgSize, CV_IN_OUT Point2l& pt1, CV_IN_OUT Point2l& pt2);

//! clips the line segment by the rectangle imgRect
CV_EXPORTS_W bool clipLine(Rect imgRect, CV_OUT CV_IN_OUT Point& pt1, CV_OUT CV_IN_OUT Point& pt2);



enum
{
    FONT_HERSHEY_SIMPLEX = 0,
    FONT_HERSHEY_PLAIN = 1,
    FONT_HERSHEY_DUPLEX = 2,
    FONT_HERSHEY_COMPLEX = 3,
    FONT_HERSHEY_TRIPLEX = 4,
    FONT_HERSHEY_COMPLEX_SMALL = 5,
    FONT_HERSHEY_SCRIPT_SIMPLEX = 6,
    FONT_HERSHEY_SCRIPT_COMPLEX = 7,
    FONT_ITALIC = 16
};



/*!
 Comma-separated Matrix Initializer

 The class instances are usually not created explicitly.
 Instead, they are created on "matrix << firstValue" operator.

 The sample below initializes 2x2 rotation matrix:

 \code
 double angle = 30, a = cos(angle*CV_PI/180), b = sin(angle*CV_PI/180);
 Mat R = (Mat_<double>(2,2) << a, -b, b, a);
 \endcode
*/


template<typename _Tp, int m, int n> class MatxCommaInitializer
{
public:
    MatxCommaInitializer(Matx<_Tp, m, n>* _mtx);
    template<typename T2> MatxCommaInitializer<_Tp, m, n>& operator , (T2 val);
    Matx<_Tp, m, n> operator *() const;

    Matx<_Tp, m, n>* dst;
    int idx;
};

template<typename _Tp, int m> class VecCommaInitializer : public MatxCommaInitializer<_Tp, m, 1>
{
public:
    VecCommaInitializer(Vec<_Tp, m>* _vec);
    template<typename T2> VecCommaInitializer<_Tp, m>& operator , (T2 val);
    Vec<_Tp, m> operator *() const;
};

/*!
 Automatically Allocated Buffer Class

 The class is used for temporary buffers in functions and methods.
 If a temporary buffer is usually small (a few K's of memory),
 but its size depends on the parameters, it makes sense to create a small
 fixed-size array on stack and use it if it's large enough. If the required buffer size
 is larger than the fixed size, another buffer of sufficient size is allocated dynamically
 and released after the processing. Therefore, in typical cases, when the buffer size is small,
 there is no overhead associated with malloc()/free().
 At the same time, there is no limit on the size of processed data.

 This is what AutoBuffer does. The template takes 2 parameters - type of the buffer elements and
 the number of stack-allocated elements. Here is how the class is used:

 \code
 void my_func(const cv::Mat& m)
 {
    cv::AutoBuffer<float, 1000> buf; // create automatic buffer containing 1000 floats

    buf.allocate(m.rows); // if m.rows <= 1000, the pre-allocated buffer is used,
                          // otherwise the buffer of "m.rows" floats will be allocated
                          // dynamically and deallocated in cv::AutoBuffer destructor
    ...
 }
 \endcode
*/
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

/*!
 n-Dimensional Dense Matrix Iterator Class.

 The class cv::NAryMatIterator is used for iterating over one or more n-dimensional dense arrays (cv::Mat's).

 The iterator is completely different from cv::Mat_ and cv::SparseMat_ iterators.
 It iterates through the slices (or planes), not the elements, where "slice" is a continuous part of the arrays.

 Here is the example on how the iterator can be used to normalize 3D histogram:

 \code
 void normalizeColorHist(Mat& hist)
 {
 #if 1
     // intialize iterator (the style is different from STL).
     // after initialization the iterator will contain
     // the number of slices or planes
     // the iterator will go through
     Mat* arrays[] = { &hist, 0 };
     Mat planes[1];
     NAryMatIterator it(arrays, planes);
     double s = 0;
     // iterate through the matrix. on each iteration
     // it.planes[i] (of type Mat) will be set to the current plane of
     // i-th n-dim matrix passed to the iterator constructor.
     for(int p = 0; p < it.nplanes; p++, ++it)
        s += sum(it.planes[0])[0];
     it = NAryMatIterator(hist);
     s = 1./s;
     for(int p = 0; p < it.nplanes; p++, ++it)
        it.planes[0] *= s;
 #elif 1
     // this is a shorter implementation of the above
     // using built-in operations on Mat
     double s = sum(hist)[0];
     hist.convertTo(hist, hist.type(), 1./s, 0);
 #else
     // and this is even shorter one
     // (assuming that the histogram elements are non-negative)
     normalize(hist, hist, 1, 0, NORM_L1);
 #endif
 }
 \endcode

 You can iterate through several matrices simultaneously as long as they have the same geometry
 (dimensionality and all the dimension sizes are the same), which is useful for binary
 and n-ary operations on such matrices. Just pass those matrices to cv::MatNDIterator.
 Then, during the iteration it.planes[0], it.planes[1], ... will
 be the slices of the corresponding matrices
*/
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

//typedef NAryMatIterator NAryMatNDIterator;

typedef void (*ConvertData)(const void* from, void* to, int cn);
typedef void (*ConvertScaleData)(const void* from, void* to, int cn, double alpha, double beta);

//! returns the function for converting pixels from one data type to another
CV_EXPORTS ConvertData getConvertElem(int fromType, int toType);
//! returns the function for converting pixels from one data type to another with the optional scaling
CV_EXPORTS ConvertScaleData getConvertScaleElem(int fromType, int toType);



////////////// convenient wrappers for operating old-style dynamic structures //////////////

template<typename _Tp> class SeqIterator;

typedef Ptr<CvMemStorage> MemStorage;

/*!
 Template Sequence Class derived from CvSeq

 The class provides more convenient access to sequence elements,
 STL-style operations and iterators.

 \note The class is targeted for simple data types,
    i.e. no constructors or destructors
    are called for the sequence elements.
*/
template<typename _Tp> class Seq
{
public:
    typedef SeqIterator<_Tp> iterator;
    typedef SeqIterator<_Tp> const_iterator;

    //! the default constructor
    Seq();
    //! the constructor for wrapping CvSeq structure. The real element type in CvSeq should match _Tp.
    Seq(const CvSeq* seq);
    //! creates the empty sequence that resides in the specified storage
    Seq(MemStorage& storage, int headerSize = sizeof(CvSeq));
    //! returns read-write reference to the specified element
    _Tp& operator [](int idx);
    //! returns read-only reference to the specified element
    const _Tp& operator[](int idx) const;
    //! returns iterator pointing to the beginning of the sequence
    SeqIterator<_Tp> begin() const;
    //! returns iterator pointing to the element following the last sequence element
    SeqIterator<_Tp> end() const;
    //! returns the number of elements in the sequence
    size_t size() const;
    //! returns the type of sequence elements (CV_8UC1 ... CV_64FC(CV_CN_MAX) ...)
    int type() const;
    //! returns the depth of sequence elements (CV_8U ... CV_64F)
    int depth() const;
    //! returns the number of channels in each sequence element
    int channels() const;
    //! returns the size of each sequence element
    size_t elemSize() const;
    //! returns index of the specified sequence element
    size_t index(const _Tp& elem) const;
    //! appends the specified element to the end of the sequence
    void push_back(const _Tp& elem);
    //! appends the specified element to the front of the sequence
    void push_front(const _Tp& elem);
    //! appends zero or more elements to the end of the sequence
    void push_back(const _Tp* elems, size_t count);
    //! appends zero or more elements to the front of the sequence
    void push_front(const _Tp* elems, size_t count);
    //! inserts the specified element to the specified position
    void insert(int idx, const _Tp& elem);
    //! inserts zero or more elements to the specified position
    void insert(int idx, const _Tp* elems, size_t count);
    //! removes element at the specified position
    void remove(int idx);
    //! removes the specified subsequence
    void remove(const Range& r);

    //! returns reference to the first sequence element
    _Tp& front();
    //! returns read-only reference to the first sequence element
    const _Tp& front() const;
    //! returns reference to the last sequence element
    _Tp& back();
    //! returns read-only reference to the last sequence element
    const _Tp& back() const;
    //! returns true iff the sequence contains no elements
    bool empty() const;

    //! removes all the elements from the sequence
    void clear();
    //! removes the first element from the sequence
    void pop_front();
    //! removes the last element from the sequence
    void pop_back();
    //! removes zero or more elements from the beginning of the sequence
    void pop_front(_Tp* elems, size_t count);
    //! removes zero or more elements from the end of the sequence
    void pop_back(_Tp* elems, size_t count);

    //! copies the whole sequence or the sequence slice to the specified vector
    void copyTo(vector<_Tp>& vec, const Range& range=Range::all()) const;
    //! returns the vector containing all the sequence elements
    operator vector<_Tp>() const;

    CvSeq* seq;
};


/*!
 STL-style Sequence Iterator inherited from the CvSeqReader structure
*/
template<typename _Tp> class SeqIterator : public CvSeqReader
{
public:
    //! the default constructor
    SeqIterator();
    //! the constructor setting the iterator to the beginning or to the end of the sequence
    SeqIterator(const Seq<_Tp>& seq, bool seekEnd=false);
    //! positions the iterator within the sequence
    void seek(size_t pos);
    //! reports the current iterator position
    size_t tell() const;
    //! returns reference to the current sequence element
    _Tp& operator *();
    //! returns read-only reference to the current sequence element
    const _Tp& operator *() const;
    //! moves iterator to the next sequence element
    SeqIterator& operator ++();
    //! moves iterator to the next sequence element
    SeqIterator operator ++(int) const;
    //! moves iterator to the previous sequence element
    SeqIterator& operator --();
    //! moves iterator to the previous sequence element
    SeqIterator operator --(int) const;

    //! moves iterator forward by the specified offset (possibly negative)
    SeqIterator& operator +=(int);
    //! moves iterator backward by the specified offset (possibly negative)
    SeqIterator& operator -=(int);

    // this is index of the current element module seq->total*2
    // (to distinguish between 0 and seq->total)
    int index;
};


class CV_EXPORTS Algorithm;
class CV_EXPORTS AlgorithmInfo;
struct CV_EXPORTS AlgorithmInfoData;

template<typename _Tp> struct ParamType {};

/*!
  Base class for high-level OpenCV algorithms
*/
class CV_EXPORTS_W Algorithm
{
public:
    Algorithm();
    virtual ~Algorithm();
    string name() const;

    template<typename _Tp> typename ParamType<_Tp>::member_type get(const string& name) const;
    template<typename _Tp> typename ParamType<_Tp>::member_type get(const char* name) const;

    CV_WRAP int getInt(const string& name) const;
    CV_WRAP double getDouble(const string& name) const;
    CV_WRAP bool getBool(const string& name) const;
    CV_WRAP string getString(const string& name) const;
    CV_WRAP Mat getMat(const string& name) const;
    CV_WRAP Ptr<Algorithm> getAlgorithm(const string& name) const;

    void set(const string& name, int value);
    void set(const string& name, double value);
    void set(const string& name, bool value);
    void set(const string& name, const string& value);
    void set(const string& name, const Mat& value);
    void set(const string& name, const vector<Mat>& value);
    void set(const string& name, const Ptr<Algorithm>& value);
    template<typename _Tp> void set(const string& name, const Ptr<_Tp>& value);

    CV_WRAP void setInt(const string& name, int value);
    CV_WRAP void setDouble(const string& name, double value);
    CV_WRAP void setBool(const string& name, bool value);
    CV_WRAP void setString(const string& name, const string& value);
    CV_WRAP void setMat(const string& name, const Mat& value);
    CV_WRAP void setMatVector(const string& name, const vector<Mat>& value);
    CV_WRAP void setAlgorithm(const string& name, const Ptr<Algorithm>& value);
    template<typename _Tp> void setAlgorithm(const string& name, const Ptr<_Tp>& value);

    void set(const char* name, int value);
    void set(const char* name, double value);
    void set(const char* name, bool value);
    void set(const char* name, const string& value);
    void set(const char* name, const Mat& value);
    void set(const char* name, const vector<Mat>& value);
    void set(const char* name, const Ptr<Algorithm>& value);
    template<typename _Tp> void set(const char* name, const Ptr<_Tp>& value);

    void setInt(const char* name, int value);
    void setDouble(const char* name, double value);
    void setBool(const char* name, bool value);
    void setString(const char* name, const string& value);
    void setMat(const char* name, const Mat& value);
    void setMatVector(const char* name, const vector<Mat>& value);
    void setAlgorithm(const char* name, const Ptr<Algorithm>& value);
    template<typename _Tp> void setAlgorithm(const char* name, const Ptr<_Tp>& value);

    CV_WRAP string paramHelp(const string& name) const;
    int paramType(const char* name) const;
    CV_WRAP int paramType(const string& name) const;
    CV_WRAP void getParams(CV_OUT vector<string>& names) const;



    typedef Algorithm* (*Constructor)(void);
    typedef int (Algorithm::*Getter)() const;
    typedef void (Algorithm::*Setter)(int);

    CV_WRAP static void getList(CV_OUT vector<string>& algorithms);
    CV_WRAP static Ptr<Algorithm> _create(const string& name);
    template<typename _Tp> static Ptr<_Tp> create(const string& name);

    virtual AlgorithmInfo* info() const /* TODO: make it = 0;*/ { return 0; }
};


class CV_EXPORTS AlgorithmInfo
{
public:
    friend class Algorithm;
    AlgorithmInfo(const string& name, Algorithm::Constructor create);
    ~AlgorithmInfo();
    void get(const Algorithm* algo, const char* name, int argType, void* value) const;
    void addParam_(Algorithm& algo, const char* name, int argType,
                   void* value, bool readOnly,
                   Algorithm::Getter getter, Algorithm::Setter setter,
                   const string& help=string());
    string paramHelp(const char* name) const;
    int paramType(const char* name) const;
    void getParams(vector<string>& names) const;

    string name() const;

    void addParam(Algorithm& algo, const char* name,
                  int& value, bool readOnly=false,
                  int (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(int)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  short& value, bool readOnly=false,
                  int (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(int)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  bool& value, bool readOnly=false,
                  int (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(int)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  double& value, bool readOnly=false,
                  double (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(double)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  string& value, bool readOnly=false,
                  string (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const string&)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  Mat& value, bool readOnly=false,
                  Mat (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const Mat&)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  vector<Mat>& value, bool readOnly=false,
                  vector<Mat> (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const vector<Mat>&)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  Ptr<Algorithm>& value, bool readOnly=false,
                  Ptr<Algorithm> (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const Ptr<Algorithm>&)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  float& value, bool readOnly=false,
                  float (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(float)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  unsigned int& value, bool readOnly=false,
                  unsigned int (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(unsigned int)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  uint64& value, bool readOnly=false,
                  uint64 (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(uint64)=0,
                  const string& help=string());
    void addParam(Algorithm& algo, const char* name,
                  uchar& value, bool readOnly=false,
                  uchar (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(uchar)=0,
                  const string& help=string());
    template<typename _Tp, typename _Base> void addParam(Algorithm& algo, const char* name,
                  Ptr<_Tp>& value, bool readOnly=false,
                  Ptr<_Tp> (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const Ptr<_Tp>&)=0,
                  const string& help=string());
    template<typename _Tp> void addParam(Algorithm& algo, const char* name,
                  Ptr<_Tp>& value, bool readOnly=false,
                  Ptr<_Tp> (Algorithm::*getter)()=0,
                  void (Algorithm::*setter)(const Ptr<_Tp>&)=0,
                  const string& help=string());
protected:
    AlgorithmInfoData* data;
    void set(Algorithm* algo, const char* name, int argType,
              const void* value, bool force=false) const;
};


struct CV_EXPORTS Param
{
    enum { INT=0, BOOLEAN=1, REAL=2, STRING=3, MAT=4, MAT_VECTOR=5, ALGORITHM=6, FLOAT=7, UNSIGNED_INT=8, UINT64=9, SHORT=10, UCHAR=11 };

    Param();
    Param(int _type, bool _readonly, int _offset,
          Algorithm::Getter _getter=0,
          Algorithm::Setter _setter=0,
          const string& _help=string());
    int type;
    int offset;
    bool readonly;
    Algorithm::Getter getter;
    Algorithm::Setter setter;
    string help;
};

template<> struct ParamType<bool>
{
    typedef bool const_param_type;
    typedef bool member_type;

    enum { type = Param::BOOLEAN };
};

template<> struct ParamType<int>
{
    typedef int const_param_type;
    typedef int member_type;

    enum { type = Param::INT };
};

template<> struct ParamType<short>
{
    typedef int const_param_type;
    typedef int member_type;

    enum { type = Param::SHORT };
};

template<> struct ParamType<double>
{
    typedef double const_param_type;
    typedef double member_type;

    enum { type = Param::REAL };
};

template<> struct ParamType<string>
{
    typedef const string& const_param_type;
    typedef string member_type;

    enum { type = Param::STRING };
};

template<> struct ParamType<Mat>
{
    typedef const Mat& const_param_type;
    typedef Mat member_type;

    enum { type = Param::MAT };
};

template<> struct ParamType<vector<Mat> >
{
    typedef const vector<Mat>& const_param_type;
    typedef vector<Mat> member_type;

    enum { type = Param::MAT_VECTOR };
};

template<> struct ParamType<Algorithm>
{
    typedef const Ptr<Algorithm>& const_param_type;
    typedef Ptr<Algorithm> member_type;

    enum { type = Param::ALGORITHM };
};

template<> struct ParamType<float>
{
    typedef float const_param_type;
    typedef float member_type;

    enum { type = Param::FLOAT };
};

template<> struct ParamType<unsigned>
{
    typedef unsigned const_param_type;
    typedef unsigned member_type;

    enum { type = Param::UNSIGNED_INT };
};

template<> struct ParamType<uint64>
{
    typedef uint64 const_param_type;
    typedef uint64 member_type;

    enum { type = Param::UINT64 };
};

template<> struct ParamType<uchar>
{
    typedef uchar const_param_type;
    typedef uchar member_type;

    enum { type = Param::UCHAR };
};



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

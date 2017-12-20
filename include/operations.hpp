#ifndef __OPENCV_CORE_OPERATIONS_HPP__
#define __OPENCV_CORE_OPERATIONS_HPP__

#ifndef SKIP_INCLUDES
  #include <string.h>
  #include <limits.h>
  #include <stddef.h>
#endif // SKIP_INCLUDES

#ifdef __cplusplus

/////// exchange-add operation for atomic operations on reference counters ///////
  static inline int CV_XADD(int* addr, int delta)
  { int tmp = *addr; *addr += delta; return tmp; }

#include <limits>


namespace cv
{

using std::cos;
using std::sin;
using std::max;
using std::min;
using std::exp;
using std::pow;
using std::sqrt;


/////////////// saturate_cast (used in image & signal processing) ///////////////////

template<typename _Tp> static inline _Tp saturate_cast(uchar v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(schar v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(ushort v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(short v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(unsigned v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(int v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(float v) { return _Tp(v); }
template<typename _Tp> static inline _Tp saturate_cast(double v) { return _Tp(v); }

template<> inline uchar saturate_cast<uchar>(schar v)
{ return (uchar)std::max((int)v, 0); }
template<> inline uchar saturate_cast<uchar>(ushort v)
{ return (uchar)std::min((unsigned)v, (unsigned)UCHAR_MAX); }
template<> inline uchar saturate_cast<uchar>(int v)
{ return (uchar)((unsigned)v <= UCHAR_MAX ? v : v > 0 ? UCHAR_MAX : 0); }
template<> inline uchar saturate_cast<uchar>(short v)
{ return saturate_cast<uchar>((int)v); }
template<> inline uchar saturate_cast<uchar>(unsigned v)
{ return (uchar)std::min(v, (unsigned)UCHAR_MAX); }
template<> inline uchar saturate_cast<uchar>(float v)
{ int iv = cvRound(v); return saturate_cast<uchar>(iv); }
template<> inline uchar saturate_cast<uchar>(double v)
{ int iv = cvRound(v); return saturate_cast<uchar>(iv); }

template<> inline schar saturate_cast<schar>(uchar v)
{ return (schar)std::min((int)v, SCHAR_MAX); }
template<> inline schar saturate_cast<schar>(ushort v)
{ return (schar)std::min((unsigned)v, (unsigned)SCHAR_MAX); }
template<> inline schar saturate_cast<schar>(int v)
{
    return (schar)((unsigned)(v-SCHAR_MIN) <= (unsigned)UCHAR_MAX ?
                v : v > 0 ? SCHAR_MAX : SCHAR_MIN);
}
template<> inline schar saturate_cast<schar>(short v)
{ return saturate_cast<schar>((int)v); }
template<> inline schar saturate_cast<schar>(unsigned v)
{ return (schar)std::min(v, (unsigned)SCHAR_MAX); }

template<> inline schar saturate_cast<schar>(float v)
{ int iv = cvRound(v); return saturate_cast<schar>(iv); }
template<> inline schar saturate_cast<schar>(double v)
{ int iv = cvRound(v); return saturate_cast<schar>(iv); }

template<> inline ushort saturate_cast<ushort>(schar v)
{ return (ushort)std::max((int)v, 0); }
template<> inline ushort saturate_cast<ushort>(short v)
{ return (ushort)std::max((int)v, 0); }
template<> inline ushort saturate_cast<ushort>(int v)
{ return (ushort)((unsigned)v <= (unsigned)USHRT_MAX ? v : v > 0 ? USHRT_MAX : 0); }
template<> inline ushort saturate_cast<ushort>(unsigned v)
{ return (ushort)std::min(v, (unsigned)USHRT_MAX); }
template<> inline ushort saturate_cast<ushort>(float v)
{ int iv = cvRound(v); return saturate_cast<ushort>(iv); }
template<> inline ushort saturate_cast<ushort>(double v)
{ int iv = cvRound(v); return saturate_cast<ushort>(iv); }

template<> inline short saturate_cast<short>(ushort v)
{ return (short)std::min((int)v, SHRT_MAX); }
template<> inline short saturate_cast<short>(int v)
{
    return (short)((unsigned)(v - SHRT_MIN) <= (unsigned)USHRT_MAX ?
            v : v > 0 ? SHRT_MAX : SHRT_MIN);
}
template<> inline short saturate_cast<short>(unsigned v)
{ return (short)std::min(v, (unsigned)SHRT_MAX); }
template<> inline short saturate_cast<short>(float v)
{ int iv = cvRound(v); return saturate_cast<short>(iv); }
template<> inline short saturate_cast<short>(double v)
{ int iv = cvRound(v); return saturate_cast<short>(iv); }



//////////////////////////////// Matx /////////////////////////////////


template<typename _Tp, int m, int n> inline Matx<_Tp, m, n>::Matx()
{
    for(int i = 0; i < channels; i++) val[i] = _Tp(0);
}


template<typename _Tp, int m, int n> inline Matx<_Tp, m, n>::Matx(const _Tp* values)
{
    for( int i = 0; i < channels; i++ ) val[i] = values[i];
}


/////////////////////////// short vector (Vec) /////////////////////////////

template<typename _Tp, int cn> inline Vec<_Tp, cn>::Vec()
{}


template<typename _Tp, int cn> inline Vec<_Tp, cn>::Vec(_Tp v0, _Tp v1)
    : Matx<_Tp, cn, 1>(v0, v1)
{}



template<typename _Tp, int cn> inline Vec<_Tp, cn>::Vec(const _Tp* values)
    : Matx<_Tp, cn, 1>(values)
{}


template<typename _Tp, int cn> inline Vec<_Tp, cn>::Vec(const Vec<_Tp, cn>& m)
    : Matx<_Tp, cn, 1>(m.val)
{}



template<typename _Tp, int cn> inline const _Tp& Vec<_Tp, cn>::operator [](int i) const
{
    CV_DbgAssert( (unsigned)i < (unsigned)cn );
    return this->val[i];
}

template<typename _Tp, int cn> inline _Tp& Vec<_Tp, cn>::operator [](int i)
{
    CV_DbgAssert( (unsigned)i < (unsigned)cn );
    return this->val[i];
}

template<typename _Tp, int cn> inline const _Tp& Vec<_Tp, cn>::operator ()(int i) const
{
    CV_DbgAssert( (unsigned)i < (unsigned)cn );
    return this->val[i];
}

template<typename _Tp, int cn> inline _Tp& Vec<_Tp, cn>::operator ()(int i)
{
    CV_DbgAssert( (unsigned)i < (unsigned)cn );
    return this->val[i];
}





//////////////////////////////// 2D Point ////////////////////////////////

template<typename _Tp> inline Point_<_Tp>::Point_() : x(0), y(0) {}
template<typename _Tp> inline Point_<_Tp>::Point_(_Tp _x, _Tp _y) : x(_x), y(_y) {}

template<typename _Tp> inline Point_<_Tp>& Point_<_Tp>::operator = (const Point_& pt)
{ x = pt.x; y = pt.y; return *this; }



//////////////////////////////// Size ////////////////////////////////

template<typename _Tp> inline Size_<_Tp>::Size_()
    : width(0), height(0) {}
template<typename _Tp> inline Size_<_Tp>::Size_(_Tp _width, _Tp _height)
    : width(_width), height(_height) {}


template<typename _Tp> inline Size_<_Tp>::operator CvSize() const
{ return cvSize(saturate_cast<int>(width), saturate_cast<int>(height)); }


template<typename _Tp> inline Size_<_Tp>& Size_<_Tp>::operator = (const Size_<_Tp>& sz)
{ width = sz.width; height = sz.height; return *this; }

template<typename _Tp> inline _Tp Size_<_Tp>::area() const { return width*height; }


template<typename _Tp> static inline bool operator == (const Size_<_Tp>& a, const Size_<_Tp>& b)
{ return a.width == b.width && a.height == b.height; }
template<typename _Tp> static inline bool operator != (const Size_<_Tp>& a, const Size_<_Tp>& b)
{ return a.width != b.width || a.height != b.height; }

//////////////////////////////// Rect ////////////////////////////////


template<typename _Tp> inline Rect_<_Tp>::Rect_() : x(0), y(0), width(0), height(0) {}
template<typename _Tp> inline Rect_<_Tp>::Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height) : x(_x), y(_y), width(_width), height(_height) {}
template<typename _Tp> inline Rect_<_Tp>::Rect_(const Rect_<_Tp>& r) : x(r.x), y(r.y), width(r.width), height(r.height) {}


template<typename _Tp> inline Rect_<_Tp>& Rect_<_Tp>::operator = ( const Rect_<_Tp>& r )
{ x = r.x; y = r.y; width = r.width; height = r.height; return *this; }


template<typename _Tp> inline Size_<_Tp> Rect_<_Tp>::size() const { return Size_<_Tp>(width, height); }
template<typename _Tp> inline _Tp Rect_<_Tp>::area() const { return width*height; }

template<typename _Tp> template<typename _Tp2> inline Rect_<_Tp>::operator Rect_<_Tp2>() const
{ return Rect_<_Tp2>(saturate_cast<_Tp2>(x), saturate_cast<_Tp2>(y),
                     saturate_cast<_Tp2>(width), saturate_cast<_Tp2>(height)); }


template<typename _Tp> inline bool Rect_<_Tp>::contains(const Point_<_Tp>& pt) const
{ return x <= pt.x && pt.x < x + width && y <= pt.y && pt.y < y + height; }

template<typename _Tp> static inline bool operator == (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

template<typename _Tp> static inline bool operator != (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
{
    return a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height;
}

template<typename _Tp> static inline Rect_<_Tp> operator + (const Rect_<_Tp>& a, const Point_<_Tp>& b)
{
    return Rect_<_Tp>( a.x + b.x, a.y + b.y, a.width, a.height );
}

template<typename _Tp> static inline Rect_<_Tp> operator - (const Rect_<_Tp>& a, const Point_<_Tp>& b)
{
    return Rect_<_Tp>( a.x - b.x, a.y - b.y, a.width, a.height );
}

template<typename _Tp> static inline Rect_<_Tp> operator + (const Rect_<_Tp>& a, const Size_<_Tp>& b)
{
    return Rect_<_Tp>( a.x, a.y, a.width + b.width, a.height + b.height );
}


template<typename _Tp> inline bool Point_<_Tp>::inside( const Rect_<_Tp>& r ) const
{
    return r.contains(*this);
}



//////////////////////////////// Scalar_ ///////////////////////////////

template<typename _Tp> inline Scalar_<_Tp>::Scalar_()
{ this->val[0] = this->val[1] = this->val[2] = this->val[3] = 0; }

template<typename _Tp> inline Scalar_<_Tp>::Scalar_(_Tp v0, _Tp v1, _Tp v2, _Tp v3)
{ this->val[0] = v0; this->val[1] = v1; this->val[2] = v2; this->val[3] = v3; }

template<typename _Tp> inline Scalar_<_Tp>::Scalar_(const CvScalar& s)
{
    this->val[0] = saturate_cast<_Tp>(s.val[0]);
    this->val[1] = saturate_cast<_Tp>(s.val[1]);
    this->val[2] = saturate_cast<_Tp>(s.val[2]);
    this->val[3] = saturate_cast<_Tp>(s.val[3]);
}

template<typename _Tp> inline Scalar_<_Tp>::Scalar_(_Tp v0)
{ this->val[0] = v0; this->val[1] = this->val[2] = this->val[3] = 0; }

template<typename _Tp> inline Scalar_<_Tp> Scalar_<_Tp>::all(_Tp v0)
{ return Scalar_<_Tp>(v0, v0, v0, v0); }



template<typename _Tp> static inline bool operator == ( const Scalar_<_Tp>& a, const Scalar_<_Tp>& b )
{
    return a.val[0] == b.val[0] && a.val[1] == b.val[1] &&
        a.val[2] == b.val[2] && a.val[3] == b.val[3];
}

template<typename _Tp> static inline bool operator != ( const Scalar_<_Tp>& a, const Scalar_<_Tp>& b )
{
    return a.val[0] != b.val[0] || a.val[1] != b.val[1] ||
        a.val[2] != b.val[2] || a.val[3] != b.val[3];
}


template<typename _Tp> inline bool Scalar_<_Tp>::isReal() const
{
    return this->val[1] == 0 && this->val[2] == 0 && this->val[3] == 0;
}


//////////////////////////////// Range /////////////////////////////////

inline Range::Range() : start(0), end(0) {}
inline Range::Range(int _start, int _end) : start(_start), end(_end) {}
inline Range::Range(const CvSlice& slice) : start(slice.start_index), end(slice.end_index)
{
    if( start == 0 && end == CV_WHOLE_SEQ_END_INDEX )
        *this = Range::all();
}

inline int Range::size() const { return end - start; }
inline bool Range::empty() const { return start == end; }
inline Range Range::all() { return Range(INT_MIN, INT_MAX); }

static inline bool operator == (const Range& r1, const Range& r2)
{ return r1.start == r2.start && r1.end == r2.end; }

static inline bool operator != (const Range& r1, const Range& r2)
{ return !(r1 == r2); }

static inline bool operator !(const Range& r)
{ return r.start == r.end; }



// Multiply-with-Carry RNG
inline RNG::RNG() { state = 0xffffffff; }
inline RNG::RNG(uint64 _state) { state = _state ? _state : 0xffffffff; }
inline unsigned RNG::next()
{
    state = (uint64)(unsigned)state*CV_RNG_COEFF + (unsigned)(state >> 32);
    return (unsigned)state;
}

inline RNG::operator uchar() { return (uchar)next(); }
inline RNG::operator schar() { return (schar)next(); }
inline RNG::operator ushort() { return (ushort)next(); }
inline RNG::operator short() { return (short)next(); }
inline RNG::operator unsigned() { return next(); }

inline unsigned RNG::operator ()() {return next();}
inline RNG::operator int() { return (int)next(); }
// * (2^32-1)^-1
inline RNG::operator float() { return next()*2.3283064365386962890625e-10f; }
inline RNG::operator double()
{
    unsigned t = next();
    return (((uint64)t << 32) | next())*5.4210108624275221700372640043497e-20;
}


/////////////////////////////// AutoBuffer ////////////////////////////////////////

template<typename _Tp, size_t fixed_size> inline AutoBuffer<_Tp, fixed_size>::AutoBuffer()
{
    ptr = buf;
    size = fixed_size;
}

template<typename _Tp, size_t fixed_size> inline AutoBuffer<_Tp, fixed_size>::AutoBuffer(size_t _size)
{
    ptr = buf;
    size = fixed_size;
    allocate(_size);
}

template<typename _Tp, size_t fixed_size> inline AutoBuffer<_Tp, fixed_size>::~AutoBuffer()
{ deallocate(); }

template<typename _Tp, size_t fixed_size> inline void AutoBuffer<_Tp, fixed_size>::allocate(size_t _size)
{
    if(_size <= size)
        return;
    deallocate();
    size = _size;
    if(_size > fixed_size)
    {
        ptr = cv::allocate<_Tp>(_size);
    }
}

template<typename _Tp, size_t fixed_size> inline void AutoBuffer<_Tp, fixed_size>::deallocate()
{
    if( ptr != buf )
    {
        cv::deallocate<_Tp>(ptr, size);
        ptr = buf;
        size = fixed_size;
    }
}

template<typename _Tp, size_t fixed_size> inline AutoBuffer<_Tp, fixed_size>::operator _Tp* ()
{ return ptr; }

template<typename _Tp, size_t fixed_size> inline AutoBuffer<_Tp, fixed_size>::operator const _Tp* () const
{ return ptr; }

template<typename _Tp, size_t fixed_size> inline size_t AutoBuffer<_Tp, fixed_size>::getSize() const
{ return size; }


/////////////////////////////////// Ptr ////////////////////////////////////////

template<typename _Tp> inline Ptr<_Tp>::Ptr() : obj(0), refcount(0) {}
template<typename _Tp> inline Ptr<_Tp>::Ptr(_Tp* _obj) : obj(_obj)
{
    if(obj)
    {
        refcount = (int*)fastMalloc(sizeof(*refcount));
        *refcount = 1;
    }
    else
        refcount = 0;
}

template<typename _Tp> inline void Ptr<_Tp>::addref()
{ if( refcount ) CV_XADD(refcount, 1); }

template<typename _Tp> inline void Ptr<_Tp>::release()
{
    if( refcount && CV_XADD(refcount, -1) == 1 )
    {
        delete_obj();
        fastFree(refcount);
    }
    refcount = 0;
    obj = 0;
}

template<typename _Tp> inline void Ptr<_Tp>::delete_obj()
{
    if( obj ) delete obj;
}

template<typename _Tp> inline Ptr<_Tp>::~Ptr() { release(); }

template<typename _Tp> inline Ptr<_Tp>::Ptr(const Ptr<_Tp>& _ptr)
{
    obj = _ptr.obj;
    refcount = _ptr.refcount;
    addref();
}

template<typename _Tp> inline Ptr<_Tp>& Ptr<_Tp>::operator = (const Ptr<_Tp>& _ptr)
{
    if (this != &_ptr)
    {
      int* _refcount = _ptr.refcount;
      if( _refcount )
          CV_XADD(_refcount, 1);
      release();
      obj = _ptr.obj;
      refcount = _refcount;
    }
    return *this;
}

template<typename _Tp> inline _Tp* Ptr<_Tp>::operator -> () { return obj; }
template<typename _Tp> inline const _Tp* Ptr<_Tp>::operator -> () const { return obj; }

template<typename _Tp> inline Ptr<_Tp>::operator _Tp* () { return obj; }
template<typename _Tp> inline Ptr<_Tp>::operator const _Tp*() const { return obj; }

template<typename _Tp> inline bool Ptr<_Tp>::empty() const { return obj == 0; }


//// specializied implementations of Ptr::delete_obj() for classic OpenCV types

template<> CV_EXPORTS void Ptr<CvMat>::delete_obj();
template<> CV_EXPORTS void Ptr<IplImage>::delete_obj();
template<> CV_EXPORTS void Ptr<CvMemStorage>::delete_obj();

}


#endif // __cplusplus
#endif

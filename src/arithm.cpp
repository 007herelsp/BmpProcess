
#include "core.precomp.hpp"

namespace cv
{

struct NOP {};

template<typename T, class Op, class Op8>
void vBinOp8(const T* src1, size_t step1, const T* src2, size_t step2, T* dst, size_t step, Size sz)
{
    Op op;

    for( ; sz.height--; src1 += step1/sizeof(src1[0]),
                        src2 += step2/sizeof(src2[0]),
                        dst += step/sizeof(dst[0]) )
    {
        int x = 0;


#if CV_ENABLE_UNROLLED

        for( ; x <= sz.width - 4; x += 4 )
        {
            T v0 = op(src1[x], src2[x]);
            T v1 = op(src1[x+1], src2[x+1]);
            dst[x] = v0; dst[x+1] = v1;
            v0 = op(src1[x+2], src2[x+2]);
            v1 = op(src1[x+3], src2[x+3]);
            dst[x+2] = v0; dst[x+3] = v1;
        }
#endif
        for( ; x < sz.width; x++ )
            dst[x] = op(src1[x], src2[x]);
    }
}

template<typename T, class Op, class Op16>
void vBinOp16(const T* src1, size_t step1, const T* src2, size_t step2,
              T* dst, size_t step, Size sz)
{
    Op op;

    for( ; sz.height--; src1 += step1/sizeof(src1[0]),
        src2 += step2/sizeof(src2[0]),
        dst += step/sizeof(dst[0]) )
    {
        int x = 0;
        for( ; x <= sz.width - 4; x += 4 )
        {
            T v0 = op(src1[x], src2[x]);
            T v1 = op(src1[x+1], src2[x+1]);
            dst[x] = v0; dst[x+1] = v1;
            v0 = op(src1[x+2], src2[x+2]);
            v1 = op(src1[x+3], src2[x+3]);
            dst[x+2] = v0; dst[x+3] = v1;
        }

        for( ; x < sz.width; x++ )
            dst[x] = op(src1[x], src2[x]);
    }
}


template<class Op, class Op32>
void vBinOp32s(const int* src1, size_t step1, const int* src2, size_t step2,
               int* dst, size_t step, Size sz)
{
    Op op;

    for( ; sz.height--; src1 += step1/sizeof(src1[0]),
        src2 += step2/sizeof(src2[0]),
        dst += step/sizeof(dst[0]) )
    {
        int x = 0;

#if CV_ENABLE_UNROLLED
        for( ; x <= sz.width - 4; x += 4 )
        {
            int v0 = op(src1[x], src2[x]);
            int v1 = op(src1[x+1], src2[x+1]);
            dst[x] = v0; dst[x+1] = v1;
            v0 = op(src1[x+2], src2[x+2]);
            v1 = op(src1[x+3], src2[x+3]);
            dst[x+2] = v0; dst[x+3] = v1;
        }
#endif
        for( ; x < sz.width; x++ )
            dst[x] = op(src1[x], src2[x]);
    }
}


template<class Op, class Op32>
void vBinOp32f(const float* src1, size_t step1, const float* src2, size_t step2,
               float* dst, size_t step, Size sz)
{
    Op op;

    for( ; sz.height--; src1 += step1/sizeof(src1[0]),
        src2 += step2/sizeof(src2[0]),
        dst += step/sizeof(dst[0]) )
    {
        int x = 0;
#if CV_ENABLE_UNROLLED
        for( ; x <= sz.width - 4; x += 4 )
        {
            float v0 = op(src1[x], src2[x]);
            float v1 = op(src1[x+1], src2[x+1]);
            dst[x] = v0; dst[x+1] = v1;
            v0 = op(src1[x+2], src2[x+2]);
            v1 = op(src1[x+3], src2[x+3]);
            dst[x+2] = v0; dst[x+3] = v1;
        }
#endif
        for( ; x < sz.width; x++ )
            dst[x] = op(src1[x], src2[x]);
    }
}

template<class Op, class Op64>
void vBinOp64f(const double* src1, size_t step1, const double* src2, size_t step2,
               double* dst, size_t step, Size sz)
{
    Op op;

    for( ; sz.height--; src1 += step1/sizeof(src1[0]),
        src2 += step2/sizeof(src2[0]),
        dst += step/sizeof(dst[0]) )
    {
        int x = 0;
        for( ; x <= sz.width - 4; x += 4 )
        {
            double v0 = op(src1[x], src2[x]);
            double v1 = op(src1[x+1], src2[x+1]);
            dst[x] = v0; dst[x+1] = v1;
            v0 = op(src1[x+2], src2[x+2]);
            v1 = op(src1[x+3], src2[x+3]);
            dst[x+2] = v0; dst[x+3] = v1;
        }

        for( ; x < sz.width; x++ )
            dst[x] = op(src1[x], src2[x]);
    }
}




#define IF_SIMD(op) NOP

template<> inline uchar OpAdd<uchar>::operator ()(uchar a, uchar b) const
{ return CV_FAST_CAST_8U(a + b); }
template<> inline uchar OpSub<uchar>::operator ()(uchar a, uchar b) const
{ return CV_FAST_CAST_8U(a - b); }

template<typename T> struct OpAbsDiff
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator()(T a, T b) const { return (T)std::abs(a - b); }
};

template<> inline short OpAbsDiff<short>::operator ()(short a, short b) const
{ return saturate_cast<short>(std::abs(a - b)); }

template<> inline schar OpAbsDiff<schar>::operator ()(schar a, schar b) const
{ return saturate_cast<schar>(std::abs(a - b)); }

template<typename T, typename WT=T> struct OpAbsDiffS
{
    typedef T type1;
    typedef WT type2;
    typedef T rtype;
    T operator()(T a, WT b) const { return saturate_cast<T>(std::abs(a - b)); }
};

template<typename T> struct OpAnd
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator()( T a, T b ) const { return a & b; }
};

template<typename T> struct OpOr
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator()( T a, T b ) const { return a | b; }
};

template<typename T> struct OpXor
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator()( T a, T b ) const { return a ^ b; }
};

template<typename T> struct OpNot
{
    typedef T type1;
    typedef T type2;
    typedef T rtype;
    T operator()( T a, T ) const { return ~a; }
};


static inline void fixSteps(Size sz, size_t elemSize, size_t& step1, size_t& step2, size_t& step)
{
    if( sz.height == 1 )
        step1 = step2 = step = sz.width*elemSize;
}

static void add8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAdd_8u_C1RSfs(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz), 0),
           (vBinOp8<uchar, OpAdd<uchar>, IF_SIMD(_VAdd8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void add8s( const schar* src1, size_t step1,
                   const schar* src2, size_t step2,
                   schar* dst, size_t step, Size sz, void* )
{
    vBinOp8<schar, OpAdd<schar>, IF_SIMD(_VAdd8s)>(src1, step1, src2, step2, dst, step, sz);
}

static void add16u( const ushort* src1, size_t step1,
                    const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAdd_16u_C1RSfs(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz), 0),
            (vBinOp16<ushort, OpAdd<ushort>, IF_SIMD(_VAdd16u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void add16s( const short* src1, size_t step1,
                    const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAdd_16s_C1RSfs(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz), 0),
           (vBinOp16<short, OpAdd<short>, IF_SIMD(_VAdd16s)>(src1, step1, src2, step2, dst, step, sz)));
}

static void add32s( const int* src1, size_t step1,
                    const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* )
{
    vBinOp32s<OpAdd<int>, IF_SIMD(_VAdd32s)>(src1, step1, src2, step2, dst, step, sz);
}

static void add32f( const float* src1, size_t step1,
                    const float* src2, size_t step2,
                    float* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAdd_32f_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp32f<OpAdd<float>, IF_SIMD(_VAdd32f)>(src1, step1, src2, step2, dst, step, sz)));
}

static void add64f( const double* src1, size_t step1,
                    const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* )
{
    vBinOp64f<OpAdd<double>, IF_SIMD(_VAdd64f)>(src1, step1, src2, step2, dst, step, sz);
}

static void sub8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiSub_8u_C1RSfs(src2, (int)step2, src1, (int)step1, dst, (int)step, ippiSize(sz), 0),
           (vBinOp8<uchar, OpSub<uchar>, IF_SIMD(_VSub8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void sub8s( const schar* src1, size_t step1,
                   const schar* src2, size_t step2,
                   schar* dst, size_t step, Size sz, void* )
{
    vBinOp8<schar, OpSub<schar>, IF_SIMD(_VSub8s)>(src1, step1, src2, step2, dst, step, sz);
}

static void sub16u( const ushort* src1, size_t step1,
                    const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiSub_16u_C1RSfs(src2, (int)step2, src1, (int)step1, dst, (int)step, ippiSize(sz), 0),
           (vBinOp16<ushort, OpSub<ushort>, IF_SIMD(_VSub16u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void sub16s( const short* src1, size_t step1,
                    const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiSub_16s_C1RSfs(src2, (int)step2, src1, (int)step1, dst, (int)step, ippiSize(sz), 0),
           (vBinOp16<short, OpSub<short>, IF_SIMD(_VSub16s)>(src1, step1, src2, step2, dst, step, sz)));
}

static void sub32s( const int* src1, size_t step1,
                    const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* )
{
    vBinOp32s<OpSub<int>, IF_SIMD(_VSub32s)>(src1, step1, src2, step2, dst, step, sz);
}

static void sub32f( const float* src1, size_t step1,
                   const float* src2, size_t step2,
                   float* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiSub_32f_C1R(src2, (int)step2, src1, (int)step1, dst, (int)step, ippiSize(sz)),
           (vBinOp32f<OpSub<float>, IF_SIMD(_VSub32f)>(src1, step1, src2, step2, dst, step, sz)));
}

static void sub64f( const double* src1, size_t step1,
                    const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* )
{
    vBinOp64f<OpSub<double>, IF_SIMD(_VSub64f)>(src1, step1, src2, step2, dst, step, sz);
}

template<> inline uchar OpMin<uchar>::operator ()(uchar a, uchar b) const { return CV_MIN_8U(a, b); }
template<> inline uchar OpMax<uchar>::operator ()(uchar a, uchar b) const { return CV_MAX_8U(a, b); }

static void max8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
  vBinOp8<uchar, OpMax<uchar>, IF_SIMD(_VMax8u)>(src1, step1, src2, step2, dst, step, sz);
}

static void max8s( const schar* src1, size_t step1,
                   const schar* src2, size_t step2,
                   schar* dst, size_t step, Size sz, void* )
{
    vBinOp8<schar, OpMax<schar>, IF_SIMD(_VMax8s)>(src1, step1, src2, step2, dst, step, sz);
}

static void max16u( const ushort* src1, size_t step1,
                    const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* )
{

  vBinOp16<ushort, OpMax<ushort>, IF_SIMD(_VMax16u)>(src1, step1, src2, step2, dst, step, sz);


//    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
//           ippiMaxEvery_16u_C1R(src1, (int)step1, src2, (int)step2, dst, ippiSize(sz)),
//           (vBinOp16<ushort, OpMax<ushort>, IF_SIMD(_VMax16u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void max16s( const short* src1, size_t step1,
                    const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* )
{
    vBinOp16<short, OpMax<short>, IF_SIMD(_VMax16s)>(src1, step1, src2, step2, dst, step, sz);
}

static void max32s( const int* src1, size_t step1,
                    const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* )
{
    vBinOp32s<OpMax<int>, IF_SIMD(_VMax32s)>(src1, step1, src2, step2, dst, step, sz);
}

static void max32f( const float* src1, size_t step1,
                    const float* src2, size_t step2,
                    float* dst, size_t step, Size sz, void* )
{
  vBinOp32f<OpMax<float>, IF_SIMD(_VMax32f)>(src1, step1, src2, step2, dst, step, sz);
}

static void max64f( const double* src1, size_t step1,
                    const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* )
{
    vBinOp64f<OpMax<double>, IF_SIMD(_VMax64f)>(src1, step1, src2, step2, dst, step, sz);
}

static void min8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
  vBinOp8<uchar, OpMin<uchar>, IF_SIMD(_VMin8u)>(src1, step1, src2, step2, dst, step, sz);
}

static void min8s( const schar* src1, size_t step1,
                   const schar* src2, size_t step2,
                   schar* dst, size_t step, Size sz, void* )
{
    vBinOp8<schar, OpMin<schar>, IF_SIMD(_VMin8s)>(src1, step1, src2, step2, dst, step, sz);
}

static void min16u( const ushort* src1, size_t step1,
                    const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* )
{
  vBinOp16<ushort, OpMin<ushort>, IF_SIMD(_VMin16u)>(src1, step1, src2, step2, dst, step, sz);
}

static void min16s( const short* src1, size_t step1,
                    const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* )
{
    vBinOp16<short, OpMin<short>, IF_SIMD(_VMin16s)>(src1, step1, src2, step2, dst, step, sz);
}

static void min32s( const int* src1, size_t step1,
                    const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* )
{
    vBinOp32s<OpMin<int>, IF_SIMD(_VMin32s)>(src1, step1, src2, step2, dst, step, sz);
}

static void min32f( const float* src1, size_t step1,
                    const float* src2, size_t step2,
                    float* dst, size_t step, Size sz, void* )
{
  vBinOp32f<OpMin<float>, IF_SIMD(_VMin32f)>(src1, step1, src2, step2, dst, step, sz);
}

static void min64f( const double* src1, size_t step1,
                    const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* )
{
    vBinOp64f<OpMin<double>, IF_SIMD(_VMin64f)>(src1, step1, src2, step2, dst, step, sz);
}

static void absdiff8u( const uchar* src1, size_t step1,
                       const uchar* src2, size_t step2,
                       uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAbsDiff_8u_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp8<uchar, OpAbsDiff<uchar>, IF_SIMD(_VAbsDiff8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void absdiff8s( const schar* src1, size_t step1,
                       const schar* src2, size_t step2,
                       schar* dst, size_t step, Size sz, void* )
{
    vBinOp8<schar, OpAbsDiff<schar>, IF_SIMD(_VAbsDiff8s)>(src1, step1, src2, step2, dst, step, sz);
}

static void absdiff16u( const ushort* src1, size_t step1,
                        const ushort* src2, size_t step2,
                        ushort* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAbsDiff_16u_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp16<ushort, OpAbsDiff<ushort>, IF_SIMD(_VAbsDiff16u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void absdiff16s( const short* src1, size_t step1,
                        const short* src2, size_t step2,
                        short* dst, size_t step, Size sz, void* )
{
    vBinOp16<short, OpAbsDiff<short>, IF_SIMD(_VAbsDiff16s)>(src1, step1, src2, step2, dst, step, sz);
}

static void absdiff32s( const int* src1, size_t step1,
                        const int* src2, size_t step2,
                        int* dst, size_t step, Size sz, void* )
{
    vBinOp32s<OpAbsDiff<int>, IF_SIMD(_VAbsDiff32s)>(src1, step1, src2, step2, dst, step, sz);
}

static void absdiff32f( const float* src1, size_t step1,
                        const float* src2, size_t step2,
                        float* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAbsDiff_32f_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp32f<OpAbsDiff<float>, IF_SIMD(_VAbsDiff32f)>(src1, step1, src2, step2, dst, step, sz)));
}

static void absdiff64f( const double* src1, size_t step1,
                        const double* src2, size_t step2,
                        double* dst, size_t step, Size sz, void* )
{
    vBinOp64f<OpAbsDiff<double>, IF_SIMD(_VAbsDiff64f)>(src1, step1, src2, step2, dst, step, sz);
}


static void and8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiAnd_8u_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp8<uchar, OpAnd<uchar>, IF_SIMD(_VAnd8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void or8u( const uchar* src1, size_t step1,
                  const uchar* src2, size_t step2,
                  uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiOr_8u_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp8<uchar, OpOr<uchar>, IF_SIMD(_VOr8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void xor8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step);
           ippiXor_8u_C1R(src1, (int)step1, src2, (int)step2, dst, (int)step, ippiSize(sz)),
           (vBinOp8<uchar, OpXor<uchar>, IF_SIMD(_VXor8u)>(src1, step1, src2, step2, dst, step, sz)));
}

static void not8u( const uchar* src1, size_t step1,
                   const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* )
{
    IF_IPP(fixSteps(sz, sizeof(dst[0]), step1, step2, step); (void)src2;
    ippiNot_8u_C1R(src1, (int)step1, dst, (int)step, ippiSize(sz)),
           (vBinOp8<uchar, OpNot<uchar>, IF_SIMD(_VNot8u)>(src1, step1, src2, step2, dst, step, sz)));
}

/****************************************************************************************\
*                                   logical operations                                   *
\****************************************************************************************/

void convertAndUnrollScalar( const Mat& sc, int buftype, uchar* scbuf, size_t blocksize )
{
    int scn = (int)sc.total(), cn = CV_MAT_CN(buftype);
    size_t esz = CV_ELEM_SIZE(buftype);
    getConvertFunc(sc.depth(), buftype)(sc.data, 0, 0, 0, scbuf, 0, Size(std::min(cn, scn), 1), 0);
    // unroll the scalar
    if( scn < cn )
    {
        CV_Assert( scn == 1 );
        size_t esz1 = CV_ELEM_SIZE1(buftype);
        for( size_t i = esz1; i < esz; i++ )
            scbuf[i] = scbuf[i - esz1];
    }
    for( size_t i = esz; i < blocksize*esz; i++ )
        scbuf[i] = scbuf[i - esz];
}

static void binary_op(InputArray _src1, InputArray _src2, OutputArray _dst,
               InputArray _mask, const BinaryFunc* tab, bool bitwise)
{
    int kind1 = _src1.kind(), kind2 = _src2.kind();
    Mat src1 = _src1.getMat(), src2 = _src2.getMat();
    bool haveMask = !_mask.empty(), haveScalar = false;
    BinaryFunc func;
    int c;

    if( src1.dims <= 2 && src2.dims <= 2 && kind1 == kind2 &&
        src1.size() == src2.size() && src1.type() == src2.type() && !haveMask )
    {
        _dst.create(src1.size(), src1.type());
        Mat dst = _dst.getMat();
        if( bitwise )
        {
            func = *tab;
            c = (int)src1.elemSize();
        }
        else
        {
            func = tab[src1.depth()];
            c = src1.channels();
        }

        Size sz = getContinuousSize(src1, src2, dst);
        size_t len = sz.width*(size_t)c;
        if( len == (size_t)(int)len )
        {
            sz.width = (int)len;
            func(src1.data, src1.step, src2.data, src2.step, dst.data, dst.step, sz, 0);
            return;
        }
    }

    if( (kind1 == _InputArray::MATX) + (kind2 == _InputArray::MATX) == 1 ||
        src1.size != src2.size || src1.type() != src2.type() )
    {
        if( checkScalar(src1, src2.type(), kind1, kind2) )
            // src1 is a scalar; swap it with src2
            swap(src1, src2);
        else if( !checkScalar(src2, src1.type(), kind2, kind1) )
            CV_Error( CV_StsUnmatchedSizes,
                      "The operation is neither 'array op array' (where arrays have the same size and type), "
                      "nor 'array op scalar', nor 'scalar op array'" );
        haveScalar = true;
    }

    size_t esz = src1.elemSize();
    size_t blocksize0 = (BLOCK_SIZE + esz-1)/esz;
    int cn = src1.channels();
    BinaryFunc copymask = 0;
    Mat mask;
    bool reallocate = false;

    if( haveMask )
    {
        mask = _mask.getMat();
        CV_Assert( (mask.type() == CV_8UC1 || mask.type() == CV_8SC1) );
        CV_Assert( mask.size == src1.size );
        copymask = getCopyMaskFunc(esz);
        Mat tdst = _dst.getMat();
        reallocate = tdst.size != src1.size || tdst.type() != src1.type();
    }

    AutoBuffer<uchar> _buf;
    uchar *scbuf = 0, *maskbuf = 0;

    _dst.create(src1.dims, src1.size, src1.type());
    Mat dst = _dst.getMat();

    // if this is mask operation and dst has been reallocated,
    // we have to
    if( haveMask && reallocate )
        dst = Scalar::all(0);

    if( bitwise )
    {
        func = *tab;
        c = (int)esz;
    }
    else
    {
        func = tab[src1.depth()];
        c = cn;
    }

    if( !haveScalar )
    {
        const Mat* arrays[] = { &src1, &src2, &dst, &mask, 0 };
        uchar* ptrs[4];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size, blocksize = total;

        if( blocksize*c > INT_MAX )
            blocksize = INT_MAX/c;

        if( haveMask )
        {
            blocksize = std::min(blocksize, blocksize0);
            _buf.allocate(blocksize*esz);
            maskbuf = _buf;
        }

        for( size_t i = 0; i < it.nplanes; i++, ++it )
        {
            for( size_t j = 0; j < total; j += blocksize )
            {
                int bsz = (int)MIN(total - j, blocksize);

                func( ptrs[0], 0, ptrs[1], 0, haveMask ? maskbuf : ptrs[2], 0, Size(bsz*c, 1), 0 );
                if( haveMask )
                {
                    copymask( maskbuf, 0, ptrs[3], 0, ptrs[2], 0, Size(bsz, 1), &esz );
                    ptrs[3] += bsz;
                }

                bsz *= (int)esz;
                ptrs[0] += bsz; ptrs[1] += bsz; ptrs[2] += bsz;
            }
        }
    }
    else
    {
        const Mat* arrays[] = { &src1, &dst, &mask, 0 };
        uchar* ptrs[3];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size, blocksize = std::min(total, blocksize0);

        _buf.allocate(blocksize*(haveMask ? 2 : 1)*esz + 32);
        scbuf = _buf;
        maskbuf = alignPtr(scbuf + blocksize*esz, 16);

        convertAndUnrollScalar( src2, src1.type(), scbuf, blocksize);

        for( size_t i = 0; i < it.nplanes; i++, ++it )
        {
            for( size_t j = 0; j < total; j += blocksize )
            {
                int bsz = (int)MIN(total - j, blocksize);

                func( ptrs[0], 0, scbuf, 0, haveMask ? maskbuf : ptrs[1], 0, Size(bsz*c, 1), 0 );
                if( haveMask )
                {
                    copymask( maskbuf, 0, ptrs[2], 0, ptrs[1], 0, Size(bsz, 1), &esz );
                    ptrs[2] += bsz;
                }

                bsz *= (int)esz;
                ptrs[0] += bsz; ptrs[1] += bsz;
            }
        }
    }
}

static BinaryFunc* getMaxTab()
{
    static BinaryFunc maxTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(max8u), (BinaryFunc)GET_OPTIMIZED(max8s),
        (BinaryFunc)GET_OPTIMIZED(max16u), (BinaryFunc)GET_OPTIMIZED(max16s),
        (BinaryFunc)GET_OPTIMIZED(max32s),
        (BinaryFunc)GET_OPTIMIZED(max32f), (BinaryFunc)max64f,
        0
    };

    return maxTab;
}

static BinaryFunc* getMinTab()
{
    static BinaryFunc minTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(min8u), (BinaryFunc)GET_OPTIMIZED(min8s),
        (BinaryFunc)GET_OPTIMIZED(min16u), (BinaryFunc)GET_OPTIMIZED(min16s),
        (BinaryFunc)GET_OPTIMIZED(min32s),
        (BinaryFunc)GET_OPTIMIZED(min32f), (BinaryFunc)min64f,
        0
    };

    return minTab;
}

}

void cv::bitwise_and(InputArray a, InputArray b, OutputArray c, InputArray mask)
{
    BinaryFunc f = (BinaryFunc)GET_OPTIMIZED(and8u);
    binary_op(a, b, c, mask, &f, true);
}

void cv::bitwise_or(InputArray a, InputArray b, OutputArray c, InputArray mask)
{
    BinaryFunc f = (BinaryFunc)GET_OPTIMIZED(or8u);
    binary_op(a, b, c, mask, &f, true);
}

void cv::bitwise_xor(InputArray a, InputArray b, OutputArray c, InputArray mask)
{
    BinaryFunc f = (BinaryFunc)GET_OPTIMIZED(xor8u);
    binary_op(a, b, c, mask, &f, true);
}

void cv::bitwise_not(InputArray a, OutputArray c, InputArray mask)
{
    BinaryFunc f = (BinaryFunc)GET_OPTIMIZED(not8u);
    binary_op(a, a, c, mask, &f, true);
}

void cv::max( InputArray src1, InputArray src2, OutputArray dst )
{
    binary_op(src1, src2, dst, noArray(), getMaxTab(), false );
}

void cv::min( InputArray src1, InputArray src2, OutputArray dst )
{
    binary_op(src1, src2, dst, noArray(), getMinTab(), false );
}

void cv::max(const Mat& src1, const Mat& src2, Mat& dst)
{
    OutputArray _dst(dst);
    binary_op(src1, src2, _dst, noArray(), getMaxTab(), false );
}

void cv::min(const Mat& src1, const Mat& src2, Mat& dst)
{
    OutputArray _dst(dst);
    binary_op(src1, src2, _dst, noArray(), getMinTab(), false );
}

void cv::max(const Mat& src1, double src2, Mat& dst)
{
    OutputArray _dst(dst);
    binary_op(src1, src2, _dst, noArray(), getMaxTab(), false );
}

void cv::min(const Mat& src1, double src2, Mat& dst)
{
    OutputArray _dst(dst);
    binary_op(src1, src2, _dst, noArray(), getMinTab(), false );
}

/****************************************************************************************\
*                                      add/subtract                                      *
\****************************************************************************************/

namespace cv
{

static int actualScalarDepth(const double* data, int len)
{
    int i = 0, minval = INT_MAX, maxval = INT_MIN;
    for(; i < len; ++i)
    {
        int ival = cvRound(data[i]);
        if( ival != data[i] )
            break;
        minval = MIN(minval, ival);
        maxval = MAX(maxval, ival);
    }
    return i < len ? CV_64F :
        minval >= 0 && maxval <= (int)UCHAR_MAX ? CV_8U :
        minval >= (int)SCHAR_MIN && maxval <= (int)SCHAR_MAX ? CV_8S :
        minval >= 0 && maxval <= (int)USHRT_MAX ? CV_16U :
        minval >= (int)SHRT_MIN && maxval <= (int)SHRT_MAX ? CV_16S :
        CV_32S;
}

static void arithm_op(InputArray _src1, InputArray _src2, OutputArray _dst,
               InputArray _mask, int dtype, BinaryFunc* tab, bool muldiv=false, void* usrdata=0)
{
    int kind1 = _src1.kind(), kind2 = _src2.kind();
    Mat src1 = _src1.getMat(), src2 = _src2.getMat();
    bool haveMask = !_mask.empty();
    bool reallocate = false;

    bool src1Scalar = checkScalar(src1, src2.type(), kind1, kind2);
    bool src2Scalar = checkScalar(src2, src1.type(), kind2, kind1);

    if( (kind1 == kind2 || src1.channels() == 1) && src1.dims <= 2 && src2.dims <= 2 &&
        src1.size() == src2.size() && src1.type() == src2.type() &&
        !haveMask && ((!_dst.fixedType() && (dtype < 0 || CV_MAT_DEPTH(dtype) == src1.depth())) ||
                       (_dst.fixedType() && _dst.type() == _src1.type())) &&
        ((src1Scalar && src2Scalar) || (!src1Scalar && !src2Scalar)) )
    {
        _dst.create(src1.size(), src1.type());
        Mat dst = _dst.getMat();
        Size sz = getContinuousSize(src1, src2, dst, src1.channels());
        tab[src1.depth()](src1.data, src1.step, src2.data, src2.step, dst.data, dst.step, sz, usrdata);
        return;
    }

    bool haveScalar = false, swapped12 = false;
    int depth2 = src2.depth();
    if( src1.size != src2.size || src1.channels() != src2.channels() ||
        (kind1 == _InputArray::MATX && (src1.size() == Size(1,4) || src1.size() == Size(1,1))) ||
        (kind2 == _InputArray::MATX && (src2.size() == Size(1,4) || src2.size() == Size(1,1))) )
    {
        if( checkScalar(src1, src2.type(), kind1, kind2) )
        {
            // src1 is a scalar; swap it with src2
            swap(src1, src2);
            swapped12 = true;
        }
        else if( !checkScalar(src2, src1.type(), kind2, kind1) )
            CV_Error( CV_StsUnmatchedSizes,
                     "The operation is neither 'array op array' (where arrays have the same size and the same number of channels), "
                     "nor 'array op scalar', nor 'scalar op array'" );
        haveScalar = true;
        CV_Assert(src2.type() == CV_64F && (src2.rows == 4 || src2.rows == 1));

        if (!muldiv)
        {
            depth2 = actualScalarDepth(src2.ptr<double>(), src1.channels());
            if( depth2 == CV_64F && (src1.depth() < CV_32S || src1.depth() == CV_32F) )
                depth2 = CV_32F;
        }
        else
            depth2 = CV_64F;
    }

    int cn = src1.channels(), depth1 = src1.depth(), wtype;
    BinaryFunc cvtsrc1 = 0, cvtsrc2 = 0, cvtdst = 0;

    if( dtype < 0 )
    {
        if( _dst.fixedType() )
            dtype = _dst.type();
        else
        {
            if( !haveScalar && src1.type() != src2.type() )
                CV_Error(CV_StsBadArg,
                     "When the input arrays in add/subtract/multiply/divide functions have different types, "
                     "the output array type must be explicitly specified");
            dtype = src1.type();
        }
    }
    dtype = CV_MAT_DEPTH(dtype);

    if( depth1 == depth2 && dtype == depth1 )
        wtype = dtype;
    else if( !muldiv )
    {
        wtype = depth1 <= CV_8S && depth2 <= CV_8S ? CV_16S :
                depth1 <= CV_32S && depth2 <= CV_32S ? CV_32S : std::max(depth1, depth2);
        wtype = std::max(wtype, dtype);

        // when the result of addition should be converted to an integer type,
        // and just one of the input arrays is floating-point, it makes sense to convert that input to integer type before the operation,
        // instead of converting the other input to floating-point and then converting the operation result back to integers.
        if( dtype < CV_32F && (depth1 < CV_32F || depth2 < CV_32F) )
            wtype = CV_32S;
    }
    else
    {
        wtype = std::max(depth1, std::max(depth2, CV_32F));
        wtype = std::max(wtype, dtype);
    }

    cvtsrc1 = depth1 == wtype ? 0 : getConvertFunc(depth1, wtype);
    cvtsrc2 = depth2 == depth1 ? cvtsrc1 : depth2 == wtype ? 0 : getConvertFunc(depth2, wtype);
    cvtdst = dtype == wtype ? 0 : getConvertFunc(wtype, dtype);

    dtype = CV_MAKETYPE(dtype, cn);
    wtype = CV_MAKETYPE(wtype, cn);

    size_t esz1 = src1.elemSize(), esz2 = src2.elemSize();
    size_t dsz = CV_ELEM_SIZE(dtype), wsz = CV_ELEM_SIZE(wtype);
    size_t blocksize0 = (size_t)(BLOCK_SIZE + wsz-1)/wsz;
    BinaryFunc copymask = 0;
    Mat mask;

    if( haveMask )
    {
        mask = _mask.getMat();
        CV_Assert( (mask.type() == CV_8UC1 || mask.type() == CV_8SC1) );
        CV_Assert( mask.size == src1.size );
        copymask = getCopyMaskFunc(dsz);
        Mat tdst = _dst.getMat();
        reallocate = tdst.size != src1.size || tdst.type() != dtype;
    }

    AutoBuffer<uchar> _buf;
    uchar *buf, *maskbuf = 0, *buf1 = 0, *buf2 = 0, *wbuf = 0;
    size_t bufesz = (cvtsrc1 ? wsz : 0) + (cvtsrc2 || haveScalar ? wsz : 0) + (cvtdst ? wsz : 0) + (haveMask ? dsz : 0);

    _dst.create(src1.dims, src1.size, dtype);
    Mat dst = _dst.getMat();

    if( haveMask && reallocate )
        dst = Scalar::all(0);

    BinaryFunc func = tab[CV_MAT_DEPTH(wtype)];

    if( !haveScalar )
    {
        const Mat* arrays[] = { &src1, &src2, &dst, &mask, 0 };
        uchar* ptrs[4];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size, blocksize = total;

        if( haveMask || cvtsrc1 || cvtsrc2 || cvtdst )
            blocksize = std::min(blocksize, blocksize0);

        _buf.allocate(bufesz*blocksize + 64);
        buf = _buf;
        if( cvtsrc1 )
            buf1 = buf, buf = alignPtr(buf + blocksize*wsz, 16);
        if( cvtsrc2 )
            buf2 = buf, buf = alignPtr(buf + blocksize*wsz, 16);
        wbuf = maskbuf = buf;
        if( cvtdst )
            buf = alignPtr(buf + blocksize*wsz, 16);
        if( haveMask )
            maskbuf = buf;

        for( size_t i = 0; i < it.nplanes; i++, ++it )
        {
            for( size_t j = 0; j < total; j += blocksize )
            {
                int bsz = (int)MIN(total - j, blocksize);
                Size bszn(bsz*cn, 1);
                const uchar *sptr1 = ptrs[0], *sptr2 = ptrs[1];
                uchar* dptr = ptrs[2];
                if( cvtsrc1 )
                {
                    cvtsrc1( sptr1, 0, 0, 0, buf1, 0, bszn, 0 );
                    sptr1 = buf1;
                }
                if( ptrs[0] == ptrs[1] )
                    sptr2 = sptr1;
                else if( cvtsrc2 )
                {
                    cvtsrc2( sptr2, 0, 0, 0, buf2, 0, bszn, 0 );
                    sptr2 = buf2;
                }

                if( !haveMask && !cvtdst )
                    func( sptr1, 0, sptr2, 0, dptr, 0, bszn, usrdata );
                else
                {
                    func( sptr1, 0, sptr2, 0, wbuf, 0, bszn, usrdata );
                    if( !haveMask )
                        cvtdst( wbuf, 0, 0, 0, dptr, 0, bszn, 0 );
                    else if( !cvtdst )
                    {
                        copymask( wbuf, 0, ptrs[3], 0, dptr, 0, Size(bsz, 1), &dsz );
                        ptrs[3] += bsz;
                    }
                    else
                    {
                        cvtdst( wbuf, 0, 0, 0, maskbuf, 0, bszn, 0 );
                        copymask( maskbuf, 0, ptrs[3], 0, dptr, 0, Size(bsz, 1), &dsz );
                        ptrs[3] += bsz;
                    }
                }
                ptrs[0] += bsz*esz1; ptrs[1] += bsz*esz2; ptrs[2] += bsz*dsz;
            }
        }
    }
    else
    {
        const Mat* arrays[] = { &src1, &dst, &mask, 0 };
        uchar* ptrs[3];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size, blocksize = std::min(total, blocksize0);

        _buf.allocate(bufesz*blocksize + 64);
        buf = _buf;
        if( cvtsrc1 )
            buf1 = buf, buf = alignPtr(buf + blocksize*wsz, 16);
        buf2 = buf; buf = alignPtr(buf + blocksize*wsz, 16);
        wbuf = maskbuf = buf;
        if( cvtdst )
            buf = alignPtr(buf + blocksize*wsz, 16);
        if( haveMask )
            maskbuf = buf;

        convertAndUnrollScalar( src2, wtype, buf2, blocksize);

        for( size_t i = 0; i < it.nplanes; i++, ++it )
        {
            for( size_t j = 0; j < total; j += blocksize )
            {
                int bsz = (int)MIN(total - j, blocksize);
                Size bszn(bsz*cn, 1);
                const uchar *sptr1 = ptrs[0];
                const uchar* sptr2 = buf2;
                uchar* dptr = ptrs[1];

                if( cvtsrc1 )
                {
                    cvtsrc1( sptr1, 0, 0, 0, buf1, 0, bszn, 0 );
                    sptr1 = buf1;
                }

                if( swapped12 )
                    std::swap(sptr1, sptr2);

                if( !haveMask && !cvtdst )
                    func( sptr1, 0, sptr2, 0, dptr, 0, bszn, usrdata );
                else
                {
                    func( sptr1, 0, sptr2, 0, wbuf, 0, bszn, usrdata );
                    if( !haveMask )
                        cvtdst( wbuf, 0, 0, 0, dptr, 0, bszn, 0 );
                    else if( !cvtdst )
                    {
                        copymask( wbuf, 0, ptrs[2], 0, dptr, 0, Size(bsz, 1), &dsz );
                        ptrs[2] += bsz;
                    }
                    else
                    {
                        cvtdst( wbuf, 0, 0, 0, maskbuf, 0, bszn, 0 );
                        copymask( maskbuf, 0, ptrs[2], 0, dptr, 0, Size(bsz, 1), &dsz );
                        ptrs[2] += bsz;
                    }
                }
                ptrs[0] += bsz*esz1; ptrs[1] += bsz*dsz;
            }
        }
    }
}

static BinaryFunc* getAddTab()
{
    static BinaryFunc addTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(add8u), (BinaryFunc)GET_OPTIMIZED(add8s),
        (BinaryFunc)GET_OPTIMIZED(add16u), (BinaryFunc)GET_OPTIMIZED(add16s),
        (BinaryFunc)GET_OPTIMIZED(add32s),
        (BinaryFunc)GET_OPTIMIZED(add32f), (BinaryFunc)add64f,
        0
    };

    return addTab;
}

static BinaryFunc* getSubTab()
{
    static BinaryFunc subTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(sub8u), (BinaryFunc)GET_OPTIMIZED(sub8s),
        (BinaryFunc)GET_OPTIMIZED(sub16u), (BinaryFunc)GET_OPTIMIZED(sub16s),
        (BinaryFunc)GET_OPTIMIZED(sub32s),
        (BinaryFunc)GET_OPTIMIZED(sub32f), (BinaryFunc)sub64f,
        0
    };

    return subTab;
}

static BinaryFunc* getAbsDiffTab()
{
    static BinaryFunc absDiffTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(absdiff8u), (BinaryFunc)GET_OPTIMIZED(absdiff8s),
        (BinaryFunc)GET_OPTIMIZED(absdiff16u), (BinaryFunc)GET_OPTIMIZED(absdiff16s),
        (BinaryFunc)GET_OPTIMIZED(absdiff32s),
        (BinaryFunc)GET_OPTIMIZED(absdiff32f), (BinaryFunc)absdiff64f,
        0
    };

    return absDiffTab;
}

}

void cv::add( InputArray src1, InputArray src2, OutputArray dst,
          InputArray mask, int dtype )
{
    arithm_op(src1, src2, dst, mask, dtype, getAddTab() );
}

void cv::subtract( InputArray _src1, InputArray _src2, OutputArray _dst,
               InputArray mask, int dtype )
{

    arithm_op(_src1, _src2, _dst, mask, dtype, getSubTab() );
}

void cv::absdiff( InputArray src1, InputArray src2, OutputArray dst )
{
    arithm_op(src1, src2, dst, noArray(), -1, getAbsDiffTab());
}

/****************************************************************************************\
*                                    multiply/divide                                     *
\****************************************************************************************/

namespace cv
{

template<typename T, typename WT> static void
mul_( const T* src1, size_t step1, const T* src2, size_t step2,
      T* dst, size_t step, Size size, WT scale )
{
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    step /= sizeof(dst[0]);

    if( scale == (WT)1. )
    {
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int i=0;
            #if CV_ENABLE_UNROLLED
            for(; i <= size.width - 4; i += 4 )
            {
                T t0;
                T t1;
                t0 = saturate_cast<T>(src1[i  ] * src2[i  ]);
                t1 = saturate_cast<T>(src1[i+1] * src2[i+1]);
                dst[i  ] = t0;
                dst[i+1] = t1;

                t0 = saturate_cast<T>(src1[i+2] * src2[i+2]);
                t1 = saturate_cast<T>(src1[i+3] * src2[i+3]);
                dst[i+2] = t0;
                dst[i+3] = t1;
            }
            #endif
            for( ; i < size.width; i++ )
                dst[i] = saturate_cast<T>(src1[i] * src2[i]);
        }
    }
    else
    {
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int i = 0;
            #if CV_ENABLE_UNROLLED
            for(; i <= size.width - 4; i += 4 )
            {
                T t0 = saturate_cast<T>(scale*(WT)src1[i]*src2[i]);
                T t1 = saturate_cast<T>(scale*(WT)src1[i+1]*src2[i+1]);
                dst[i] = t0; dst[i+1] = t1;

                t0 = saturate_cast<T>(scale*(WT)src1[i+2]*src2[i+2]);
                t1 = saturate_cast<T>(scale*(WT)src1[i+3]*src2[i+3]);
                dst[i+2] = t0; dst[i+3] = t1;
            }
            #endif
            for( ; i < size.width; i++ )
                dst[i] = saturate_cast<T>(scale*(WT)src1[i]*src2[i]);
        }
    }
}

template<typename T> static void
div_( const T* src1, size_t step1, const T* src2, size_t step2,
      T* dst, size_t step, Size size, double scale )
{
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    step /= sizeof(dst[0]);

    for( ; size.height--; src1 += step1, src2 += step2, dst += step )
    {
        int i = 0;
        #if CV_ENABLE_UNROLLED
        for( ; i <= size.width - 4; i += 4 )
        {
            if( src2[i] != 0 && src2[i+1] != 0 && src2[i+2] != 0 && src2[i+3] != 0 )
            {
                double a = (double)src2[i] * src2[i+1];
                double b = (double)src2[i+2] * src2[i+3];
                double d = scale/(a * b);
                b *= d;
                a *= d;

                T z0 = saturate_cast<T>(src2[i+1] * ((double)src1[i] * b));
                T z1 = saturate_cast<T>(src2[i] * ((double)src1[i+1] * b));
                T z2 = saturate_cast<T>(src2[i+3] * ((double)src1[i+2] * a));
                T z3 = saturate_cast<T>(src2[i+2] * ((double)src1[i+3] * a));

                dst[i] = z0; dst[i+1] = z1;
                dst[i+2] = z2; dst[i+3] = z3;
            }
            else
            {
                T z0 = src2[i] != 0 ? saturate_cast<T>(src1[i]*scale/src2[i]) : 0;
                T z1 = src2[i+1] != 0 ? saturate_cast<T>(src1[i+1]*scale/src2[i+1]) : 0;
                T z2 = src2[i+2] != 0 ? saturate_cast<T>(src1[i+2]*scale/src2[i+2]) : 0;
                T z3 = src2[i+3] != 0 ? saturate_cast<T>(src1[i+3]*scale/src2[i+3]) : 0;

                dst[i] = z0; dst[i+1] = z1;
                dst[i+2] = z2; dst[i+3] = z3;
            }
        }
        #endif
        for( ; i < size.width; i++ )
            dst[i] = src2[i] != 0 ? saturate_cast<T>(src1[i]*scale/src2[i]) : 0;
    }
}

template<typename T> static void
recip_( const T*, size_t, const T* src2, size_t step2,
        T* dst, size_t step, Size size, double scale )
{
    step2 /= sizeof(src2[0]);
    step /= sizeof(dst[0]);

    for( ; size.height--; src2 += step2, dst += step )
    {
        int i = 0;
        #if CV_ENABLE_UNROLLED
        for( ; i <= size.width - 4; i += 4 )
        {
            if( src2[i] != 0 && src2[i+1] != 0 && src2[i+2] != 0 && src2[i+3] != 0 )
            {
                double a = (double)src2[i] * src2[i+1];
                double b = (double)src2[i+2] * src2[i+3];
                double d = scale/(a * b);
                b *= d;
                a *= d;

                T z0 = saturate_cast<T>(src2[i+1] * b);
                T z1 = saturate_cast<T>(src2[i] * b);
                T z2 = saturate_cast<T>(src2[i+3] * a);
                T z3 = saturate_cast<T>(src2[i+2] * a);

                dst[i] = z0; dst[i+1] = z1;
                dst[i+2] = z2; dst[i+3] = z3;
            }
            else
            {
                T z0 = src2[i] != 0 ? saturate_cast<T>(scale/src2[i]) : 0;
                T z1 = src2[i+1] != 0 ? saturate_cast<T>(scale/src2[i+1]) : 0;
                T z2 = src2[i+2] != 0 ? saturate_cast<T>(scale/src2[i+2]) : 0;
                T z3 = src2[i+3] != 0 ? saturate_cast<T>(scale/src2[i+3]) : 0;

                dst[i] = z0; dst[i+1] = z1;
                dst[i+2] = z2; dst[i+3] = z3;
            }
        }
        #endif
        for( ; i < size.width; i++ )
            dst[i] = src2[i] != 0 ? saturate_cast<T>(scale/src2[i]) : 0;
    }
}


static void mul8u( const uchar* src1, size_t step1, const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, (float)*(const double*)scale);
}

static void mul8s( const schar* src1, size_t step1, const schar* src2, size_t step2,
                   schar* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, (float)*(const double*)scale);
}

static void mul16u( const ushort* src1, size_t step1, const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, (float)*(const double*)scale);
}

static void mul16s( const short* src1, size_t step1, const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, (float)*(const double*)scale);
}

static void mul32s( const int* src1, size_t step1, const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void mul32f( const float* src1, size_t step1, const float* src2, size_t step2,
                    float* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, (float)*(const double*)scale);
}

static void mul64f( const double* src1, size_t step1, const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* scale)
{
    mul_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div8u( const uchar* src1, size_t step1, const uchar* src2, size_t step2,
                   uchar* dst, size_t step, Size sz, void* scale)
{
    if( src1 )
        div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
    else
        recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div8s( const schar* src1, size_t step1, const schar* src2, size_t step2,
                  schar* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div16u( const ushort* src1, size_t step1, const ushort* src2, size_t step2,
                    ushort* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div16s( const short* src1, size_t step1, const short* src2, size_t step2,
                    short* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div32s( const int* src1, size_t step1, const int* src2, size_t step2,
                    int* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div32f( const float* src1, size_t step1, const float* src2, size_t step2,
                    float* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void div64f( const double* src1, size_t step1, const double* src2, size_t step2,
                    double* dst, size_t step, Size sz, void* scale)
{
    div_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip8u( const uchar* src1, size_t step1, const uchar* src2, size_t step2,
                  uchar* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip8s( const schar* src1, size_t step1, const schar* src2, size_t step2,
                  schar* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip16u( const ushort* src1, size_t step1, const ushort* src2, size_t step2,
                   ushort* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip16s( const short* src1, size_t step1, const short* src2, size_t step2,
                   short* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip32s( const int* src1, size_t step1, const int* src2, size_t step2,
                   int* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip32f( const float* src1, size_t step1, const float* src2, size_t step2,
                   float* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}

static void recip64f( const double* src1, size_t step1, const double* src2, size_t step2,
                   double* dst, size_t step, Size sz, void* scale)
{
    recip_(src1, step1, src2, step2, dst, step, sz, *(const double*)scale);
}


static BinaryFunc* getMulTab()
{
    static BinaryFunc mulTab[] =
    {
        (BinaryFunc)mul8u, (BinaryFunc)mul8s, (BinaryFunc)mul16u,
        (BinaryFunc)mul16s, (BinaryFunc)mul32s, (BinaryFunc)mul32f,
        (BinaryFunc)mul64f, 0
    };

    return mulTab;
}

static BinaryFunc* getDivTab()
{
    static BinaryFunc divTab[] =
    {
        (BinaryFunc)div8u, (BinaryFunc)div8s, (BinaryFunc)div16u,
        (BinaryFunc)div16s, (BinaryFunc)div32s, (BinaryFunc)div32f,
        (BinaryFunc)div64f, 0
    };

    return divTab;
}

static BinaryFunc* getRecipTab()
{
    static BinaryFunc recipTab[] =
    {
        (BinaryFunc)recip8u, (BinaryFunc)recip8s, (BinaryFunc)recip16u,
        (BinaryFunc)recip16s, (BinaryFunc)recip32s, (BinaryFunc)recip32f,
        (BinaryFunc)recip64f, 0
    };

    return recipTab;
}

}

void cv::multiply(InputArray src1, InputArray src2,
                  OutputArray dst, double scale, int dtype)
{
    arithm_op(src1, src2, dst, noArray(), dtype, getMulTab(), true, &scale);
}

void cv::divide(InputArray src1, InputArray src2,
                OutputArray dst, double scale, int dtype)
{
    arithm_op(src1, src2, dst, noArray(), dtype, getDivTab(), true, &scale);
}

void cv::divide(double scale, InputArray src2,
                OutputArray dst, int dtype)
{
    arithm_op(src2, src2, dst, noArray(), dtype, getRecipTab(), true, &scale);
}

/****************************************************************************************\
*                                      addWeighted                                       *
\****************************************************************************************/

namespace cv
{

template<typename T, typename WT> static void
addWeighted_( const T* src1, size_t step1, const T* src2, size_t step2,
              T* dst, size_t step, Size size, void* _scalars )
{
    const double* scalars = (const double*)_scalars;
    WT alpha = (WT)scalars[0], beta = (WT)scalars[1], gamma = (WT)scalars[2];
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    step /= sizeof(dst[0]);

    for( ; size.height--; src1 += step1, src2 += step2, dst += step )
    {
        int x = 0;
        #if CV_ENABLE_UNROLLED
        for( ; x <= size.width - 4; x += 4 )
        {
            T t0 = saturate_cast<T>(src1[x]*alpha + src2[x]*beta + gamma);
            T t1 = saturate_cast<T>(src1[x+1]*alpha + src2[x+1]*beta + gamma);
            dst[x] = t0; dst[x+1] = t1;

            t0 = saturate_cast<T>(src1[x+2]*alpha + src2[x+2]*beta + gamma);
            t1 = saturate_cast<T>(src1[x+3]*alpha + src2[x+3]*beta + gamma);
            dst[x+2] = t0; dst[x+3] = t1;
        }
        #endif
        for( ; x < size.width; x++ )
            dst[x] = saturate_cast<T>(src1[x]*alpha + src2[x]*beta + gamma);
    }
}


static void
addWeighted8u( const uchar* src1, size_t step1,
               const uchar* src2, size_t step2,
               uchar* dst, size_t step, Size size,
               void* _scalars )
{
    const double* scalars = (const double*)_scalars;
    float alpha = (float)scalars[0], beta = (float)scalars[1], gamma = (float)scalars[2];

    for( ; size.height--; src1 += step1, src2 += step2, dst += step )
    {
        int x = 0;

        #if CV_ENABLE_UNROLLED
        for( ; x <= size.width - 4; x += 4 )
        {
            float t0, t1;
            t0 = CV_8TO32F(src1[x])*alpha + CV_8TO32F(src2[x])*beta + gamma;
            t1 = CV_8TO32F(src1[x+1])*alpha + CV_8TO32F(src2[x+1])*beta + gamma;

            dst[x] = saturate_cast<uchar>(t0);
            dst[x+1] = saturate_cast<uchar>(t1);

            t0 = CV_8TO32F(src1[x+2])*alpha + CV_8TO32F(src2[x+2])*beta + gamma;
            t1 = CV_8TO32F(src1[x+3])*alpha + CV_8TO32F(src2[x+3])*beta + gamma;

            dst[x+2] = saturate_cast<uchar>(t0);
            dst[x+3] = saturate_cast<uchar>(t1);
        }
        #endif

        for( ; x < size.width; x++ )
        {
            float t0 = CV_8TO32F(src1[x])*alpha + CV_8TO32F(src2[x])*beta + gamma;
            dst[x] = saturate_cast<uchar>(t0);
        }
    }
}

static void addWeighted8s( const schar* src1, size_t step1, const schar* src2, size_t step2,
                           schar* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<schar, float>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static void addWeighted16u( const ushort* src1, size_t step1, const ushort* src2, size_t step2,
                            ushort* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<ushort, float>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static void addWeighted16s( const short* src1, size_t step1, const short* src2, size_t step2,
                            short* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<short, float>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static void addWeighted32s( const int* src1, size_t step1, const int* src2, size_t step2,
                            int* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<int, double>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static void addWeighted32f( const float* src1, size_t step1, const float* src2, size_t step2,
                            float* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<float, double>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static void addWeighted64f( const double* src1, size_t step1, const double* src2, size_t step2,
                            double* dst, size_t step, Size sz, void* scalars )
{
    addWeighted_<double, double>(src1, step1, src2, step2, dst, step, sz, scalars);
}

static BinaryFunc* getAddWeightedTab()
{
    static BinaryFunc addWeightedTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(addWeighted8u), (BinaryFunc)GET_OPTIMIZED(addWeighted8s), (BinaryFunc)GET_OPTIMIZED(addWeighted16u),
        (BinaryFunc)GET_OPTIMIZED(addWeighted16s), (BinaryFunc)GET_OPTIMIZED(addWeighted32s), (BinaryFunc)addWeighted32f,
        (BinaryFunc)addWeighted64f, 0
    };

    return addWeightedTab;
}

}

void cv::addWeighted( InputArray src1, double alpha, InputArray src2,
                      double beta, double gamma, OutputArray dst, int dtype )
{
    double scalars[] = {alpha, beta, gamma};
    arithm_op(src1, src2, dst, noArray(), dtype, getAddWeightedTab(), true, scalars);
}


/****************************************************************************************\
*                                          compare                                       *
\****************************************************************************************/

namespace cv
{

template<typename T> static void
cmp_(const T* src1, size_t step1, const T* src2, size_t step2,
     uchar* dst, size_t step, Size size, int code)
{
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    if( code == CMP_GE || code == CMP_LT )
    {
        std::swap(src1, src2);
        std::swap(step1, step2);
        code = code == CMP_GE ? CMP_LE : CMP_GT;
    }

    if( code == CMP_GT || code == CMP_LE )
    {
        int m = code == CMP_GT ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x = 0;
            #if CV_ENABLE_UNROLLED
            for( ; x <= size.width - 4; x += 4 )
            {
                int t0, t1;
                t0 = -(src1[x] > src2[x]) ^ m;
                t1 = -(src1[x+1] > src2[x+1]) ^ m;
                dst[x] = (uchar)t0; dst[x+1] = (uchar)t1;
                t0 = -(src1[x+2] > src2[x+2]) ^ m;
                t1 = -(src1[x+3] > src2[x+3]) ^ m;
                dst[x+2] = (uchar)t0; dst[x+3] = (uchar)t1;
            }
            #endif
            for( ; x < size.width; x++ )
                dst[x] = (uchar)(-(src1[x] > src2[x]) ^ m);
               }
    }
    else if( code == CMP_EQ || code == CMP_NE )
    {
        int m = code == CMP_EQ ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x = 0;
            #if CV_ENABLE_UNROLLED
            for( ; x <= size.width - 4; x += 4 )
            {
                int t0, t1;
                t0 = -(src1[x] == src2[x]) ^ m;
                t1 = -(src1[x+1] == src2[x+1]) ^ m;
                dst[x] = (uchar)t0; dst[x+1] = (uchar)t1;
                t0 = -(src1[x+2] == src2[x+2]) ^ m;
                t1 = -(src1[x+3] == src2[x+3]) ^ m;
                dst[x+2] = (uchar)t0; dst[x+3] = (uchar)t1;
            }
            #endif
            for( ; x < size.width; x++ )
                dst[x] = (uchar)(-(src1[x] == src2[x]) ^ m);
        }
    }
}


static void cmp8u(const uchar* src1, size_t step1, const uchar* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{

  //vz optimized  cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
    int code = *(int*)_cmpop;
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    if( code == CMP_GE || code == CMP_LT )
    {
        std::swap(src1, src2);
        std::swap(step1, step2);
        code = code == CMP_GE ? CMP_LE : CMP_GT;
    }

    if( code == CMP_GT || code == CMP_LE )
    {
        int m = code == CMP_GT ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x =0;
            #if CV_SSE2
            if( USE_SSE2 ){
                __m128i m128 = code == CMP_GT ? _mm_setzero_si128() : _mm_set1_epi8 (-1);
                __m128i c128 = _mm_set1_epi8 (-128);
                for( ; x <= size.width - 16; x += 16 )
                {
                    __m128i r00 = _mm_loadu_si128((const __m128i*)(src1 + x));
                    __m128i r10 = _mm_loadu_si128((const __m128i*)(src2 + x));
                    // no simd for 8u comparison, that's why we need the trick
                    r00 = _mm_sub_epi8(r00,c128);
                    r10 = _mm_sub_epi8(r10,c128);

                    r00 =_mm_xor_si128(_mm_cmpgt_epi8(r00, r10), m128);
                    _mm_storeu_si128((__m128i*)(dst + x),r00);

                }
            }
           #endif

            for( ; x < size.width; x++ ){
                dst[x] = (uchar)(-(src1[x] > src2[x]) ^ m);
            }
        }
    }
    else if( code == CMP_EQ || code == CMP_NE )
    {
        int m = code == CMP_EQ ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x = 0;
 
           for( ; x < size.width; x++ )
                dst[x] = (uchar)(-(src1[x] == src2[x]) ^ m);
        }
    }
}

static void cmp8s(const schar* src1, size_t step1, const schar* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{
    cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
}

static void cmp16u(const ushort* src1, size_t step1, const ushort* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{
    cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
}

static void cmp16s(const short* src1, size_t step1, const short* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{

   //vz optimized cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);

    int code = *(int*)_cmpop;
    step1 /= sizeof(src1[0]);
    step2 /= sizeof(src2[0]);
    if( code == CMP_GE || code == CMP_LT )
    {
        std::swap(src1, src2);
        std::swap(step1, step2);
        code = code == CMP_GE ? CMP_LE : CMP_GT;
    }

    if( code == CMP_GT || code == CMP_LE )
    {
        int m = code == CMP_GT ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x =0;
            for( ; x < size.width; x++ ){
                 dst[x] = (uchar)(-(src1[x] > src2[x]) ^ m);
            }
        }
    }
    else if( code == CMP_EQ || code == CMP_NE )
    {
        int m = code == CMP_EQ ? 0 : 255;
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int x = 0;
           for( ; x < size.width; x++ )
                dst[x] = (uchar)(-(src1[x] == src2[x]) ^ m);
        }
    }
}

static void cmp32s(const int* src1, size_t step1, const int* src2, size_t step2,
                   uchar* dst, size_t step, Size size, void* _cmpop)
{
    cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
}

static void cmp32f(const float* src1, size_t step1, const float* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{
    cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
}

static void cmp64f(const double* src1, size_t step1, const double* src2, size_t step2,
                  uchar* dst, size_t step, Size size, void* _cmpop)
{
    cmp_(src1, step1, src2, step2, dst, step, size, *(int*)_cmpop);
}

static BinaryFunc getCmpFunc(int depth)
{
    static BinaryFunc cmpTab[] =
    {
        (BinaryFunc)GET_OPTIMIZED(cmp8u), (BinaryFunc)GET_OPTIMIZED(cmp8s),
        (BinaryFunc)GET_OPTIMIZED(cmp16u), (BinaryFunc)GET_OPTIMIZED(cmp16s),
        (BinaryFunc)GET_OPTIMIZED(cmp32s),
        (BinaryFunc)GET_OPTIMIZED(cmp32f), (BinaryFunc)cmp64f,
        0
    };

    return cmpTab[depth];
}

static double getMinVal(int depth)
{
    static const double tab[] = {0, -128, 0, -32768, INT_MIN, -FLT_MAX, -DBL_MAX, 0};
    return tab[depth];
}

static double getMaxVal(int depth)
{
    static const double tab[] = {255, 127, 65535, 32767, INT_MAX, FLT_MAX, DBL_MAX, 0};
    return tab[depth];
}

}

void cv::compare(InputArray _src1, InputArray _src2, OutputArray _dst, int op)
{
    CV_Assert( op == CMP_LT || op == CMP_LE || op == CMP_EQ ||
               op == CMP_NE || op == CMP_GE || op == CMP_GT );

    int kind1 = _src1.kind(), kind2 = _src2.kind();
    Mat src1 = _src1.getMat(), src2 = _src2.getMat();

    if( kind1 == kind2 && src1.dims <= 2 && src2.dims <= 2 && src1.size() == src2.size() && src1.type() == src2.type() )
    {
        int cn = src1.channels();
        _dst.create(src1.size(), CV_8UC(cn));
        Mat dst = _dst.getMat();
        Size sz = getContinuousSize(src1, src2, dst, src1.channels());
        getCmpFunc(src1.depth())(src1.data, src1.step, src2.data, src2.step, dst.data, dst.step, sz, &op);
        return;
    }

    bool haveScalar = false;

    if( (kind1 == _InputArray::MATX) + (kind2 == _InputArray::MATX) == 1 ||
        src1.size != src2.size || src1.type() != src2.type() )
    {
        if( checkScalar(src1, src2.type(), kind1, kind2) )
        {
            // src1 is a scalar; swap it with src2
            swap(src1, src2);
            op = op == CMP_LT ? CMP_GT : op == CMP_LE ? CMP_GE :
                op == CMP_GE ? CMP_LE : op == CMP_GT ? CMP_LT : op;
        }
        else if( !checkScalar(src2, src1.type(), kind2, kind1) )
            CV_Error( CV_StsUnmatchedSizes,
                     "The operation is neither 'array op array' (where arrays have the same size and the same type), "
                     "nor 'array op scalar', nor 'scalar op array'" );
        haveScalar = true;
    }


    int cn = src1.channels(), depth1 = src1.depth(), depth2 = src2.depth();

    _dst.create(src1.dims, src1.size, CV_8UC(cn));
    src1 = src1.reshape(1); src2 = src2.reshape(1);
    Mat dst = _dst.getMat().reshape(1);

    size_t esz = src1.elemSize();
    size_t blocksize0 = (size_t)(BLOCK_SIZE + esz-1)/esz;
    BinaryFunc func = getCmpFunc(depth1);

    if( !haveScalar )
    {
        const Mat* arrays[] = { &src1, &src2, &dst, 0 };
        uchar* ptrs[3];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size;

        for( size_t i = 0; i < it.nplanes; i++, ++it )
            func( ptrs[0], 0, ptrs[1], 0, ptrs[2], 0, Size((int)total, 1), &op );
    }
    else
    {
        const Mat* arrays[] = { &src1, &dst, 0 };
        uchar* ptrs[2];

        NAryMatIterator it(arrays, ptrs);
        size_t total = it.size, blocksize = std::min(total, blocksize0);

        AutoBuffer<uchar> _buf(blocksize*esz);
        uchar *buf = _buf;

        if( depth1 > CV_32S )
            convertAndUnrollScalar( src2, depth1, buf, blocksize );
        else
        {
            double fval=0;
            getConvertFunc(depth2, CV_64F)(src2.data, 0, 0, 0, (uchar*)&fval, 0, Size(1,1), 0);
            if( fval < getMinVal(depth1) )
            {
                dst = Scalar::all(op == CMP_GT || op == CMP_GE || op == CMP_NE ? 255 : 0);
                return;
            }

            if( fval > getMaxVal(depth1) )
            {
                dst = Scalar::all(op == CMP_LT || op == CMP_LE || op == CMP_NE ? 255 : 0);
                return;
            }

            int ival = cvRound(fval);
            if( fval != ival )
            {
                if( op == CMP_LT || op == CMP_GE )
                    ival = cvCeil(fval);
                else if( op == CMP_LE || op == CMP_GT )
                    ival = cvFloor(fval);
                else
                {
                    dst = Scalar::all(op == CMP_NE ? 255 : 0);
                    return;
                }
            }
            convertAndUnrollScalar(Mat(1, 1, CV_32S, &ival), depth1, buf, blocksize);
        }

        for( size_t i = 0; i < it.nplanes; i++, ++it )
        {
            for( size_t j = 0; j < total; j += blocksize )
            {
                int bsz = (int)MIN(total - j, blocksize);
                func( ptrs[0], 0, buf, 0, ptrs[1], 0, Size(bsz, 1), &op);
                ptrs[0] += bsz*esz;
                ptrs[1] += bsz;
            }
        }
    }
}





/* End of file. */

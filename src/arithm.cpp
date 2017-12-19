
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

}


void cv::subtract( InputArray _src1, InputArray _src2, OutputArray _dst,
               InputArray mask, int dtype )
{

    arithm_op(_src1, _src2, _dst, mask, dtype, getSubTab() );
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
            for( ; i < size.width; i++ )
                dst[i] = saturate_cast<T>(src1[i] * src2[i]);
        }
    }
    else
    {
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )
        {
            int i = 0;
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




/* End of file. */

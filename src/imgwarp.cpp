 

#include "imgproc.precomp.hpp"
#include <iostream>
#include <vector>


namespace cv
{


/************** interpolation formulas and tables ***************/

const int INTER_RESIZE_COEF_BITS=11;
const int INTER_RESIZE_COEF_SCALE=1 << INTER_RESIZE_COEF_BITS;

const int INTER_REMAP_COEF_BITS=15;
const int INTER_REMAP_COEF_SCALE=1 << INTER_REMAP_COEF_BITS;

static uchar NNDeltaTab_i[INTER_TAB_SIZE2][2];

static float BilinearTab_f[INTER_TAB_SIZE2][2][2];
static short BilinearTab_i[INTER_TAB_SIZE2][2][2];

static float BicubicTab_f[INTER_TAB_SIZE2][4][4];
static short BicubicTab_i[INTER_TAB_SIZE2][4][4];

static float Lanczos4Tab_f[INTER_TAB_SIZE2][8][8];
static short Lanczos4Tab_i[INTER_TAB_SIZE2][8][8];

static inline void interpolateLinear( float x, float* coeffs )
{
    coeffs[0] = 1.f - x;
    coeffs[1] = x;
}

static inline void interpolateCubic( float x, float* coeffs )
{
    const float A = -0.75f;

    coeffs[0] = ((A*(x + 1) - 5*A)*(x + 1) + 8*A)*(x + 1) - 4*A;
    coeffs[1] = ((A + 2)*x - (A + 3))*x*x + 1;
    coeffs[2] = ((A + 2)*(1 - x) - (A + 3))*(1 - x)*(1 - x) + 1;
    coeffs[3] = 1.f - coeffs[0] - coeffs[1] - coeffs[2];
}

static inline void interpolateLanczos4( float x, float* coeffs )
{
    static const double s45 = 0.70710678118654752440084436210485;
    static const double cs[][2]=
    {{1, 0}, {-s45, -s45}, {0, 1}, {s45, -s45}, {-1, 0}, {s45, s45}, {0, -1}, {-s45, s45}};

    if( x < FLT_EPSILON )
    {
        for( int i = 0; i < 8; i++ )
            coeffs[i] = 0;
        coeffs[3] = 1;
        return;
    }

    float sum = 0;
    double y0=-(x+3)*CV_PI*0.25, s0 = sin(y0), c0=cos(y0);
    for(int i = 0; i < 8; i++ )
    {
        double y = -(x+3-i)*CV_PI*0.25;
        coeffs[i] = (float)((cs[i][0]*s0 + cs[i][1]*c0)/(y*y));
        sum += coeffs[i];
    }

    sum = 1.f/sum;
    for(int i = 0; i < 8; i++ )
        coeffs[i] *= sum;
}

static void initInterTab1D(int method, float* tab, int tabsz)
{
    float scale = 1.f/tabsz;
    if( method == INTER_LINEAR )
    {
        for( int i = 0; i < tabsz; i++, tab += 2 )
            interpolateLinear( i*scale, tab );
    }
    else if( method == INTER_CUBIC )
    {
        for( int i = 0; i < tabsz; i++, tab += 4 )
            interpolateCubic( i*scale, tab );
    }
    else if( method == INTER_LANCZOS4 )
    {
        for( int i = 0; i < tabsz; i++, tab += 8 )
            interpolateLanczos4( i*scale, tab );
    }
    else
        CV_Error( CV_StsBadArg, "Unknown interpolation method" );
}


static const void* initInterTab2D( int method, bool fixpt )
{
    static bool inittab[INTER_MAX+1] = {false};
    float* tab = 0;
    short* itab = 0;
    int ksize = 0;
    if( method == INTER_LINEAR )
        tab = BilinearTab_f[0][0], itab = BilinearTab_i[0][0], ksize=2;
    else if( method == INTER_CUBIC )
        tab = BicubicTab_f[0][0], itab = BicubicTab_i[0][0], ksize=4;
    else if( method == INTER_LANCZOS4 )
        tab = Lanczos4Tab_f[0][0], itab = Lanczos4Tab_i[0][0], ksize=8;
    else
        CV_Error( CV_StsBadArg, "Unknown/unsupported interpolation type" );

    if( !inittab[method] )
    {
        AutoBuffer<float> _tab(8*INTER_TAB_SIZE);
        int i, j, k1, k2;
        initInterTab1D(method, _tab, INTER_TAB_SIZE);
        for( i = 0; i < INTER_TAB_SIZE; i++ )
            for( j = 0; j < INTER_TAB_SIZE; j++, tab += ksize*ksize, itab += ksize*ksize )
            {
                int isum = 0;
                NNDeltaTab_i[i*INTER_TAB_SIZE+j][0] = j < INTER_TAB_SIZE/2;
                NNDeltaTab_i[i*INTER_TAB_SIZE+j][1] = i < INTER_TAB_SIZE/2;

                for( k1 = 0; k1 < ksize; k1++ )
                {
                    float vy = _tab[i*ksize + k1];
                    for( k2 = 0; k2 < ksize; k2++ )
                    {
                        float v = vy*_tab[j*ksize + k2];
                        tab[k1*ksize + k2] = v;
                        isum += itab[k1*ksize + k2] = saturate_cast<short>(v*INTER_REMAP_COEF_SCALE);
                    }
                }

                if( isum != INTER_REMAP_COEF_SCALE )
                {
                    int diff = isum - INTER_REMAP_COEF_SCALE;
                    int ksize2 = ksize/2, Mk1=ksize2, Mk2=ksize2, mk1=ksize2, mk2=ksize2;
                    for( k1 = ksize2; k1 < ksize2+2; k1++ )
                        for( k2 = ksize2; k2 < ksize2+2; k2++ )
                        {
                            if( itab[k1*ksize+k2] < itab[mk1*ksize+mk2] )
                                mk1 = k1, mk2 = k2;
                            else if( itab[k1*ksize+k2] > itab[Mk1*ksize+Mk2] )
                                Mk1 = k1, Mk2 = k2;
                        }
                    if( diff < 0 )
                        itab[Mk1*ksize + Mk2] = (short)(itab[Mk1*ksize + Mk2] - diff);
                    else
                        itab[mk1*ksize + mk2] = (short)(itab[mk1*ksize + mk2] - diff);
                }
            }
        tab -= INTER_TAB_SIZE2*ksize*ksize;
        itab -= INTER_TAB_SIZE2*ksize*ksize;
        inittab[method] = true;
    }
    return fixpt ? (const void*)itab : (const void*)tab;
}

#ifndef __MINGW32__
static bool initAllInterTab2D()
{
    return  initInterTab2D( INTER_LINEAR, false ) &&
            initInterTab2D( INTER_LINEAR, true ) &&
            initInterTab2D( INTER_CUBIC, false ) &&
            initInterTab2D( INTER_CUBIC, true ) &&
            initInterTab2D( INTER_LANCZOS4, false ) &&
            initInterTab2D( INTER_LANCZOS4, true );
}

static volatile bool doInitAllInterTab2D = initAllInterTab2D();
#endif

template<typename ST, typename DT> struct Cast
{
    typedef ST type1;
    typedef DT rtype;

    DT operator()(ST val) const { return saturate_cast<DT>(val); }
};

template<typename ST, typename DT, int bits> struct FixedPtCast
{
    typedef ST type1;
    typedef DT rtype;
    enum { SHIFT = bits, DELTA = 1 << (bits-1) };

    DT operator()(ST val) const { return saturate_cast<DT>((val + DELTA)>>SHIFT); }
};

/****************************************************************************************\
*                                         Resize                                         *
\****************************************************************************************/

class resizeNNInvoker :
    public ParallelLoopBody
{
public:
    resizeNNInvoker(const Mat& _src, Mat &_dst, int *_x_ofs, int _pix_size4, double _ify) :
        ParallelLoopBody(), src(_src), dst(_dst), x_ofs(_x_ofs), pix_size4(_pix_size4),
        ify(_ify)
    {
    }

    virtual void operator() (const Range& range) const
    {
        Size ssize = src.size(), dsize = dst.size();
        int y, x, pix_size = (int)src.elemSize();

        for( y = range.start; y < range.end; y++ )
        {
            uchar* D = dst.data + dst.step*y;
            int sy = std::min(cvFloor(y*ify), ssize.height-1);
            const uchar* S = src.data + src.step*sy;

            switch( pix_size )
            {
            case 1:
                for( x = 0; x <= dsize.width - 2; x += 2 )
                {
                    uchar t0 = S[x_ofs[x]];
                    uchar t1 = S[x_ofs[x+1]];
                    D[x] = t0;
                    D[x+1] = t1;
                }

                for( ; x < dsize.width; x++ )
                    D[x] = S[x_ofs[x]];
                break;
            case 2:
                for( x = 0; x < dsize.width; x++ )
                    *(ushort*)(D + x*2) = *(ushort*)(S + x_ofs[x]);
                break;
            case 3:
                for( x = 0; x < dsize.width; x++, D += 3 )
                {
                    const uchar* _tS = S + x_ofs[x];
                    D[0] = _tS[0]; D[1] = _tS[1]; D[2] = _tS[2];
                }
                break;
            case 4:
                for( x = 0; x < dsize.width; x++ )
                    *(int*)(D + x*4) = *(int*)(S + x_ofs[x]);
                break;
            case 6:
                for( x = 0; x < dsize.width; x++, D += 6 )
                {
                    const ushort* _tS = (const ushort*)(S + x_ofs[x]);
                    ushort* _tD = (ushort*)D;
                    _tD[0] = _tS[0]; _tD[1] = _tS[1]; _tD[2] = _tS[2];
                }
                break;
            case 8:
                for( x = 0; x < dsize.width; x++, D += 8 )
                {
                    const int* _tS = (const int*)(S + x_ofs[x]);
                    int* _tD = (int*)D;
                    _tD[0] = _tS[0]; _tD[1] = _tS[1];
                }
                break;
            case 12:
                for( x = 0; x < dsize.width; x++, D += 12 )
                {
                    const int* _tS = (const int*)(S + x_ofs[x]);
                    int* _tD = (int*)D;
                    _tD[0] = _tS[0]; _tD[1] = _tS[1]; _tD[2] = _tS[2];
                }
                break;
            default:
                for( x = 0; x < dsize.width; x++, D += pix_size )
                {
                    const int* _tS = (const int*)(S + x_ofs[x]);
                    int* _tD = (int*)D;
                    for( int k = 0; k < pix_size4; k++ )
                        _tD[k] = _tS[k];
                }
            }
        }
    }

private:
    const Mat src;
    Mat dst;
    int* x_ofs, pix_size4;
    double ify;

    resizeNNInvoker(const resizeNNInvoker&);
    resizeNNInvoker& operator=(const resizeNNInvoker&);
};

static void
resizeNN( const Mat& src, Mat& dst, double fx, double fy )
{
    Size ssize = src.size(), dsize = dst.size();
    AutoBuffer<int> _x_ofs(dsize.width);
    int* x_ofs = _x_ofs;
    int pix_size = (int)src.elemSize();
    int pix_size4 = (int)(pix_size / sizeof(int));
    double ifx = 1./fx, ify = 1./fy;
    int x;

    for( x = 0; x < dsize.width; x++ )
    {
        int sx = cvFloor(x*ifx);
        x_ofs[x] = std::min(sx, ssize.width-1)*pix_size;
    }

    Range range(0, dsize.height);
    resizeNNInvoker invoker(src, dst, x_ofs, pix_size4, ify);
    parallel_for_(range, invoker, dst.total()/(double)(1<<16));
}


struct VResizeNoVec
{
    int operator()(const uchar**, uchar*, const uchar*, int ) const { return 0; }
};

struct HResizeNoVec
{
    int operator()(const uchar**, uchar**, int, const int*,
        const uchar*, int, int, int, int, int) const { return 0; }
};

typedef VResizeNoVec VResizeLinearVec_32s8u;
typedef VResizeNoVec VResizeLinearVec_32f16u;
typedef VResizeNoVec VResizeLinearVec_32f16s;
typedef VResizeNoVec VResizeLinearVec_32f;

typedef VResizeNoVec VResizeCubicVec_32s8u;
typedef VResizeNoVec VResizeCubicVec_32f16u;
typedef VResizeNoVec VResizeCubicVec_32f16s;
typedef VResizeNoVec VResizeCubicVec_32f;

typedef HResizeNoVec HResizeLinearVec_8u32s;
typedef HResizeNoVec HResizeLinearVec_16u32f;
typedef HResizeNoVec HResizeLinearVec_16s32f;
typedef HResizeNoVec HResizeLinearVec_32f;
typedef HResizeNoVec HResizeLinearVec_64f;

template<typename T, typename WT, typename AT, int ONE, class VecOp>
struct HResizeLinear
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const T** src, WT** dst, int count,
                    const int* xofs, const AT* alpha,
                    int swidth, int dwidth, int cn, int xmin, int xmax ) const
    {
        int dx, k;
        VecOp vecOp;

        int dx0 = vecOp((const uchar**)src, (uchar**)dst, count,
            xofs, (const uchar*)alpha, swidth, dwidth, cn, xmin, xmax );

        for( k = 0; k <= count - 2; k++ )
        {
            const T *S0 = src[k], *S1 = src[k+1];
            WT *D0 = dst[k], *D1 = dst[k+1];
            for( dx = dx0; dx < xmax; dx++ )
            {
                int sx = xofs[dx];
                WT a0 = alpha[dx*2], a1 = alpha[dx*2+1];
                WT t0 = S0[sx]*a0 + S0[sx + cn]*a1;
                WT t1 = S1[sx]*a0 + S1[sx + cn]*a1;
                D0[dx] = t0; D1[dx] = t1;
            }

            for( ; dx < dwidth; dx++ )
            {
                int sx = xofs[dx];
                D0[dx] = WT(S0[sx]*ONE); D1[dx] = WT(S1[sx]*ONE);
            }
        }

        for( ; k < count; k++ )
        {
            const T *S = src[k];
            WT *D = dst[k];
            for( dx = 0; dx < xmax; dx++ )
            {
                int sx = xofs[dx];
                D[dx] = S[sx]*alpha[dx*2] + S[sx+cn]*alpha[dx*2+1];
            }

            for( ; dx < dwidth; dx++ )
                D[dx] = WT(S[xofs[dx]]*ONE);
        }
    }
};


template<typename T, typename WT, typename AT, class CastOp, class VecOp>
struct VResizeLinear
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const WT** src, T* dst, const AT* beta, int width ) const
    {
        WT b0 = beta[0], b1 = beta[1];
        const WT *S0 = src[0], *S1 = src[1];
        CastOp castOp;
        VecOp vecOp;

        int x = vecOp((const uchar**)src, (uchar*)dst, (const uchar*)beta, width);
        #if CV_ENABLE_UNROLLED
        for( ; x <= width - 4; x += 4 )
        {
            WT t0, t1;
            t0 = S0[x]*b0 + S1[x]*b1;
            t1 = S0[x+1]*b0 + S1[x+1]*b1;
            dst[x] = castOp(t0); dst[x+1] = castOp(t1);
            t0 = S0[x+2]*b0 + S1[x+2]*b1;
            t1 = S0[x+3]*b0 + S1[x+3]*b1;
            dst[x+2] = castOp(t0); dst[x+3] = castOp(t1);
        }
        #endif
        for( ; x < width; x++ )
            dst[x] = castOp(S0[x]*b0 + S1[x]*b1);
    }
};

template<>
struct VResizeLinear<uchar, int, short, FixedPtCast<int, uchar, INTER_RESIZE_COEF_BITS*2>, VResizeLinearVec_32s8u>
{
    typedef uchar value_type;
    typedef int buf_type;
    typedef short alpha_type;

    void operator()(const buf_type** src, value_type* dst, const alpha_type* beta, int width ) const
    {
        alpha_type b0 = beta[0], b1 = beta[1];
        const buf_type *S0 = src[0], *S1 = src[1];
        VResizeLinearVec_32s8u vecOp;

        int x = vecOp((const uchar**)src, (uchar*)dst, (const uchar*)beta, width);
        #if CV_ENABLE_UNROLLED
        for( ; x <= width - 4; x += 4 )
        {
            dst[x+0] = uchar(( ((b0 * (S0[x+0] >> 4)) >> 16) + ((b1 * (S1[x+0] >> 4)) >> 16) + 2)>>2);
            dst[x+1] = uchar(( ((b0 * (S0[x+1] >> 4)) >> 16) + ((b1 * (S1[x+1] >> 4)) >> 16) + 2)>>2);
            dst[x+2] = uchar(( ((b0 * (S0[x+2] >> 4)) >> 16) + ((b1 * (S1[x+2] >> 4)) >> 16) + 2)>>2);
            dst[x+3] = uchar(( ((b0 * (S0[x+3] >> 4)) >> 16) + ((b1 * (S1[x+3] >> 4)) >> 16) + 2)>>2);
        }
        #endif
        for( ; x < width; x++ )
            dst[x] = uchar(( ((b0 * (S0[x] >> 4)) >> 16) + ((b1 * (S1[x] >> 4)) >> 16) + 2)>>2);
    }
};


template<typename T, typename WT, typename AT>
struct HResizeCubic
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const T** src, WT** dst, int count,
                    const int* xofs, const AT* alpha,
                    int swidth, int dwidth, int cn, int xmin, int xmax ) const
    {
        for( int k = 0; k < count; k++ )
        {
            const T *S = src[k];
            WT *D = dst[k];
            int dx = 0, limit = xmin;
            for(;;)
            {
                for( ; dx < limit; dx++, alpha += 4 )
                {
                    int j, sx = xofs[dx] - cn;
                    WT v = 0;
                    for( j = 0; j < 4; j++ )
                    {
                        int sxj = sx + j*cn;
                        if( (unsigned)sxj >= (unsigned)swidth )
                        {
                            while( sxj < 0 )
                                sxj += cn;
                            while( sxj >= swidth )
                                sxj -= cn;
                        }
                        v += S[sxj]*alpha[j];
                    }
                    D[dx] = v;
                }
                if( limit == dwidth )
                    break;
                for( ; dx < xmax; dx++, alpha += 4 )
                {
                    int sx = xofs[dx];
                    D[dx] = S[sx-cn]*alpha[0] + S[sx]*alpha[1] +
                        S[sx+cn]*alpha[2] + S[sx+cn*2]*alpha[3];
                }
                limit = dwidth;
            }
            alpha -= dwidth*4;
        }
    }
};


template<typename T, typename WT, typename AT, class CastOp, class VecOp>
struct VResizeCubic
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const WT** src, T* dst, const AT* beta, int width ) const
    {
        WT b0 = beta[0], b1 = beta[1], b2 = beta[2], b3 = beta[3];
        const WT *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3];
        CastOp castOp;
        VecOp vecOp;

        int x = vecOp((const uchar**)src, (uchar*)dst, (const uchar*)beta, width);
        for( ; x < width; x++ )
            dst[x] = castOp(S0[x]*b0 + S1[x]*b1 + S2[x]*b2 + S3[x]*b3);
    }
};


template<typename T, typename WT, typename AT>
struct HResizeLanczos4
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const T** src, WT** dst, int count,
                    const int* xofs, const AT* alpha,
                    int swidth, int dwidth, int cn, int xmin, int xmax ) const
    {
        for( int k = 0; k < count; k++ )
        {
            const T *S = src[k];
            WT *D = dst[k];
            int dx = 0, limit = xmin;
            for(;;)
            {
                for( ; dx < limit; dx++, alpha += 8 )
                {
                    int j, sx = xofs[dx] - cn*3;
                    WT v = 0;
                    for( j = 0; j < 8; j++ )
                    {
                        int sxj = sx + j*cn;
                        if( (unsigned)sxj >= (unsigned)swidth )
                        {
                            while( sxj < 0 )
                                sxj += cn;
                            while( sxj >= swidth )
                                sxj -= cn;
                        }
                        v += S[sxj]*alpha[j];
                    }
                    D[dx] = v;
                }
                if( limit == dwidth )
                    break;
                for( ; dx < xmax; dx++, alpha += 8 )
                {
                    int sx = xofs[dx];
                    D[dx] = S[sx-cn*3]*alpha[0] + S[sx-cn*2]*alpha[1] +
                        S[sx-cn]*alpha[2] + S[sx]*alpha[3] +
                        S[sx+cn]*alpha[4] + S[sx+cn*2]*alpha[5] +
                        S[sx+cn*3]*alpha[6] + S[sx+cn*4]*alpha[7];
                }
                limit = dwidth;
            }
            alpha -= dwidth*8;
        }
    }
};


template<typename T, typename WT, typename AT, class CastOp, class VecOp>
struct VResizeLanczos4
{
    typedef T value_type;
    typedef WT buf_type;
    typedef AT alpha_type;

    void operator()(const WT** src, T* dst, const AT* beta, int width ) const
    {
        CastOp castOp;
        VecOp vecOp;
        int k, x = vecOp((const uchar**)src, (uchar*)dst, (const uchar*)beta, width);
        #if CV_ENABLE_UNROLLED
        for( ; x <= width - 4; x += 4 )
        {
            WT b = beta[0];
            const WT* S = src[0];
            WT s0 = S[x]*b, s1 = S[x+1]*b, s2 = S[x+2]*b, s3 = S[x+3]*b;

            for( k = 1; k < 8; k++ )
            {
                b = beta[k]; S = src[k];
                s0 += S[x]*b; s1 += S[x+1]*b;
                s2 += S[x+2]*b; s3 += S[x+3]*b;
            }

            dst[x] = castOp(s0); dst[x+1] = castOp(s1);
            dst[x+2] = castOp(s2); dst[x+3] = castOp(s3);
        }
        #endif
        for( ; x < width; x++ )
        {
            dst[x] = castOp(src[0][x]*beta[0] + src[1][x]*beta[1] +
                src[2][x]*beta[2] + src[3][x]*beta[3] + src[4][x]*beta[4] +
                src[5][x]*beta[5] + src[6][x]*beta[6] + src[7][x]*beta[7]);
        }
    }
};


static inline int clip(int x, int a, int b)
{
    return x >= a ? (x < b ? x : b-1) : a;
}

static const int MAX_ESIZE=16;

template <typename HResize, typename VResize>
class resizeGeneric_Invoker :
    public ParallelLoopBody
{
public:
    typedef typename HResize::value_type T;
    typedef typename HResize::buf_type WT;
    typedef typename HResize::alpha_type AT;

    resizeGeneric_Invoker(const Mat& _src, Mat &_dst, const int *_xofs, const int *_yofs,
        const AT* _alpha, const AT* __beta, const Size& _ssize, const Size &_dsize,
        int _ksize, int _xmin, int _xmax) :
        ParallelLoopBody(), src(_src), dst(_dst), xofs(_xofs), yofs(_yofs),
        alpha(_alpha), _beta(__beta), ssize(_ssize), dsize(_dsize),
        ksize(_ksize), xmin(_xmin), xmax(_xmax)
    {
        CV_Assert(ksize <= MAX_ESIZE);
    }

    virtual void operator() (const Range& range) const
    {
        int dy, cn = src.channels();
        HResize hresize;
        VResize vresize;

        int bufstep = (int)alignSize(dsize.width, 16);
        AutoBuffer<WT> _buffer(bufstep*ksize);
        const T* srows[MAX_ESIZE]={0};
        WT* rows[MAX_ESIZE]={0};
        int prev_sy[MAX_ESIZE];

        for(int k = 0; k < ksize; k++ )
        {
            prev_sy[k] = -1;
            rows[k] = (WT*)_buffer + bufstep*k;
        }

        const AT* beta = _beta + ksize * range.start;

        for( dy = range.start; dy < range.end; dy++, beta += ksize )
        {
            int sy0 = yofs[dy], k0=ksize, k1=0, ksize2 = ksize/2;

            for(int k = 0; k < ksize; k++ )
            {
                int sy = clip(sy0 - ksize2 + 1 + k, 0, ssize.height);
                for( k1 = std::max(k1, k); k1 < ksize; k1++ )
                {
                    if( sy == prev_sy[k1] ) // if the sy-th row has been computed already, reuse it.
                    {
                        if( k1 > k )
                            memcpy( rows[k], rows[k1], bufstep*sizeof(rows[0][0]) );
                        break;
                    }
                }
                if( k1 == ksize )
                    k0 = std::min(k0, k); // remember the first row that needs to be computed
                srows[k] = (T*)(src.data + src.step*sy);
                prev_sy[k] = sy;
            }

            if( k0 < ksize )
                hresize( (const T**)(srows + k0), (WT**)(rows + k0), ksize - k0, xofs, (const AT*)(alpha),
                        ssize.width, dsize.width, cn, xmin, xmax );
            vresize( (const WT**)rows, (T*)(dst.data + dst.step*dy), beta, dsize.width );
        }
    }
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 8)
# pragma GCC diagnostic pop
#endif

private:
    Mat src;
    Mat dst;
    const int* xofs, *yofs;
    const AT* alpha, *_beta;
    Size ssize, dsize;
    const int ksize, xmin, xmax;

    resizeGeneric_Invoker& operator = (const resizeGeneric_Invoker&);
};

template<class HResize, class VResize>
static void resizeGeneric_( const Mat& src, Mat& dst,
                            const int* xofs, const void* _alpha,
                            const int* yofs, const void* _beta,
                            int xmin, int xmax, int ksize )
{
    typedef typename HResize::alpha_type AT;

    const AT* beta = (const AT*)_beta;
    Size ssize = src.size(), dsize = dst.size();
    int cn = src.channels();
    ssize.width *= cn;
    dsize.width *= cn;
    xmin *= cn;
    xmax *= cn;
    // image resize is a separable operation. In case of not too strong

    Range range(0, dsize.height);
    resizeGeneric_Invoker<HResize, VResize> invoker(src, dst, xofs, yofs, (const AT*)_alpha, beta,
        ssize, dsize, ksize, xmin, xmax);
    parallel_for_(range, invoker, dst.total()/(double)(1<<16));
}

template <typename T, typename WT>
struct ResizeAreaFastNoVec
{
    ResizeAreaFastNoVec(int /*_scale_x*/, int /*_scale_y*/,
        int /*_cn*/, int /*_step*//*, const int**/ /*_ofs*/) { }
    int operator() (const T* /*S*/, T* /*D*/, int /*w*/) const { return 0; }
};

template<typename T>
struct ResizeAreaFastVec
{
    ResizeAreaFastVec(int _scale_x, int _scale_y, int _cn, int _step/*, const int* _ofs*/) :
        scale_x(_scale_x), scale_y(_scale_y), cn(_cn), step(_step)/*, ofs(_ofs)*/
    {
        fast_mode = scale_x == 2 && scale_y == 2 && (cn == 1 || cn == 3 || cn == 4);
    }

    int operator() (const T* S, T* D, int w) const
    {
        if( !fast_mode )
            return 0;

        const T* nextS = (const T*)((const uchar*)S + step);
        int dx = 0;

        if (cn == 1)
            for( ; dx < w; ++dx )
            {
                int index = dx*2;
                D[dx] = (T)((S[index] + S[index+1] + nextS[index] + nextS[index+1] + 2) >> 2);
            }
        else if (cn == 3)
            for( ; dx < w; dx += 3 )
            {
                int index = dx*2;
                D[dx] = (T)((S[index] + S[index+3] + nextS[index] + nextS[index+3] + 2) >> 2);
                D[dx+1] = (T)((S[index+1] + S[index+4] + nextS[index+1] + nextS[index+4] + 2) >> 2);
                D[dx+2] = (T)((S[index+2] + S[index+5] + nextS[index+2] + nextS[index+5] + 2) >> 2);
            }
        else
            {
                assert(cn == 4);
                for( ; dx < w; dx += 4 )
                {
                    int index = dx*2;
                    D[dx] = (T)((S[index] + S[index+4] + nextS[index] + nextS[index+4] + 2) >> 2);
                    D[dx+1] = (T)((S[index+1] + S[index+5] + nextS[index+1] + nextS[index+5] + 2) >> 2);
                    D[dx+2] = (T)((S[index+2] + S[index+6] + nextS[index+2] + nextS[index+6] + 2) >> 2);
                    D[dx+3] = (T)((S[index+3] + S[index+7] + nextS[index+3] + nextS[index+7] + 2) >> 2);
                }
            }

        return dx;
    }

private:
    int scale_x, scale_y;
    int cn;
    bool fast_mode;
    int step;
};

template <typename T, typename WT, typename VecOp>
class resizeAreaFast_Invoker :
    public ParallelLoopBody
{
public:
    resizeAreaFast_Invoker(const Mat &_src, Mat &_dst,
        int _scale_x, int _scale_y, const int* _ofs, const int* _xofs) :
        ParallelLoopBody(), src(_src), dst(_dst), scale_x(_scale_x),
        scale_y(_scale_y), ofs(_ofs), xofs(_xofs)
    {
    }

    virtual void operator() (const Range& range) const
    {
        Size ssize = src.size(), dsize = dst.size();
        int cn = src.channels();
        int area = scale_x*scale_y;
        float scale = 1.f/(area);
        int dwidth1 = (ssize.width/scale_x)*cn;
        dsize.width *= cn;
        ssize.width *= cn;
        int dy, dx, k = 0;

        VecOp vop(scale_x, scale_y, src.channels(), (int)src.step/*, area_ofs*/);

        for( dy = range.start; dy < range.end; dy++ )
        {
            T* D = (T*)(dst.data + dst.step*dy);
            int sy0 = dy*scale_y;
            int w = sy0 + scale_y <= ssize.height ? dwidth1 : 0;

            if( sy0 >= ssize.height )
            {
                for( dx = 0; dx < dsize.width; dx++ )
                    D[dx] = 0;
                continue;
            }

            dx = vop((const T*)(src.data + src.step * sy0), D, w);
            for( ; dx < w; dx++ )
            {
                const T* S = (const T*)(src.data + src.step * sy0) + xofs[dx];
                WT sum = 0;
                k = 0;
                #if CV_ENABLE_UNROLLED
                for( ; k <= area - 4; k += 4 )
                    sum += S[ofs[k]] + S[ofs[k+1]] + S[ofs[k+2]] + S[ofs[k+3]];
                #endif
                for( ; k < area; k++ )
                    sum += S[ofs[k]];

                D[dx] = saturate_cast<T>(sum * scale);
            }

            for( ; dx < dsize.width; dx++ )
            {
                WT sum = 0;
                int count = 0, sx0 = xofs[dx];
                if( sx0 >= ssize.width )
                    D[dx] = 0;

                for( int sy = 0; sy < scale_y; sy++ )
                {
                    if( sy0 + sy >= ssize.height )
                        break;
                    const T* S = (const T*)(src.data + src.step*(sy0 + sy)) + sx0;
                    for( int sx = 0; sx < scale_x*cn; sx += cn )
                    {
                        if( sx0 + sx >= ssize.width )
                            break;
                        sum += S[sx];
                        count++;
                    }
                }

                D[dx] = saturate_cast<T>((float)sum/count);
            }
        }
    }

private:
    Mat src;
    Mat dst;
    int scale_x, scale_y;
    const int *ofs, *xofs;
};

template<typename T, typename WT, typename VecOp>
static void resizeAreaFast_( const Mat& src, Mat& dst, const int* ofs, const int* xofs,
                             int scale_x, int scale_y )
{
    Range range(0, dst.rows);
    resizeAreaFast_Invoker<T, WT, VecOp> invoker(src, dst, scale_x,
        scale_y, ofs, xofs);
    parallel_for_(range, invoker, dst.total()/(double)(1<<16));
}

struct DecimateAlpha
{
    int si, di;
    float alpha;
};


template<typename T, typename WT> class ResizeArea_Invoker :
    public ParallelLoopBody
{
public:
    ResizeArea_Invoker( const Mat& _src, Mat& _dst,
                        const DecimateAlpha* _xtab, int _xtab_size,
                        const DecimateAlpha* _ytab, int _ytab_size,
                        const int* _tabofs )
    {
        src = &_src;
        dst = &_dst;
        xtab0 = _xtab;
        xtab_size0 = _xtab_size;
        ytab = _ytab;
        ytab_size = _ytab_size;
        tabofs = _tabofs;
    }

    virtual void operator() (const Range& range) const
    {
        Size dsize = dst->size();
        int cn = dst->channels();
        dsize.width *= cn;
        AutoBuffer<WT> _buffer(dsize.width*2);
        const DecimateAlpha* xtab = xtab0;
        int xtab_size = xtab_size0;
        WT *buf = _buffer, *sum = buf + dsize.width;
        int j_start = tabofs[range.start], j_end = tabofs[range.end], j, k, dx, prev_dy = ytab[j_start].di;

        for( dx = 0; dx < dsize.width; dx++ )
            sum[dx] = (WT)0;

        for( j = j_start; j < j_end; j++ )
        {
            WT beta = ytab[j].alpha;
            int dy = ytab[j].di;
            int sy = ytab[j].si;

            {
                const T* S = (const T*)(src->data + src->step*sy);
                for( dx = 0; dx < dsize.width; dx++ )
                    buf[dx] = (WT)0;

                if( cn == 1 )
                    for( k = 0; k < xtab_size; k++ )
                    {
                        int dxn = xtab[k].di;
                        WT alpha = xtab[k].alpha;
                        buf[dxn] += S[xtab[k].si]*alpha;
                    }
                else if( cn == 2 )
                    for( k = 0; k < xtab_size; k++ )
                    {
                        int sxn = xtab[k].si;
                        int dxn = xtab[k].di;
                        WT alpha = xtab[k].alpha;
                        WT t0 = buf[dxn] + S[sxn]*alpha;
                        WT t1 = buf[dxn+1] + S[sxn+1]*alpha;
                        buf[dxn] = t0; buf[dxn+1] = t1;
                    }
                else if( cn == 3 )
                    for( k = 0; k < xtab_size; k++ )
                    {
                        int sxn = xtab[k].si;
                        int dxn = xtab[k].di;
                        WT alpha = xtab[k].alpha;
                        WT t0 = buf[dxn] + S[sxn]*alpha;
                        WT t1 = buf[dxn+1] + S[sxn+1]*alpha;
                        WT t2 = buf[dxn+2] + S[sxn+2]*alpha;
                        buf[dxn] = t0; buf[dxn+1] = t1; buf[dxn+2] = t2;
                    }
                else if( cn == 4 )
                {
                    for( k = 0; k < xtab_size; k++ )
                    {
                        int sxn = xtab[k].si;
                        int dxn = xtab[k].di;
                        WT alpha = xtab[k].alpha;
                        WT t0 = buf[dxn] + S[sxn]*alpha;
                        WT t1 = buf[dxn+1] + S[sxn+1]*alpha;
                        buf[dxn] = t0; buf[dxn+1] = t1;
                        t0 = buf[dxn+2] + S[sxn+2]*alpha;
                        t1 = buf[dxn+3] + S[sxn+3]*alpha;
                        buf[dxn+2] = t0; buf[dxn+3] = t1;
                    }
                }
                else
                {
                    for( k = 0; k < xtab_size; k++ )
                    {
                        int sxn = xtab[k].si;
                        int dxn = xtab[k].di;
                        WT alpha = xtab[k].alpha;
                        for( int c = 0; c < cn; c++ )
                            buf[dxn + c] += S[sxn + c]*alpha;
                    }
                }
            }

            if( dy != prev_dy )
            {
                T* D = (T*)(dst->data + dst->step*prev_dy);

                for( dx = 0; dx < dsize.width; dx++ )
                {
                    D[dx] = saturate_cast<T>(sum[dx]);
                    sum[dx] = beta*buf[dx];
                }
                prev_dy = dy;
            }
            else
            {
                for( dx = 0; dx < dsize.width; dx++ )
                    sum[dx] += beta*buf[dx];
            }
        }

        {
        T* D = (T*)(dst->data + dst->step*prev_dy);
        for( dx = 0; dx < dsize.width; dx++ )
            D[dx] = saturate_cast<T>(sum[dx]);
        }
    }

private:
    const Mat* src;
    Mat* dst;
    const DecimateAlpha* xtab0;
    const DecimateAlpha* ytab;
    int xtab_size0, ytab_size;
    const int* tabofs;
};


template <typename T, typename WT>
static void resizeArea_( const Mat& src, Mat& dst,
                         const DecimateAlpha* xtab, int xtab_size,
                         const DecimateAlpha* ytab, int ytab_size,
                         const int* tabofs )
{
    parallel_for_(Range(0, dst.rows),
                 ResizeArea_Invoker<T, WT>(src, dst, xtab, xtab_size, ytab, ytab_size, tabofs),
                 dst.total()/((double)(1 << 16)));
}


typedef void (*ResizeFunc)( const Mat& src, Mat& dst,
                            const int* xofs, const void* alpha,
                            const int* yofs, const void* beta,
                            int xmin, int xmax, int ksize );

typedef void (*ResizeAreaFastFunc)( const Mat& src, Mat& dst,
                                    const int* ofs, const int *xofs,
                                    int scale_x, int scale_y );

typedef void (*ResizeAreaFunc)( const Mat& src, Mat& dst,
                                const DecimateAlpha* xtab, int xtab_size,
                                const DecimateAlpha* ytab, int ytab_size,
                                const int* yofs);


static int computeResizeAreaTab( int ssize, int dsize, int cn, double scale, DecimateAlpha* tab )
{
    int k = 0;
    for(int dx = 0; dx < dsize; dx++ )
    {
        double fsx1 = dx * scale;
        double fsx2 = fsx1 + scale;
        double cellWidth = min(scale, ssize - fsx1);

        int sx1 = cvCeil(fsx1), sx2 = cvFloor(fsx2);

        sx2 = std::min(sx2, ssize - 1);
        sx1 = std::min(sx1, sx2);

        if( sx1 - fsx1 > 1e-3 )
        {
            assert( k < ssize*2 );
            tab[k].di = dx * cn;
            tab[k].si = (sx1 - 1) * cn;
            tab[k++].alpha = (float)((sx1 - fsx1) / cellWidth);
        }

        for(int sx = sx1; sx < sx2; sx++ )
        {
            assert( k < ssize*2 );
            tab[k].di = dx * cn;
            tab[k].si = sx * cn;
            tab[k++].alpha = float(1.0 / cellWidth);
        }

        if( fsx2 - sx2 > 1e-3 )
        {
            assert( k < ssize*2 );
            tab[k].di = dx * cn;
            tab[k].si = sx2 * cn;
            tab[k++].alpha = (float)(min(min(fsx2 - sx2, 1.), cellWidth) / cellWidth);
        }
    }
    return k;
}


}


//////////////////////////////////////////////////////////////////////////////////////////

void cv::resize( InputArray _src, OutputArray _dst, Size dsize,
                 double inv_scale_x, double inv_scale_y, int interpolation )
{
    static ResizeFunc linear_tab[] =
    {
        resizeGeneric_<
            HResizeLinear<uchar, int, short,
                INTER_RESIZE_COEF_SCALE,
                HResizeLinearVec_8u32s>,
            VResizeLinear<uchar, int, short,
                FixedPtCast<int, uchar, INTER_RESIZE_COEF_BITS*2>,
                VResizeLinearVec_32s8u> >,
        0,
        resizeGeneric_<
            HResizeLinear<ushort, float, float, 1,
                HResizeLinearVec_16u32f>,
            VResizeLinear<ushort, float, float, Cast<float, ushort>,
                VResizeLinearVec_32f16u> >,
        resizeGeneric_<
            HResizeLinear<short, float, float, 1,
                HResizeLinearVec_16s32f>,
            VResizeLinear<short, float, float, Cast<float, short>,
                VResizeLinearVec_32f16s> >,
        0,
        resizeGeneric_<
            HResizeLinear<float, float, float, 1,
                HResizeLinearVec_32f>,
            VResizeLinear<float, float, float, Cast<float, float>,
                VResizeLinearVec_32f> >,
        resizeGeneric_<
            HResizeLinear<double, double, float, 1,
                HResizeNoVec>,
            VResizeLinear<double, double, float, Cast<double, double>,
                VResizeNoVec> >,
        0
    };

    static ResizeFunc cubic_tab[] =
    {
        resizeGeneric_<
            HResizeCubic<uchar, int, short>,
            VResizeCubic<uchar, int, short,
                FixedPtCast<int, uchar, INTER_RESIZE_COEF_BITS*2>,
                VResizeCubicVec_32s8u> >,
        0,
        resizeGeneric_<
            HResizeCubic<ushort, float, float>,
            VResizeCubic<ushort, float, float, Cast<float, ushort>,
            VResizeCubicVec_32f16u> >,
        resizeGeneric_<
            HResizeCubic<short, float, float>,
            VResizeCubic<short, float, float, Cast<float, short>,
            VResizeCubicVec_32f16s> >,
        0,
        resizeGeneric_<
            HResizeCubic<float, float, float>,
            VResizeCubic<float, float, float, Cast<float, float>,
            VResizeCubicVec_32f> >,
        resizeGeneric_<
            HResizeCubic<double, double, float>,
            VResizeCubic<double, double, float, Cast<double, double>,
            VResizeNoVec> >,
        0
    };

    static ResizeFunc lanczos4_tab[] =
    {
        resizeGeneric_<HResizeLanczos4<uchar, int, short>,
            VResizeLanczos4<uchar, int, short,
            FixedPtCast<int, uchar, INTER_RESIZE_COEF_BITS*2>,
            VResizeNoVec> >,
        0,
        resizeGeneric_<HResizeLanczos4<ushort, float, float>,
            VResizeLanczos4<ushort, float, float, Cast<float, ushort>,
            VResizeNoVec> >,
        resizeGeneric_<HResizeLanczos4<short, float, float>,
            VResizeLanczos4<short, float, float, Cast<float, short>,
            VResizeNoVec> >,
        0,
        resizeGeneric_<HResizeLanczos4<float, float, float>,
            VResizeLanczos4<float, float, float, Cast<float, float>,
            VResizeNoVec> >,
        resizeGeneric_<HResizeLanczos4<double, double, float>,
            VResizeLanczos4<double, double, float, Cast<double, double>,
            VResizeNoVec> >,
        0
    };

    static ResizeAreaFastFunc areafast_tab[] =
    {
        resizeAreaFast_<uchar, int, ResizeAreaFastVec<uchar> >,
        0,
        resizeAreaFast_<ushort, float, ResizeAreaFastVec<ushort> >,
        resizeAreaFast_<short, float, ResizeAreaFastVec<short> >,
        0,
        resizeAreaFast_<float, float, ResizeAreaFastNoVec<float, float> >,
        resizeAreaFast_<double, double, ResizeAreaFastNoVec<double, double> >,
        0
    };

    static ResizeAreaFunc area_tab[] =
    {
        resizeArea_<uchar, float>, 0, resizeArea_<ushort, float>,
        resizeArea_<short, float>, 0, resizeArea_<float, float>,
        resizeArea_<double, double>, 0
    };

    Mat src = _src.getMat();
    Size ssize = src.size();

    CV_Assert( ssize.area() > 0 );
    CV_Assert( dsize.area() || (inv_scale_x > 0 && inv_scale_y > 0) );
    if( !dsize.area() )
    {
        dsize = Size(saturate_cast<int>(src.cols*inv_scale_x),
            saturate_cast<int>(src.rows*inv_scale_y));
        CV_Assert( dsize.area() );
    }
    else
    {
        inv_scale_x = (double)dsize.width/src.cols;
        inv_scale_y = (double)dsize.height/src.rows;
    }
    _dst.create(dsize, src.type());
    Mat dst = _dst.getMat();




    int depth = src.depth(), cn = src.channels();
    double scale_x = 1./inv_scale_x, scale_y = 1./inv_scale_y;
    int k, sx, sy, dx, dy;

    if( interpolation == INTER_NEAREST )
    {
        resizeNN( src, dst, inv_scale_x, inv_scale_y );
        return;
    }

    {
        int iscale_x = saturate_cast<int>(scale_x);
        int iscale_y = saturate_cast<int>(scale_y);

        bool is_area_fast = std::abs(scale_x - iscale_x) < DBL_EPSILON &&
                std::abs(scale_y - iscale_y) < DBL_EPSILON;

        // in case of scale_x && scale_y is equal to 2
        // INTER_AREA (fast) also is equal to INTER_LINEAR
        if( interpolation == INTER_LINEAR && is_area_fast && iscale_x == 2 && iscale_y == 2 )
        {
            interpolation = INTER_AREA;
        }

        // true "area" interpolation is only implemented for the case (scale_x <= 1 && scale_y <= 1).
        // In other cases it is emulated using some variant of bilinear interpolation
        if( interpolation == INTER_AREA && scale_x >= 1 && scale_y >= 1 )
        {
            if( is_area_fast )
            {
                int area = iscale_x*iscale_y;
                size_t srcstep = src.step / src.elemSize1();
                AutoBuffer<int> _ofs(area + dsize.width*cn);
                int* ofs = _ofs;
                int* xofs = ofs + area;
                ResizeAreaFastFunc func = areafast_tab[depth];
                CV_Assert( func != 0 );

                for( sy = 0, k = 0; sy < iscale_y; sy++ )
                    for( sx = 0; sx < iscale_x; sx++ )
                        ofs[k++] = (int)(sy*srcstep + sx*cn);

                for( dx = 0; dx < dsize.width; dx++ )
                {
                    int j = dx * cn;
                    sx = iscale_x * j;
                    for( k = 0; k < cn; k++ )
                        xofs[j + k] = sx + k;
                }

                func( src, dst, ofs, xofs, iscale_x, iscale_y );
                return;
            }

            ResizeAreaFunc func = area_tab[depth];
            CV_Assert( func != 0 && cn <= 4 );

            AutoBuffer<DecimateAlpha> _xytab((ssize.width + ssize.height)*2);
            DecimateAlpha* xtab = _xytab, *ytab = xtab + ssize.width*2;

            int xtab_size = computeResizeAreaTab(ssize.width, dsize.width, cn, scale_x, xtab);
            int ytab_size = computeResizeAreaTab(ssize.height, dsize.height, 1, scale_y, ytab);

            AutoBuffer<int> _tabofs(dsize.height + 1);
            int* tabofs = _tabofs;
            for( k = 0, dy = 0; k < ytab_size; k++ )
            {
                if( k == 0 || ytab[k].di != ytab[k-1].di )
                {
                    assert( ytab[k].di == dy );
                    tabofs[dy++] = k;
                }
            }
            tabofs[dy] = ytab_size;

            func( src, dst, xtab, xtab_size, ytab, ytab_size, tabofs );
            return;
        }
    }

    int xmin = 0, xmax = dsize.width, width = dsize.width*cn;
    bool area_mode = interpolation == INTER_AREA;
    bool fixpt = depth == CV_8U;
    float fx, fy;
    ResizeFunc func=0;
    int ksize=0, ksize2;
    if( interpolation == INTER_CUBIC )
        ksize = 4, func = cubic_tab[depth];
    else if( interpolation == INTER_LANCZOS4 )
        ksize = 8, func = lanczos4_tab[depth];
    else if( interpolation == INTER_LINEAR || interpolation == INTER_AREA )
        ksize = 2, func = linear_tab[depth];
    else
        CV_Error( CV_StsBadArg, "Unknown interpolation method" );
    ksize2 = ksize/2;

    CV_Assert( func != 0 );

    AutoBuffer<uchar> _buffer((width + dsize.height)*(sizeof(int) + sizeof(float)*ksize));
    int* xofs = (int*)(uchar*)_buffer;
    int* yofs = xofs + width;
    float* alpha = (float*)(yofs + dsize.height);
    short* ialpha = (short*)alpha;
    float* beta = alpha + width*ksize;
    short* ibeta = ialpha + width*ksize;
    float cbuf[MAX_ESIZE];

    for( dx = 0; dx < dsize.width; dx++ )
    {
        if( !area_mode )
        {
            fx = (float)((dx+0.5)*scale_x - 0.5);
            sx = cvFloor(fx);
            fx -= sx;
        }
        else
        {
            sx = cvFloor(dx*scale_x);
            fx = (float)((dx+1) - (sx+1)*inv_scale_x);
            fx = fx <= 0 ? 0.f : fx - cvFloor(fx);
        }

        if( sx < ksize2-1 )
        {
            xmin = dx+1;
            if( sx < 0 )
                fx = 0, sx = 0;
        }

        if( sx + ksize2 >= ssize.width )
        {
            xmax = std::min( xmax, dx );
            if( sx >= ssize.width-1 )
                fx = 0, sx = ssize.width-1;
        }

        for( k = 0, sx *= cn; k < cn; k++ )
            xofs[dx*cn + k] = sx + k;

        if( interpolation == INTER_CUBIC )
            interpolateCubic( fx, cbuf );
        else if( interpolation == INTER_LANCZOS4 )
            interpolateLanczos4( fx, cbuf );
        else
        {
            cbuf[0] = 1.f - fx;
            cbuf[1] = fx;
        }
        if( fixpt )
        {
            for( k = 0; k < ksize; k++ )
                ialpha[dx*cn*ksize + k] = saturate_cast<short>(cbuf[k]*INTER_RESIZE_COEF_SCALE);
            for( ; k < cn*ksize; k++ )
                ialpha[dx*cn*ksize + k] = ialpha[dx*cn*ksize + k - ksize];
        }
        else
        {
            for( k = 0; k < ksize; k++ )
                alpha[dx*cn*ksize + k] = cbuf[k];
            for( ; k < cn*ksize; k++ )
                alpha[dx*cn*ksize + k] = alpha[dx*cn*ksize + k - ksize];
        }
    }

    for( dy = 0; dy < dsize.height; dy++ )
    {
        if( !area_mode )
        {
            fy = (float)((dy+0.5)*scale_y - 0.5);
            sy = cvFloor(fy);
            fy -= sy;
        }
        else
        {
            sy = cvFloor(dy*scale_y);
            fy = (float)((dy+1) - (sy+1)*inv_scale_y);
            fy = fy <= 0 ? 0.f : fy - cvFloor(fy);
        }

        yofs[dy] = sy;
        if( interpolation == INTER_CUBIC )
            interpolateCubic( fy, cbuf );
        else if( interpolation == INTER_LANCZOS4 )
            interpolateLanczos4( fy, cbuf );
        else
        {
            cbuf[0] = 1.f - fy;
            cbuf[1] = fy;
        }

        if( fixpt )
        {
            for( k = 0; k < ksize; k++ )
                ibeta[dy*ksize + k] = saturate_cast<short>(cbuf[k]*INTER_RESIZE_COEF_SCALE);
        }
        else
        {
            for( k = 0; k < ksize; k++ )
                beta[dy*ksize + k] = cbuf[k];
        }
    }

    func( src, dst, xofs, fixpt ? (void*)ialpha : (void*)alpha, yofs,
          fixpt ? (void*)ibeta : (void*)beta, xmin, xmax, ksize );
}


/****************************************************************************************\
*                       General warping (affine, perspective, remap)                     *
\****************************************************************************************/

namespace cv
{

template<typename T>
static void remapNearest( const Mat& _src, Mat& _dst, const Mat& _xy,
                          int borderType, const Scalar& _borderValue )
{
    Size ssize = _src.size(), dsize = _dst.size();
    int cn = _src.channels();
    const T* S0 = (const T*)_src.data;
    size_t sstep = _src.step/sizeof(S0[0]);
    Scalar_<T> cval(saturate_cast<T>(_borderValue[0]),
        saturate_cast<T>(_borderValue[1]),
        saturate_cast<T>(_borderValue[2]),
        saturate_cast<T>(_borderValue[3]));
    int dx, dy;

    unsigned width1 = ssize.width, height1 = ssize.height;

    if( _dst.isContinuous() && _xy.isContinuous() )
    {
        dsize.width *= dsize.height;
        dsize.height = 1;
    }

    for( dy = 0; dy < dsize.height; dy++ )
    {
        T* D = (T*)(_dst.data + _dst.step*dy);
        const short* XY = (const short*)(_xy.data + _xy.step*dy);

        if( cn == 1 )
        {
            for( dx = 0; dx < dsize.width; dx++ )
            {
                int sx = XY[dx*2], sy = XY[dx*2+1];
                if( (unsigned)sx < width1 && (unsigned)sy < height1 )
                    D[dx] = S0[sy*sstep + sx];
                else
                {
                    if( borderType == BORDER_REPLICATE )
                    {
                        sx = clip(sx, 0, ssize.width);
                        sy = clip(sy, 0, ssize.height);
                        D[dx] = S0[sy*sstep + sx];
                    }
                    else if( borderType == BORDER_CONSTANT )
                        D[dx] = cval[0];
                    else if( borderType != BORDER_TRANSPARENT )
                    {
                        sx = borderInterpolate(sx, ssize.width, borderType);
                        sy = borderInterpolate(sy, ssize.height, borderType);
                        D[dx] = S0[sy*sstep + sx];
                    }
                }
            }
        }
        else
        {
            for( dx = 0; dx < dsize.width; dx++, D += cn )
            {
                int sx = XY[dx*2], sy = XY[dx*2+1], k;
                const T *S;
                if( (unsigned)sx < width1 && (unsigned)sy < height1 )
                {
                    if( cn == 3 )
                    {
                        S = S0 + sy*sstep + sx*3;
                        D[0] = S[0], D[1] = S[1], D[2] = S[2];
                    }
                    else if( cn == 4 )
                    {
                        S = S0 + sy*sstep + sx*4;
                        D[0] = S[0], D[1] = S[1], D[2] = S[2], D[3] = S[3];
                    }
                    else
                    {
                        S = S0 + sy*sstep + sx*cn;
                        for( k = 0; k < cn; k++ )
                            D[k] = S[k];
                    }
                }
                else if( borderType != BORDER_TRANSPARENT )
                {
                    if( borderType == BORDER_REPLICATE )
                    {
                        sx = clip(sx, 0, ssize.width);
                        sy = clip(sy, 0, ssize.height);
                        S = S0 + sy*sstep + sx*cn;
                    }
                    else if( borderType == BORDER_CONSTANT )
                        S = &cval[0];
                    else
                    {
                        sx = borderInterpolate(sx, ssize.width, borderType);
                        sy = borderInterpolate(sy, ssize.height, borderType);
                        S = S0 + sy*sstep + sx*cn;
                    }
                    for( k = 0; k < cn; k++ )
                        D[k] = S[k];
                }
            }
        }
    }
}


struct RemapNoVec
{
    int operator()( const Mat&, void*, const short*, const ushort*,
                    const void*, int ) const { return 0; }
};


typedef RemapNoVec RemapVec_8u;


template<class CastOp, class VecOp, typename AT>
static void remapBilinear( const Mat& _src, Mat& _dst, const Mat& _xy,
                           const Mat& _fxy, const void* _wtab,
                           int borderType, const Scalar& _borderValue )
{
    typedef typename CastOp::rtype T;
    typedef typename CastOp::type1 WT;
    Size ssize = _src.size(), dsize = _dst.size();
    int cn = _src.channels();
    const AT* wtab = (const AT*)_wtab;
    const T* S0 = (const T*)_src.data;
    size_t sstep = _src.step/sizeof(S0[0]);
    Scalar_<T> cval(saturate_cast<T>(_borderValue[0]),
        saturate_cast<T>(_borderValue[1]),
        saturate_cast<T>(_borderValue[2]),
        saturate_cast<T>(_borderValue[3]));
    int dx, dy;
    CastOp castOp;
    VecOp vecOp;

    unsigned width1 = std::max(ssize.width-1, 0), height1 = std::max(ssize.height-1, 0);
    CV_Assert( cn <= 4 && ssize.area() > 0 );
#if CV_SSE2
    if( _src.type() == CV_8UC3 )
        width1 = std::max(ssize.width-2, 0);
#endif

    for( dy = 0; dy < dsize.height; dy++ )
    {
        T* D = (T*)(_dst.data + _dst.step*dy);
        const short* XY = (const short*)(_xy.data + _xy.step*dy);
        const ushort* FXY = (const ushort*)(_fxy.data + _fxy.step*dy);
        int X0 = 0;
        bool prevInlier = false;

        for( dx = 0; dx <= dsize.width; dx++ )
        {
            bool curInlier = dx < dsize.width ?
                (unsigned)XY[dx*2] < width1 &&
                (unsigned)XY[dx*2+1] < height1 : !prevInlier;
            if( curInlier == prevInlier )
                continue;

            int X1 = dx;
            dx = X0;
            X0 = X1;
            prevInlier = curInlier;

            if( !curInlier )
            {
                int len = vecOp( _src, D, XY + dx*2, FXY + dx, wtab, X1 - dx );
                D += len*cn;
                dx += len;

                if( cn == 1 )
                {
                    for( ; dx < X1; dx++, D++ )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx;
                        *D = castOp(WT(S[0]*w[0] + S[1]*w[1] + S[sstep]*w[2] + S[sstep+1]*w[3]));
                    }
                }
                else if( cn == 2 )
                    for( ; dx < X1; dx++, D += 2 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*2;
                        WT t0 = S[0]*w[0] + S[2]*w[1] + S[sstep]*w[2] + S[sstep+2]*w[3];
                        WT t1 = S[1]*w[0] + S[3]*w[1] + S[sstep+1]*w[2] + S[sstep+3]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1);
                    }
                else if( cn == 3 )
                    for( ; dx < X1; dx++, D += 3 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*3;
                        WT t0 = S[0]*w[0] + S[3]*w[1] + S[sstep]*w[2] + S[sstep+3]*w[3];
                        WT t1 = S[1]*w[0] + S[4]*w[1] + S[sstep+1]*w[2] + S[sstep+4]*w[3];
                        WT t2 = S[2]*w[0] + S[5]*w[1] + S[sstep+2]*w[2] + S[sstep+5]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1); D[2] = castOp(t2);
                    }
                else
                    for( ; dx < X1; dx++, D += 4 )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        const AT* w = wtab + FXY[dx]*4;
                        const T* S = S0 + sy*sstep + sx*4;
                        WT t0 = S[0]*w[0] + S[4]*w[1] + S[sstep]*w[2] + S[sstep+4]*w[3];
                        WT t1 = S[1]*w[0] + S[5]*w[1] + S[sstep+1]*w[2] + S[sstep+5]*w[3];
                        D[0] = castOp(t0); D[1] = castOp(t1);
                        t0 = S[2]*w[0] + S[6]*w[1] + S[sstep+2]*w[2] + S[sstep+6]*w[3];
                        t1 = S[3]*w[0] + S[7]*w[1] + S[sstep+3]*w[2] + S[sstep+7]*w[3];
                        D[2] = castOp(t0); D[3] = castOp(t1);
                    }
            }
            else
            {
                if( borderType == BORDER_TRANSPARENT && cn != 3 )
                {
                    D += (X1 - dx)*cn;
                    dx = X1;
                    continue;
                }

                if( cn == 1 )
                    for( ; dx < X1; dx++, D++ )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1];
                        if( borderType == BORDER_CONSTANT &&
                            (sx >= ssize.width || sx+1 < 0 ||
                             sy >= ssize.height || sy+1 < 0) )
                        {
                            D[0] = cval[0];
                        }
                        else
                        {
                            int sx0, sx1, sy0, sy1;
                            T v0, v1, v2, v3;
                            const AT* w = wtab + FXY[dx]*4;
                            if( borderType == BORDER_REPLICATE )
                            {
                                sx0 = clip(sx, 0, ssize.width);
                                sx1 = clip(sx+1, 0, ssize.width);
                                sy0 = clip(sy, 0, ssize.height);
                                sy1 = clip(sy+1, 0, ssize.height);
                                v0 = S0[sy0*sstep + sx0];
                                v1 = S0[sy0*sstep + sx1];
                                v2 = S0[sy1*sstep + sx0];
                                v3 = S0[sy1*sstep + sx1];
                            }
                            else
                            {
                                sx0 = borderInterpolate(sx, ssize.width, borderType);
                                sx1 = borderInterpolate(sx+1, ssize.width, borderType);
                                sy0 = borderInterpolate(sy, ssize.height, borderType);
                                sy1 = borderInterpolate(sy+1, ssize.height, borderType);
                                v0 = sx0 >= 0 && sy0 >= 0 ? S0[sy0*sstep + sx0] : cval[0];
                                v1 = sx1 >= 0 && sy0 >= 0 ? S0[sy0*sstep + sx1] : cval[0];
                                v2 = sx0 >= 0 && sy1 >= 0 ? S0[sy1*sstep + sx0] : cval[0];
                                v3 = sx1 >= 0 && sy1 >= 0 ? S0[sy1*sstep + sx1] : cval[0];
                            }
                            D[0] = castOp(WT(v0*w[0] + v1*w[1] + v2*w[2] + v3*w[3]));
                        }
                    }
                else
                    for( ; dx < X1; dx++, D += cn )
                    {
                        int sx = XY[dx*2], sy = XY[dx*2+1], k;
                        if( borderType == BORDER_CONSTANT &&
                            (sx >= ssize.width || sx+1 < 0 ||
                             sy >= ssize.height || sy+1 < 0) )
                        {
                            for( k = 0; k < cn; k++ )
                                D[k] = cval[k];
                        }
                        else
                        {
                            int sx0, sx1, sy0, sy1;
                            const T *v0, *v1, *v2, *v3;
                            const AT* w = wtab + FXY[dx]*4;
                            if( borderType == BORDER_REPLICATE )
                            {
                                sx0 = clip(sx, 0, ssize.width);
                                sx1 = clip(sx+1, 0, ssize.width);
                                sy0 = clip(sy, 0, ssize.height);
                                sy1 = clip(sy+1, 0, ssize.height);
                                v0 = S0 + sy0*sstep + sx0*cn;
                                v1 = S0 + sy0*sstep + sx1*cn;
                                v2 = S0 + sy1*sstep + sx0*cn;
                                v3 = S0 + sy1*sstep + sx1*cn;
                            }
                            else if( borderType == BORDER_TRANSPARENT &&
                                ((unsigned)sx >= (unsigned)(ssize.width-1) ||
                                (unsigned)sy >= (unsigned)(ssize.height-1)))
                                continue;
                            else
                            {
                                sx0 = borderInterpolate(sx, ssize.width, borderType);
                                sx1 = borderInterpolate(sx+1, ssize.width, borderType);
                                sy0 = borderInterpolate(sy, ssize.height, borderType);
                                sy1 = borderInterpolate(sy+1, ssize.height, borderType);
                                v0 = sx0 >= 0 && sy0 >= 0 ? S0 + sy0*sstep + sx0*cn : &cval[0];
                                v1 = sx1 >= 0 && sy0 >= 0 ? S0 + sy0*sstep + sx1*cn : &cval[0];
                                v2 = sx0 >= 0 && sy1 >= 0 ? S0 + sy1*sstep + sx0*cn : &cval[0];
                                v3 = sx1 >= 0 && sy1 >= 0 ? S0 + sy1*sstep + sx1*cn : &cval[0];
                            }
                            for( k = 0; k < cn; k++ )
                                D[k] = castOp(WT(v0[k]*w[0] + v1[k]*w[1] + v2[k]*w[2] + v3[k]*w[3]));
                        }
                    }
            }
        }
    }
}


template<class CastOp, typename AT, int ONE>
static void remapBicubic( const Mat& _src, Mat& _dst, const Mat& _xy,
                          const Mat& _fxy, const void* _wtab,
                          int borderType, const Scalar& _borderValue )
{
    typedef typename CastOp::rtype T;
    typedef typename CastOp::type1 WT;
    Size ssize = _src.size(), dsize = _dst.size();
    int cn = _src.channels();
    const AT* wtab = (const AT*)_wtab;
    const T* S0 = (const T*)_src.data;
    size_t sstep = _src.step/sizeof(S0[0]);
    Scalar_<T> cval(saturate_cast<T>(_borderValue[0]),
        saturate_cast<T>(_borderValue[1]),
        saturate_cast<T>(_borderValue[2]),
        saturate_cast<T>(_borderValue[3]));
    int dx, dy;
    CastOp castOp;
    int borderType1 = borderType != BORDER_TRANSPARENT ? borderType : BORDER_REFLECT_101;

    unsigned width1 = std::max(ssize.width-3, 0), height1 = std::max(ssize.height-3, 0);

    if( _dst.isContinuous() && _xy.isContinuous() && _fxy.isContinuous() )
    {
        dsize.width *= dsize.height;
        dsize.height = 1;
    }

    for( dy = 0; dy < dsize.height; dy++ )
    {
        T* D = (T*)(_dst.data + _dst.step*dy);
        const short* XY = (const short*)(_xy.data + _xy.step*dy);
        const ushort* FXY = (const ushort*)(_fxy.data + _fxy.step*dy);

        for( dx = 0; dx < dsize.width; dx++, D += cn )
        {
            int sx = XY[dx*2]-1, sy = XY[dx*2+1]-1;
            const AT* w = wtab + FXY[dx]*16;
            int i, k;
            if( (unsigned)sx < width1 && (unsigned)sy < height1 )
            {
                const T* S = S0 + sy*sstep + sx*cn;
                for( k = 0; k < cn; k++ )
                {
                    WT sum = S[0]*w[0] + S[cn]*w[1] + S[cn*2]*w[2] + S[cn*3]*w[3];
                    S += sstep;
                    sum += S[0]*w[4] + S[cn]*w[5] + S[cn*2]*w[6] + S[cn*3]*w[7];
                    S += sstep;
                    sum += S[0]*w[8] + S[cn]*w[9] + S[cn*2]*w[10] + S[cn*3]*w[11];
                    S += sstep;
                    sum += S[0]*w[12] + S[cn]*w[13] + S[cn*2]*w[14] + S[cn*3]*w[15];
                    S += 1 - sstep*3;
                    D[k] = castOp(sum);
                }
            }
            else
            {
                int x[4], y[4];
                if( borderType == BORDER_TRANSPARENT &&
                    ((unsigned)(sx+1) >= (unsigned)ssize.width ||
                    (unsigned)(sy+1) >= (unsigned)ssize.height) )
                    continue;

                if( borderType1 == BORDER_CONSTANT &&
                    (sx >= ssize.width || sx+4 <= 0 ||
                    sy >= ssize.height || sy+4 <= 0))
                {
                    for( k = 0; k < cn; k++ )
                        D[k] = cval[k];
                    continue;
                }

                for( i = 0; i < 4; i++ )
                {
                    x[i] = borderInterpolate(sx + i, ssize.width, borderType1)*cn;
                    y[i] = borderInterpolate(sy + i, ssize.height, borderType1);
                }

                for( k = 0; k < cn; k++, S0++, w -= 16 )
                {
                    WT cv = cval[k], sum = cv*ONE;
                    for( i = 0; i < 4; i++, w += 4 )
                    {
                        int yi = y[i];
                        const T* S = S0 + yi*sstep;
                        if( yi < 0 )
                            continue;
                        if( x[0] >= 0 )
                            sum += (S[x[0]] - cv)*w[0];
                        if( x[1] >= 0 )
                            sum += (S[x[1]] - cv)*w[1];
                        if( x[2] >= 0 )
                            sum += (S[x[2]] - cv)*w[2];
                        if( x[3] >= 0 )
                            sum += (S[x[3]] - cv)*w[3];
                    }
                    D[k] = castOp(sum);
                }
                S0 -= cn;
            }
        }
    }
}


template<class CastOp, typename AT, int ONE>
static void remapLanczos4( const Mat& _src, Mat& _dst, const Mat& _xy,
                           const Mat& _fxy, const void* _wtab,
                           int borderType, const Scalar& _borderValue )
{
    typedef typename CastOp::rtype T;
    typedef typename CastOp::type1 WT;
    Size ssize = _src.size(), dsize = _dst.size();
    int cn = _src.channels();
    const AT* wtab = (const AT*)_wtab;
    const T* S0 = (const T*)_src.data;
    size_t sstep = _src.step/sizeof(S0[0]);
    Scalar_<T> cval(saturate_cast<T>(_borderValue[0]),
        saturate_cast<T>(_borderValue[1]),
        saturate_cast<T>(_borderValue[2]),
        saturate_cast<T>(_borderValue[3]));
    int dx, dy;
    CastOp castOp;
    int borderType1 = borderType != BORDER_TRANSPARENT ? borderType : BORDER_REFLECT_101;

    unsigned width1 = std::max(ssize.width-7, 0), height1 = std::max(ssize.height-7, 0);

    if( _dst.isContinuous() && _xy.isContinuous() && _fxy.isContinuous() )
    {
        dsize.width *= dsize.height;
        dsize.height = 1;
    }

    for( dy = 0; dy < dsize.height; dy++ )
    {
        T* D = (T*)(_dst.data + _dst.step*dy);
        const short* XY = (const short*)(_xy.data + _xy.step*dy);
        const ushort* FXY = (const ushort*)(_fxy.data + _fxy.step*dy);

        for( dx = 0; dx < dsize.width; dx++, D += cn )
        {
            int sx = XY[dx*2]-3, sy = XY[dx*2+1]-3;
            const AT* w = wtab + FXY[dx]*64;
            const T* S = S0 + sy*sstep + sx*cn;
            int i, k;
            if( (unsigned)sx < width1 && (unsigned)sy < height1 )
            {
                for( k = 0; k < cn; k++ )
                {
                    WT sum = 0;
                    for( int r = 0; r < 8; r++, S += sstep, w += 8 )
                        sum += S[0]*w[0] + S[cn]*w[1] + S[cn*2]*w[2] + S[cn*3]*w[3] +
                            S[cn*4]*w[4] + S[cn*5]*w[5] + S[cn*6]*w[6] + S[cn*7]*w[7];
                    w -= 64;
                    S -= sstep*8 - 1;
                    D[k] = castOp(sum);
                }
            }
            else
            {
                int x[8], y[8];
                if( borderType == BORDER_TRANSPARENT &&
                    ((unsigned)(sx+3) >= (unsigned)ssize.width ||
                    (unsigned)(sy+3) >= (unsigned)ssize.height) )
                    continue;

                if( borderType1 == BORDER_CONSTANT &&
                    (sx >= ssize.width || sx+8 <= 0 ||
                    sy >= ssize.height || sy+8 <= 0))
                {
                    for( k = 0; k < cn; k++ )
                        D[k] = cval[k];
                    continue;
                }

                for( i = 0; i < 8; i++ )
                {
                    x[i] = borderInterpolate(sx + i, ssize.width, borderType1)*cn;
                    y[i] = borderInterpolate(sy + i, ssize.height, borderType1);
                }

                for( k = 0; k < cn; k++, S0++, w -= 64 )
                {
                    WT cv = cval[k], sum = cv*ONE;
                    for( i = 0; i < 8; i++, w += 8 )
                    {
                        int yi = y[i];
                        const T* S1 = S0 + yi*sstep;
                        if( yi < 0 )
                            continue;
                        if( x[0] >= 0 )
                            sum += (S1[x[0]] - cv)*w[0];
                        if( x[1] >= 0 )
                            sum += (S1[x[1]] - cv)*w[1];
                        if( x[2] >= 0 )
                            sum += (S1[x[2]] - cv)*w[2];
                        if( x[3] >= 0 )
                            sum += (S1[x[3]] - cv)*w[3];
                        if( x[4] >= 0 )
                            sum += (S1[x[4]] - cv)*w[4];
                        if( x[5] >= 0 )
                            sum += (S1[x[5]] - cv)*w[5];
                        if( x[6] >= 0 )
                            sum += (S1[x[6]] - cv)*w[6];
                        if( x[7] >= 0 )
                            sum += (S1[x[7]] - cv)*w[7];
                    }
                    D[k] = castOp(sum);
                }
                S0 -= cn;
            }
        }
    }
}


typedef void (*RemapNNFunc)(const Mat& _src, Mat& _dst, const Mat& _xy,
                            int borderType, const Scalar& _borderValue );

typedef void (*RemapFunc)(const Mat& _src, Mat& _dst, const Mat& _xy,
                          const Mat& _fxy, const void* _wtab,
                          int borderType, const Scalar& _borderValue);

class RemapInvoker :
    public ParallelLoopBody
{
public:
    RemapInvoker(const Mat& _src, Mat& _dst, const Mat *_m1,
                 const Mat *_m2, int _borderType, const Scalar &_borderValue,
                 int _planar_input, RemapNNFunc _nnfunc, RemapFunc _ifunc, const void *_ctab) :
        ParallelLoopBody(), src(&_src), dst(&_dst), m1(_m1), m2(_m2),
        borderType(_borderType), borderValue(_borderValue),
        planar_input(_planar_input), nnfunc(_nnfunc), ifunc(_ifunc), ctab(_ctab)
    {
    }

    virtual void operator() (const Range& range) const
    {
        int x, y, x1, y1;
        const int buf_size = 1 << 14;
        int brows0 = std::min(128, dst->rows), map_depth = m1->depth();
        int bcols0 = std::min(buf_size/brows0, dst->cols);
        brows0 = std::min(buf_size/bcols0, dst->rows);


        Mat _bufxy(brows0, bcols0, CV_16SC2), _bufa;
        if( !nnfunc )
            _bufa.create(brows0, bcols0, CV_16UC1);

        for( y = range.start; y < range.end; y += brows0 )
        {
            for( x = 0; x < dst->cols; x += bcols0 )
            {
                int brows = std::min(brows0, range.end - y);
                int bcols = std::min(bcols0, dst->cols - x);
                Mat dpart(*dst, Rect(x, y, bcols, brows));
                Mat bufxy(_bufxy, Rect(0, 0, bcols, brows));

                if( nnfunc )
                {
                    if( m1->type() == CV_16SC2 && !m2->data ) // the data is already in the right format
                        bufxy = (*m1)(Rect(x, y, bcols, brows));
                    else if( map_depth != CV_32F )
                    {
                        for( y1 = 0; y1 < brows; y1++ )
                        {
                            short* XY = (short*)(bufxy.data + bufxy.step*y1);
                            const short* sXY = (const short*)(m1->data + m1->step*(y+y1)) + x*2;
                            const ushort* sA = (const ushort*)(m2->data + m2->step*(y+y1)) + x;

                            for( x1 = 0; x1 < bcols; x1++ )
                            {
                                int a = sA[x1] & (INTER_TAB_SIZE2-1);
                                XY[x1*2] = sXY[x1*2] + NNDeltaTab_i[a][0];
                                XY[x1*2+1] = sXY[x1*2+1] + NNDeltaTab_i[a][1];
                            }
                        }
                    }
                    else if( !planar_input )
                        (*m1)(Rect(x, y, bcols, brows)).convertTo(bufxy, bufxy.depth());
                    else
                    {
                        for( y1 = 0; y1 < brows; y1++ )
                        {
                            short* XY = (short*)(bufxy.data + bufxy.step*y1);
                            const float* sX = (const float*)(m1->data + m1->step*(y+y1)) + x;
                            const float* sY = (const float*)(m2->data + m2->step*(y+y1)) + x;
                            x1 = 0;


                            for( ; x1 < bcols; x1++ )
                            {
                                XY[x1*2] = saturate_cast<short>(sX[x1]);
                                XY[x1*2+1] = saturate_cast<short>(sY[x1]);
                            }
                        }
                    }
                    nnfunc( *src, dpart, bufxy, borderType, borderValue );
                    continue;
                }

                Mat bufa(_bufa, Rect(0, 0, bcols, brows));
                for( y1 = 0; y1 < brows; y1++ )
                {
                    short* XY = (short*)(bufxy.data + bufxy.step*y1);
                    ushort* A = (ushort*)(bufa.data + bufa.step*y1);

                    if( m1->type() == CV_16SC2 && (m2->type() == CV_16UC1 || m2->type() == CV_16SC1) )
                    {
                        bufxy = (*m1)(Rect(x, y, bcols, brows));

                        const ushort* sA = (const ushort*)(m2->data + m2->step*(y+y1)) + x;
                        for( x1 = 0; x1 < bcols; x1++ )
                            A[x1] = (ushort)(sA[x1] & (INTER_TAB_SIZE2-1));
                    }
                    else if( planar_input )
                    {
                        const float* sX = (const float*)(m1->data + m1->step*(y+y1)) + x;
                        const float* sY = (const float*)(m2->data + m2->step*(y+y1)) + x;

                        x1 = 0;

                        for( ; x1 < bcols; x1++ )
                        {
                            int sx = cvRound(sX[x1]*INTER_TAB_SIZE);
                            int sy = cvRound(sY[x1]*INTER_TAB_SIZE);
                            int v = (sy & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE + (sx & (INTER_TAB_SIZE-1));
                            XY[x1*2] = saturate_cast<short>(sx >> INTER_BITS);
                            XY[x1*2+1] = saturate_cast<short>(sy >> INTER_BITS);
                            A[x1] = (ushort)v;
                        }
                    }
                    else
                    {
                        const float* sXY = (const float*)(m1->data + m1->step*(y+y1)) + x*2;

                        for( x1 = 0; x1 < bcols; x1++ )
                        {
                            int sx = cvRound(sXY[x1*2]*INTER_TAB_SIZE);
                            int sy = cvRound(sXY[x1*2+1]*INTER_TAB_SIZE);
                            int v = (sy & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE + (sx & (INTER_TAB_SIZE-1));
                            XY[x1*2] = saturate_cast<short>(sx >> INTER_BITS);
                            XY[x1*2+1] = saturate_cast<short>(sy >> INTER_BITS);
                            A[x1] = (ushort)v;
                        }
                    }
                }
                ifunc(*src, dpart, bufxy, bufa, ctab, borderType, borderValue);
            }
        }
    }

private:
    const Mat* src;
    Mat* dst;
    const Mat *m1, *m2;
    int borderType;
    Scalar borderValue;
    int planar_input;
    RemapNNFunc nnfunc;
    RemapFunc ifunc;
    const void *ctab;
};

}

void cv::remap( InputArray _src, OutputArray _dst,
                InputArray _map1, InputArray _map2,
                int interpolation, int borderType, const Scalar& borderValue )
{
    static RemapNNFunc nn_tab[] =
    {
        remapNearest<uchar>, remapNearest<schar>, remapNearest<ushort>, remapNearest<short>,
        remapNearest<int>, remapNearest<float>, remapNearest<double>, 0
    };

    static RemapFunc linear_tab[] =
    {
        remapBilinear<FixedPtCast<int, uchar, INTER_REMAP_COEF_BITS>, RemapVec_8u, short>, 0,
        remapBilinear<Cast<float, ushort>, RemapNoVec, float>,
        remapBilinear<Cast<float, short>, RemapNoVec, float>, 0,
        remapBilinear<Cast<float, float>, RemapNoVec, float>,
        remapBilinear<Cast<double, double>, RemapNoVec, float>, 0
    };

    static RemapFunc cubic_tab[] =
    {
        remapBicubic<FixedPtCast<int, uchar, INTER_REMAP_COEF_BITS>, short, INTER_REMAP_COEF_SCALE>, 0,
        remapBicubic<Cast<float, ushort>, float, 1>,
        remapBicubic<Cast<float, short>, float, 1>, 0,
        remapBicubic<Cast<float, float>, float, 1>,
        remapBicubic<Cast<double, double>, float, 1>, 0
    };

    static RemapFunc lanczos4_tab[] =
    {
        remapLanczos4<FixedPtCast<int, uchar, INTER_REMAP_COEF_BITS>, short, INTER_REMAP_COEF_SCALE>, 0,
        remapLanczos4<Cast<float, ushort>, float, 1>,
        remapLanczos4<Cast<float, short>, float, 1>, 0,
        remapLanczos4<Cast<float, float>, float, 1>,
        remapLanczos4<Cast<double, double>, float, 1>, 0
    };

    Mat src = _src.getMat(), map1 = _map1.getMat(), map2 = _map2.getMat();

    CV_Assert( map1.size().area() > 0 );
    CV_Assert( !map2.data || (map2.size() == map1.size()));

    _dst.create( map1.size(), src.type() );
    Mat dst = _dst.getMat();
    if( dst.data == src.data )
        src = src.clone();

    int depth = src.depth();
    RemapNNFunc nnfunc = 0;
    RemapFunc ifunc = 0;
    const void* ctab = 0;
    bool fixpt = depth == CV_8U;
    bool planar_input = false;

    if( interpolation == INTER_NEAREST )
    {
        nnfunc = nn_tab[depth];
        CV_Assert( nnfunc != 0 );
    }
    else
    {
        if( interpolation == INTER_AREA )
            interpolation = INTER_LINEAR;

        if( interpolation == INTER_LINEAR )
            ifunc = linear_tab[depth];
        else if( interpolation == INTER_CUBIC )
            ifunc = cubic_tab[depth];
        else if( interpolation == INTER_LANCZOS4 )
            ifunc = lanczos4_tab[depth];
        else
            CV_Error( CV_StsBadArg, "Unknown interpolation method" );
        CV_Assert( ifunc != 0 );
        ctab = initInterTab2D( interpolation, fixpt );
    }

    const Mat *m1 = &map1, *m2 = &map2;

    if( (map1.type() == CV_16SC2 && (map2.type() == CV_16UC1 || map2.type() == CV_16SC1 || !map2.data)) ||
        (map2.type() == CV_16SC2 && (map1.type() == CV_16UC1 || map1.type() == CV_16SC1 || !map1.data)) )
    {
        if( map1.type() != CV_16SC2 )
            std::swap(m1, m2);
    }
    else
    {
        CV_Assert( ((map1.type() == CV_32FC2 || map1.type() == CV_16SC2) && !map2.data) ||
            (map1.type() == CV_32FC1 && map2.type() == CV_32FC1) );
        planar_input = map1.channels() == 1;
    }

    RemapInvoker invoker(src, dst, m1, m2,
                         borderType, borderValue, planar_input, nnfunc, ifunc,
                         ctab);
    parallel_for_(Range(0, dst.rows), invoker, dst.total()/(double)(1<<16));
}



namespace cv
{

class warpPerspectiveInvoker :
    public ParallelLoopBody
{
public:
    warpPerspectiveInvoker(const Mat &_src, Mat &_dst, double *_M, int _interpolation,
                           int _borderType, const Scalar &_borderValue) :
        ParallelLoopBody(), src(_src), dst(_dst), M(_M), interpolation(_interpolation),
        borderType(_borderType), borderValue(_borderValue)
    {
    }

    virtual void operator() (const Range& range) const
    {
        const int BLOCK_SZ = 32;
        short XY[BLOCK_SZ*BLOCK_SZ*2], A[BLOCK_SZ*BLOCK_SZ];
        int x, y, x1, y1, width = dst.cols, height = dst.rows;

        int bh0 = std::min(BLOCK_SZ/2, height);
        int bw0 = std::min(BLOCK_SZ*BLOCK_SZ/bh0, width);
        bh0 = std::min(BLOCK_SZ*BLOCK_SZ/bw0, height);

        for( y = range.start; y < range.end; y += bh0 )
        {
            for( x = 0; x < width; x += bw0 )
            {
                int bw = std::min( bw0, width - x);
                int bh = std::min( bh0, range.end - y); // height

                Mat _XY(bh, bw, CV_16SC2, XY), matA;
                Mat dpart(dst, Rect(x, y, bw, bh));

                for( y1 = 0; y1 < bh; y1++ )
                {
                    short* xy = XY + y1*bw*2;
                    double X0 = M[0]*x + M[1]*(y + y1) + M[2];
                    double Y0 = M[3]*x + M[4]*(y + y1) + M[5];
                    double W0 = M[6]*x + M[7]*(y + y1) + M[8];

                    if( interpolation == INTER_NEAREST )
                        for( x1 = 0; x1 < bw; x1++ )
                        {
                            double W = W0 + M[6]*x1;
                            W = W ? 1./W : 0;
                            double fX = std::max((double)INT_MIN, std::min((double)INT_MAX, (X0 + M[0]*x1)*W));
                            double fY = std::max((double)INT_MIN, std::min((double)INT_MAX, (Y0 + M[3]*x1)*W));
                            int X = saturate_cast<int>(fX);
                            int Y = saturate_cast<int>(fY);

                            xy[x1*2] = saturate_cast<short>(X);
                            xy[x1*2+1] = saturate_cast<short>(Y);
                        }
                    else
                    {
                        short* alpha = A + y1*bw;
                        for( x1 = 0; x1 < bw; x1++ )
                        {
                            double W = W0 + M[6]*x1;
                            W = W ? INTER_TAB_SIZE/W : 0;
                            double fX = std::max((double)INT_MIN, std::min((double)INT_MAX, (X0 + M[0]*x1)*W));
                            double fY = std::max((double)INT_MIN, std::min((double)INT_MAX, (Y0 + M[3]*x1)*W));
                            int X = saturate_cast<int>(fX);
                            int Y = saturate_cast<int>(fY);

                            xy[x1*2] = saturate_cast<short>(X >> INTER_BITS);
                            xy[x1*2+1] = saturate_cast<short>(Y >> INTER_BITS);
                            alpha[x1] = (short)((Y & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE +
                                                (X & (INTER_TAB_SIZE-1)));
                        }
                    }
                }

                if( interpolation == INTER_NEAREST )
                    remap( src, dpart, _XY, Mat(), interpolation, borderType, borderValue );
                else
                {
                    Mat _matA(bh, bw, CV_16U, A);
                    remap( src, dpart, _XY, _matA, interpolation, borderType, borderValue );
                }
            }
        }
    }

private:
    Mat src;
    Mat dst;
    double* M;
    int interpolation, borderType;
    Scalar borderValue;
};
}

void cv::warpPerspective( InputArray _src, OutputArray _dst, InputArray _M0,
                          Size dsize, int flags, int borderType, const Scalar& borderValue )
{
    Mat src = _src.getMat(), M0 = _M0.getMat();
    _dst.create( dsize.area() == 0 ? src.size() : dsize, src.type() );
    Mat dst = _dst.getMat();

    CV_Assert( src.cols > 0 && src.rows > 0 );
    if( dst.data == src.data )
        src = src.clone();

    double M[9];
    Mat matM(3, 3, CV_64F, M);
    int interpolation = flags & INTER_MAX;
    if( interpolation == INTER_AREA )
        interpolation = INTER_LINEAR;

    CV_Assert( (M0.type() == CV_32F || M0.type() == CV_64F) && M0.rows == 3 && M0.cols == 3 );
    M0.convertTo(matM, matM.type());



    if( !(flags & WARP_INVERSE_MAP) )
    {
        assert("herelsp remove");
         //invert(matM, matM);
    }

    Range range(0, dst.rows);
    warpPerspectiveInvoker invoker(src, dst, M, interpolation, borderType, borderValue);
   parallel_for_(range, invoker, dst.total()/(double)(1<<16));
}




/* Calculates coefficients of perspective transformation
 * which maps (xi,yi) to (ui,vi), (i=1,2,3,4):
 *
 *      c00*xi + c01*yi + c02
 * ui = ---------------------
 *      c20*xi + c21*yi + c22
 *
 *      c10*xi + c11*yi + c12
 * vi = ---------------------
 *      c20*xi + c21*yi + c22
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 -x0*u0 -y0*u0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 -x1*u1 -y1*u1 | |c01| |u1|
 * | x2 y2  1  0  0  0 -x2*u2 -y2*u2 | |c02| |u2|
 * | x3 y3  1  0  0  0 -x3*u3 -y3*u3 |.|c10|=|u3|,
 * |  0  0  0 x0 y0  1 -x0*v0 -y0*v0 | |c11| |v0|
 * |  0  0  0 x1 y1  1 -x1*v1 -y1*v1 | |c12| |v1|
 * |  0  0  0 x2 y2  1 -x2*v2 -y2*v2 | |c20| |v2|
 * \  0  0  0 x3 y3  1 -x3*v3 -y3*v3 / \c21/ \v3/
 *
 * where:
 *   cij - matrix coefficients, c22 = 1
 */
cv::Mat cv::getPerspectiveTransform( const Point2f src[], const Point2f dst[] )
{
    Mat M(3, 3, CV_64F), X(8, 1, CV_64F, M.data);
    double a[8][8], b[8];
    Mat A(8, 8, CV_64F, a), B(8, 1, CV_64F, b);

    for( int i = 0; i < 4; ++i )
    {
        a[i][0] = a[i+4][3] = src[i].x;
        a[i][1] = a[i+4][4] = src[i].y;
        a[i][2] = a[i+4][5] = 1;
        a[i][3] = a[i][4] = a[i][5] =
        a[i+4][0] = a[i+4][1] = a[i+4][2] = 0;
        a[i][6] = -src[i].x*dst[i].x;
        a[i][7] = -src[i].y*dst[i].x;
        a[i+4][6] = -src[i].x*dst[i].y;
        a[i+4][7] = -src[i].y*dst[i].y;
        b[i] = dst[i].x;
        b[i+4] = dst[i].y;
    }

    solve( A, B, X, DECOMP_SVD );
    ((double*)M.data)[8] = 1.;

    return M;
}

/* Calculates coefficients of affine transformation
 * which maps (xi,yi) to (ui,vi), (i=1,2,3):
 *
 * ui = c00*xi + c01*yi + c02
 *
 * vi = c10*xi + c11*yi + c12
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 | |c01| |u1|
 * | x2 y2  1  0  0  0 | |c02| |u2|
 * |  0  0  0 x0 y0  1 | |c10| |v0|
 * |  0  0  0 x1 y1  1 | |c11| |v1|
 * \  0  0  0 x2 y2  1 / |c12| |v2|
 *
 * where:
 *   cij - matrix coefficients
 */

cv::Mat cv::getAffineTransform( const Point2f src[], const Point2f dst[] )
{
    Mat M(2, 3, CV_64F), X(6, 1, CV_64F, M.data);
    double a[6*6], b[6];
    Mat A(6, 6, CV_64F, a), B(6, 1, CV_64F, b);

    for( int i = 0; i < 3; i++ )
    {
        int j = i*12;
        int k = i*12+6;
        a[j] = a[k+3] = src[i].x;
        a[j+1] = a[k+4] = src[i].y;
        a[j+2] = a[k+5] = 1;
        a[j+3] = a[j+4] = a[j+5] = 0;
        a[k] = a[k+1] = a[k+2] = 0;
        b[i*2] = dst[i].x;
        b[i*2+1] = dst[i].y;
    }

    solve( A, B, X );
    return M;
}


cv::Mat cv::getPerspectiveTransform(InputArray _src, InputArray _dst)
{
    Mat src = _src.getMat(), dst = _dst.getMat();
    CV_Assert(src.checkVector(2, CV_32F) == 4 && dst.checkVector(2, CV_32F) == 4);
    return getPerspectiveTransform((const Point2f*)src.data, (const Point2f*)dst.data);
}

cv::Mat cv::getAffineTransform(InputArray _src, InputArray _dst)
{
    Mat src = _src.getMat(), dst = _dst.getMat();
    CV_Assert(src.checkVector(2, CV_32F) == 3 && dst.checkVector(2, CV_32F) == 3);
    return getAffineTransform((const Point2f*)src.data, (const Point2f*)dst.data);
}

CV_IMPL void
cvResize( const CvArr* srcarr, CvArr* dstarr, int method )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr);
    CV_Assert( src.type() == dst.type() );
    cv::resize( src, dst, dst.size(), (double)dst.cols/src.cols,
        (double)dst.rows/src.rows, method );
}



CV_IMPL void
cvWarpPerspective( const CvArr* srcarr, CvArr* dstarr, const CvMat* marr,
                   int flags, CvScalar fillval )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr);
    cv::Mat matrix = cv::cvarrToMat(marr);
    CV_Assert( src.type() == dst.type() );
    cv::warpPerspective( src, dst, matrix, dst.size(), flags,
        (flags & CV_WARP_FILL_OUTLIERS) ? cv::BORDER_CONSTANT : cv::BORDER_TRANSPARENT,
        fillval );
}





CV_IMPL CvMat*
cvGetPerspectiveTransform( const CvPoint2D32f* src,
                          const CvPoint2D32f* dst,
                          CvMat* matrix )
{
    cv::Mat M0 = cv::cvarrToMat(matrix),
        M = cv::getPerspectiveTransform((const cv::Point2f*)src, (const cv::Point2f*)dst);
    CV_Assert( M.size() == M0.size() );
    M.convertTo(M0, M0.type());
    return matrix;
}




/****************************************************************************************\
*                                   Log-Polar Transform                                  *
\****************************************************************************************/



/* End of file. */

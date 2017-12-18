

#include "imgproc.precomp.hpp"
#include <limits>
#include <iostream>

namespace cv
{
//constants for conversion from/to RGB and Gray, YUV, YCrCb according to BT.601
const float B2YF = 0.114f;
const float G2YF = 0.587f;
const float R2YF = 0.299f;
//to YCbCr
const float YCBF = 0.564f; // == 1/2/(1-B2YF)
const float YCRF = 0.713f; // == 1/2/(1-R2YF)
const int YCBI = 9241;  // == YCBF*16384
const int YCRI = 11682; // == YCRF*16384
//to YUV
const float B2UF = 0.492f;
const float R2VF = 0.877f;
const int B2UI = 8061;  // == B2UF*16384
const int R2VI = 14369; // == R2VF*16384
//from YUV
const float U2BF = 2.032f;
const float U2GF = -0.395f;
const float V2GF = -0.581f;
const float V2RF = 1.140f;
const int U2BI = 33292;
const int U2GI = -6472;
const int V2GI = -9519;
const int V2RI = 18678;
//from YCrCb
const float CR2RF = 1.403f;
const float CB2GF = -0.344f;
const float CR2GF = -0.714f;
const float CB2BF = 1.773f;
const int CR2RI = 22987;
const int CB2GI = -5636;
const int CR2GI = -11698;
const int CB2BI = 29049;



template<typename _Tp> struct ColorChannel
{
    typedef float worktype_f;
    static _Tp max() { return std::numeric_limits<_Tp>::max(); }
    static _Tp half() { return (_Tp)(max()/2 + 1); }
};

template<> struct ColorChannel<float>
{
    typedef float worktype_f;
    static float max() { return 1.f; }
    static float half() { return 0.5f; }
};

/*template<> struct ColorChannel<double>
{
    typedef double worktype_f;
    static double max() { return 1.; }
    static double half() { return 0.5; }
};*/


///////////////////////////// Top-level template function ////////////////////////////////

template <typename Cvt>
class CvtColorLoop_Invoker : public ParallelLoopBody
{
    typedef typename Cvt::channel_type _Tp;
public:

    CvtColorLoop_Invoker(const Mat& _src, Mat& _dst, const Cvt& _cvt) :
        ParallelLoopBody(), src(_src), dst(_dst), cvt(_cvt)
    {
    }

    virtual void operator()(const Range& range) const
    {
        const uchar* yS = src.ptr<uchar>(range.start);
        uchar* yD = dst.ptr<uchar>(range.start);

        for( int i = range.start; i < range.end; ++i, yS += src.step, yD += dst.step )
            cvt((const _Tp*)yS, (_Tp*)yD, src.cols);
    }

private:
    const Mat& src;
    Mat& dst;
    const Cvt& cvt;

    const CvtColorLoop_Invoker& operator= (const CvtColorLoop_Invoker&);
};

template <typename Cvt>
void CvtColorLoop(const Mat& src, Mat& dst, const Cvt& cvt)
{
    parallel_for_(Range(0, src.rows), CvtColorLoop_Invoker<Cvt>(src, dst, cvt), src.total()/(double)(1<<16) );
}


////////////////// Various 3/4-channel to 3/4-channel RGB transformations /////////////////

template<typename _Tp> struct RGB2RGB
{
    typedef _Tp channel_type;

    RGB2RGB(int _srccn, int _dstcn, int _blueIdx) : srccn(_srccn), dstcn(_dstcn), blueIdx(_blueIdx) {}
    void operator()(const _Tp* src, _Tp* dst, int n) const
    {
        int scn = srccn, dcn = dstcn, bidx = blueIdx;
        if( dcn == 3 )
        {
            n *= 3;
            for( int i = 0; i < n; i += 3, src += scn )
            {
                _Tp t0 = src[bidx], t1 = src[1], t2 = src[bidx ^ 2];
                dst[i] = t0; dst[i+1] = t1; dst[i+2] = t2;
            }
        }
        else if( scn == 3 )
        {
            n *= 3;
            _Tp alpha = ColorChannel<_Tp>::max();
            for( int i = 0; i < n; i += 3, dst += 4 )
            {
                _Tp t0 = src[i], t1 = src[i+1], t2 = src[i+2];
                dst[bidx] = t0; dst[1] = t1; dst[bidx^2] = t2; dst[3] = alpha;
            }
        }
        else
        {
            n *= 4;
            for( int i = 0; i < n; i += 4 )
            {
                _Tp t0 = src[i], t1 = src[i+1], t2 = src[i+2], t3 = src[i+3];
                dst[i] = t2; dst[i+1] = t1; dst[i+2] = t0; dst[i+3] = t3;
            }
        }
    }

    int srccn, dstcn, blueIdx;
};

/////////// Transforming 16-bit (565 or 555) RGB to/from 24/32-bit (888[8]) RGB //////////

struct RGB5x52RGB
{
    typedef uchar channel_type;

    RGB5x52RGB(int _dstcn, int _blueIdx, int _greenBits)
        : dstcn(_dstcn), blueIdx(_blueIdx), greenBits(_greenBits) {}

    void operator()(const uchar* src, uchar* dst, int n) const
    {
        int dcn = dstcn, bidx = blueIdx;
        if( greenBits == 6 )
            for( int i = 0; i < n; i++, dst += dcn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[bidx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 3) & ~3);
                dst[bidx ^ 2] = (uchar)((t >> 8) & ~7);
                if( dcn == 4 )
                    dst[3] = 255;
            }
        else
            for( int i = 0; i < n; i++, dst += dcn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[bidx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 2) & ~7);
                dst[bidx ^ 2] = (uchar)((t >> 7) & ~7);
                if( dcn == 4 )
                    dst[3] = t & 0x8000 ? 255 : 0;
            }
    }

    int dstcn, blueIdx, greenBits;
};


struct RGB2RGB5x5
{
    typedef uchar channel_type;

    RGB2RGB5x5(int _srccn, int _blueIdx, int _greenBits)
        : srccn(_srccn), blueIdx(_blueIdx), greenBits(_greenBits) {}

    void operator()(const uchar* src, uchar* dst, int n) const
    {
        int scn = srccn, bidx = blueIdx;
        if( greenBits == 6 )
            for( int i = 0; i < n; i++, src += scn )
            {
                ((ushort*)dst)[i] = (ushort)((src[bidx] >> 3)|((src[1]&~3) << 3)|((src[bidx^2]&~7) << 8));
            }
        else if( scn == 3 )
            for( int i = 0; i < n; i++, src += 3 )
            {
                ((ushort*)dst)[i] = (ushort)((src[bidx] >> 3)|((src[1]&~7) << 2)|((src[bidx^2]&~7) << 7));
            }
        else
            for( int i = 0; i < n; i++, src += 4 )
            {
                ((ushort*)dst)[i] = (ushort)((src[bidx] >> 3)|((src[1]&~7) << 2)|
                    ((src[bidx^2]&~7) << 7)|(src[3] ? 0x8000 : 0));
            }
    }

    int srccn, blueIdx, greenBits;
};

///////////////////////////////// Color to/from Grayscale ////////////////////////////////

template<typename _Tp>
struct Gray2RGB
{
    typedef _Tp channel_type;

    Gray2RGB(int _dstcn) : dstcn(_dstcn) {}
    void operator()(const _Tp* src, _Tp* dst, int n) const
    {
        if( dstcn == 3 )
            for( int i = 0; i < n; i++, dst += 3 )
            {
                dst[0] = dst[1] = dst[2] = src[i];
            }
        else
        {
            _Tp alpha = ColorChannel<_Tp>::max();
            for( int i = 0; i < n; i++, dst += 4 )
            {
                dst[0] = dst[1] = dst[2] = src[i];
                dst[3] = alpha;
            }
        }
    }

    int dstcn;
};


struct Gray2RGB5x5
{
    typedef uchar channel_type;

    Gray2RGB5x5(int _greenBits) : greenBits(_greenBits) {}
    void operator()(const uchar* src, uchar* dst, int n) const
    {
        if( greenBits == 6 )
            for( int i = 0; i < n; i++ )
            {
                int t = src[i];
                ((ushort*)dst)[i] = (ushort)((t >> 3)|((t & ~3) << 3)|((t & ~7) << 8));
            }
        else
            for( int i = 0; i < n; i++ )
            {
                int t = src[i] >> 3;
                ((ushort*)dst)[i] = (ushort)(t|(t << 5)|(t << 10));
            }
    }
    int greenBits;
};


#undef R2Y
#undef G2Y
#undef B2Y

enum
{
    yuv_shift = 14,
    xyz_shift = 12,
    R2Y = 4899, // B2YF*16384
    G2Y = 9617, // G2YF*16384
    B2Y = 1868, // B2YF*16384
    BLOCK_SIZE = 256
};


struct RGB5x52Gray
{
    typedef uchar channel_type;

    RGB5x52Gray(int _greenBits) : greenBits(_greenBits) {}
    void operator()(const uchar* src, uchar* dst, int n) const
    {
        if( greenBits == 6 )
            for( int i = 0; i < n; i++ )
            {
                int t = ((ushort*)src)[i];
                dst[i] = (uchar)CV_DESCALE(((t << 3) & 0xf8)*B2Y +
                                           ((t >> 3) & 0xfc)*G2Y +
                                           ((t >> 8) & 0xf8)*R2Y, yuv_shift);
            }
        else
            for( int i = 0; i < n; i++ )
            {
                int t = ((ushort*)src)[i];
                dst[i] = (uchar)CV_DESCALE(((t << 3) & 0xf8)*B2Y +
                                           ((t >> 2) & 0xf8)*G2Y +
                                           ((t >> 7) & 0xf8)*R2Y, yuv_shift);
            }
    }
    int greenBits;
};


template<typename _Tp> struct RGB2Gray
{
    typedef _Tp channel_type;

    RGB2Gray(int _srccn, int blueIdx, const float* _coeffs) : srccn(_srccn)
    {
        static const float coeffs0[] = { R2YF, G2YF, B2YF };
        memcpy( coeffs, _coeffs ? _coeffs : coeffs0, 3*sizeof(coeffs[0]) );
        if(blueIdx == 0)
            std::swap(coeffs[0], coeffs[2]);
    }

    void operator()(const _Tp* src, _Tp* dst, int n) const
    {
        int scn = srccn;
        float cb = coeffs[0], cg = coeffs[1], cr = coeffs[2];
        for(int i = 0; i < n; i++, src += scn)
            dst[i] = saturate_cast<_Tp>(src[0]*cb + src[1]*cg + src[2]*cr);
    }
    int srccn;
    float coeffs[3];
};


template<> struct RGB2Gray<uchar>
{
    typedef uchar channel_type;

    RGB2Gray(int _srccn, int blueIdx, const int* coeffs) : srccn(_srccn)
    {
        const int coeffs0[] = { R2Y, G2Y, B2Y };
        if(!coeffs) coeffs = coeffs0;

        int b = 0, g = 0, r = (1 << (yuv_shift-1));
        int db = coeffs[blueIdx^2], dg = coeffs[1], dr = coeffs[blueIdx];

        for( int i = 0; i < 256; i++, b += db, g += dg, r += dr )
        {
            tab[i] = b;
            tab[i+256] = g;
            tab[i+512] = r;
        }
    }
    void operator()(const uchar* src, uchar* dst, int n) const
    {
        int scn = srccn;
        const int* _tab = tab;
        for(int i = 0; i < n; i++, src += scn)
            dst[i] = (uchar)((_tab[src[0]] + _tab[src[1]+256] + _tab[src[2]+512]) >> yuv_shift);
    }
    int srccn;
    int tab[256*3];
};


template<> struct RGB2Gray<ushort>
{
    typedef ushort channel_type;

    RGB2Gray(int _srccn, int blueIdx, const int* _coeffs) : srccn(_srccn)
    {
        static const int coeffs0[] = { R2Y, G2Y, B2Y };
        memcpy(coeffs, _coeffs ? _coeffs : coeffs0, 3*sizeof(coeffs[0]));
        if( blueIdx == 0 )
            std::swap(coeffs[0], coeffs[2]);
    }

    void operator()(const ushort* src, ushort* dst, int n) const
    {
        int scn = srccn, cb = coeffs[0], cg = coeffs[1], cr = coeffs[2];
        for(int i = 0; i < n; i++, src += scn)
            dst[i] = (ushort)CV_DESCALE((unsigned)(src[0]*cb + src[1]*cg + src[2]*cr), yuv_shift);
    }
    int srccn;
    int coeffs[3];
};




}//namespace cv

//////////////////////////////////////////////////////////////////////////////////////////
//                                   The main function                                  //
//////////////////////////////////////////////////////////////////////////////////////////

void cv::cvtColor( InputArray _src, OutputArray _dst, int code, int dcn )
{
    Mat src = _src.getMat(), dst;
    Size sz = src.size();
    int scn = src.channels(), depth = src.depth(), bidx;

    CV_Assert( depth == CV_8U || depth == CV_16U || depth == CV_32F );

    switch( code )
    {

        case CV_BGR2GRAY: case CV_BGRA2GRAY: case CV_RGB2GRAY: case CV_RGBA2GRAY:
            CV_Assert( scn == 3 || scn == 4 );
            _dst.create(sz, CV_MAKETYPE(depth, 1));
            dst = _dst.getMat();

            bidx = code == CV_BGR2GRAY || code == CV_BGRA2GRAY ? 0 : 2;

            if( depth == CV_8U )
            {

                CvtColorLoop(src, dst, RGB2Gray<uchar>(scn, bidx, 0));
            }
            else if( depth == CV_16U )
                CvtColorLoop(src, dst, RGB2Gray<ushort>(scn, bidx, 0));
            else
                CvtColorLoop(src, dst, RGB2Gray<float>(scn, bidx, 0));
            break;



        case CV_GRAY2BGR: case CV_GRAY2BGRA:
            if( dcn <= 0 ) dcn = (code==CV_GRAY2BGRA) ? 4 : 3;
            CV_Assert( scn == 1 && (dcn == 3 || dcn == 4));
            _dst.create(sz, CV_MAKETYPE(depth, dcn));
            dst = _dst.getMat();

            if( depth == CV_8U )
            {

                CvtColorLoop(src, dst, Gray2RGB<uchar>(dcn));
            }
            else if( depth == CV_16U )
                CvtColorLoop(src, dst, Gray2RGB<ushort>(dcn));
            else
                CvtColorLoop(src, dst, Gray2RGB<float>(dcn));
            break;




        default:
            CV_Error( CV_StsBadFlag, "Unknown/unsupported color conversion code" );
    }
}

CV_IMPL void
cvCvtColor( const CvArr* srcarr, CvArr* dstarr, int code )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst0 = cv::cvarrToMat(dstarr), dst = dst0;
    CV_Assert( src.depth() == dst.depth() );

    cv::cvtColor(src, dst, code, dst.channels());
    CV_Assert( dst.data == dst0.data );
}


/* End of file. */

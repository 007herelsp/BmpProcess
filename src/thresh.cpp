

#include "imgproc.precomp.hpp"
#include <opencv2/imgproc/imgproc.hpp>
namespace cv
{

static void
thresh_8u( const Mat& _src, Mat& _dst, uchar thresh, uchar maxval, int type )
{
    int i, j, j_scalar = 0;
    uchar tab[256];
    Size roi = _src.size();
    roi.width *= _src.channels();

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }



    switch( type )
    {
    case THRESH_BINARY:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = maxval;
        break;
    case THRESH_BINARY_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = maxval;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    case THRESH_TRUNC:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = thresh;
        break;
    case THRESH_TOZERO:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = (uchar)i;
        break;
    case THRESH_TOZERO_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    default:
        CV_Error( CV_StsBadArg, "Unknown threshold type" );
    }

#if CV_SSE2
    if( checkHardwareSupport(CV_CPU_SSE2) )
    {
        __m128i _x80 = _mm_set1_epi8('\x80');
        __m128i thresh_u = _mm_set1_epi8(thresh);
        __m128i thresh_s = _mm_set1_epi8(thresh ^ 0x80);
        __m128i maxval_ = _mm_set1_epi8(maxval);
        j_scalar = roi.width & -8;

        for( i = 0; i < roi.height; i++ )
        {
            const uchar* src = (const uchar*)(_src.data + _src.step*i);
            uchar* dst = (uchar*)(_dst.data + _dst.step*i);

            switch( type )
            {
            case THRESH_BINARY:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v1 = _mm_cmpgt_epi8( _mm_xor_si128(v1, _x80), thresh_s );
                    v0 = _mm_and_si128( v0, maxval_ );
                    v1 = _mm_and_si128( v1, maxval_ );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v0 = _mm_and_si128( v0, maxval_ );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_BINARY_INV:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v1 = _mm_cmpgt_epi8( _mm_xor_si128(v1, _x80), thresh_s );
                    v0 = _mm_andnot_si128( v0, maxval_ );
                    v1 = _mm_andnot_si128( v1, maxval_ );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v0 = _mm_andnot_si128( v0, maxval_ );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TRUNC:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_subs_epu8( v0, _mm_subs_epu8( v0, thresh_u ));
                    v1 = _mm_subs_epu8( v1, _mm_subs_epu8( v1, thresh_u ));
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_subs_epu8( v0, _mm_subs_epu8( v0, thresh_u ));
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TOZERO:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_and_si128( v0, _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ));
                    v1 = _mm_and_si128( v1, _mm_cmpgt_epi8(_mm_xor_si128(v1, _x80), thresh_s ));
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_and_si128( v0, _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ));
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TOZERO_INV:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ), v0 );
                    v1 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v1, _x80), thresh_s ), v1 );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ), v0 );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;
            }
        }
    }
#endif

    if( j_scalar < roi.width )
    {
        for( i = 0; i < roi.height; i++ )
        {
            const uchar* src = (const uchar*)(_src.data + _src.step*i);
            uchar* dst = (uchar*)(_dst.data + _dst.step*i);
            j = j_scalar;
#if CV_ENABLE_UNROLLED
            for( ; j <= roi.width - 4; j += 4 )
            {
                uchar t0 = tab[src[j]];
                uchar t1 = tab[src[j+1]];

                dst[j] = t0;
                dst[j+1] = t1;

                t0 = tab[src[j+2]];
                t1 = tab[src[j+3]];

                dst[j+2] = t0;
                dst[j+3] = t1;
            }
#endif
            for( ; j < roi.width; j++ )
                dst[j] = tab[src[j]];
        }
    }
}


static void
thresh_16s( const Mat& _src, Mat& _dst, short thresh, short maxval, int type )
{
    int i, j;
    Size roi = _src.size();
    roi.width *= _src.channels();
    const short* src = (const short*)_src.data;
    short* dst = (short*)_dst.data;
    size_t src_step = _src.step/sizeof(src[0]);
    size_t dst_step = _dst.step/sizeof(dst[0]);

#if CV_SSE2
    volatile bool useSIMD = checkHardwareSupport(CV_CPU_SSE);
#endif

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }



    switch( type )
    {
    case THRESH_BINARY:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh), maxval8 = _mm_set1_epi16(maxval);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_cmpgt_epi16( v0, thresh8 );
                    v1 = _mm_cmpgt_epi16( v1, thresh8 );
                    v0 = _mm_and_si128( v0, maxval8 );
                    v1 = _mm_and_si128( v1, maxval8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = src[j] > thresh ? maxval : 0;
        }
        break;

    case THRESH_BINARY_INV:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh), maxval8 = _mm_set1_epi16(maxval);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_cmpgt_epi16( v0, thresh8 );
                    v1 = _mm_cmpgt_epi16( v1, thresh8 );
                    v0 = _mm_andnot_si128( v0, maxval8 );
                    v1 = _mm_andnot_si128( v1, maxval8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = src[j] <= thresh ? maxval : 0;
        }
        break;

    case THRESH_TRUNC:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_min_epi16( v0, thresh8 );
                    v1 = _mm_min_epi16( v1, thresh8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = std::min(src[j], thresh);
        }
        break;

    case THRESH_TOZERO:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_and_si128(v0, _mm_cmpgt_epi16(v0, thresh8));
                    v1 = _mm_and_si128(v1, _mm_cmpgt_epi16(v1, thresh8));
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
            {
                short v = src[j];
                dst[j] = v > thresh ? v : 0;
            }
        }
        break;

    case THRESH_TOZERO_INV:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_andnot_si128(_mm_cmpgt_epi16(v0, thresh8), v0);
                    v1 = _mm_andnot_si128(_mm_cmpgt_epi16(v1, thresh8), v1);
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif
            for( ; j < roi.width; j++ )
            {
                short v = src[j];
                dst[j] = v <= thresh ? v : 0;
            }
        }
        break;
    default:
        return CV_Error( CV_StsBadArg, "" );
    }
}


static void
thresh_32f( const Mat& _src, Mat& _dst, float thresh, float maxval, int type )
{
    int i, j;
    Size roi = _src.size();
    roi.width *= _src.channels();
    const float* src = (const float*)_src.data;
    float* dst = (float*)_dst.data;
    size_t src_step = _src.step/sizeof(src[0]);
    size_t dst_step = _dst.step/sizeof(dst[0]);

#if CV_SSE2
    volatile bool useSIMD = checkHardwareSupport(CV_CPU_SSE);
#endif

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }



    switch( type )
    {
        case THRESH_BINARY:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh), maxval4 = _mm_set1_ps(maxval);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_cmpgt_ps( v0, thresh4 );
                        v1 = _mm_cmpgt_ps( v1, thresh4 );
                        v0 = _mm_and_ps( v0, maxval4 );
                        v1 = _mm_and_ps( v1, maxval4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = src[j] > thresh ? maxval : 0;
            }
            break;

        case THRESH_BINARY_INV:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh), maxval4 = _mm_set1_ps(maxval);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_cmple_ps( v0, thresh4 );
                        v1 = _mm_cmple_ps( v1, thresh4 );
                        v0 = _mm_and_ps( v0, maxval4 );
                        v1 = _mm_and_ps( v1, maxval4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = src[j] <= thresh ? maxval : 0;
            }
            break;

        case THRESH_TRUNC:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_min_ps( v0, thresh4 );
                        v1 = _mm_min_ps( v1, thresh4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = std::min(src[j], thresh);
            }
            break;

        case THRESH_TOZERO:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_and_ps(v0, _mm_cmpgt_ps(v0, thresh4));
                        v1 = _mm_and_ps(v1, _mm_cmpgt_ps(v1, thresh4));
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                {
                    float v = src[j];
                    dst[j] = v > thresh ? v : 0;
                }
            }
            break;

        case THRESH_TOZERO_INV:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_and_ps(v0, _mm_cmple_ps(v0, thresh4));
                        v1 = _mm_and_ps(v1, _mm_cmple_ps(v1, thresh4));
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif
                for( ; j < roi.width; j++ )
                {
                    float v = src[j];
                    dst[j] = v <= thresh ? v : 0;
                }
            }
            break;
        default:
            return CV_Error( CV_StsBadArg, "" );
    }
}


static double
getThreshVal_Otsu_8u( const Mat& _src )
{
    Size size = _src.size();
    if( _src.isContinuous() )
    {
        size.width *= size.height;
        size.height = 1;
    }
    const int N = 256;
    int i, j, h[N] = {0};
    for( i = 0; i < size.height; i++ )
    {
        const uchar* src = _src.data + _src.step*i;
        j = 0;
        #if CV_ENABLE_UNROLLED
        for( ; j <= size.width - 4; j += 4 )
        {
            int v0 = src[j], v1 = src[j+1];
            h[v0]++; h[v1]++;
            v0 = src[j+2]; v1 = src[j+3];
            h[v0]++; h[v1]++;
        }
        #endif
        for( ; j < size.width; j++ )
            h[src[j]]++;
    }

    double mu = 0, scale = 1./(size.width*size.height);
    for( i = 0; i < N; i++ )
        mu += i*(double)h[i];

    mu *= scale;
    double mu1 = 0, q1 = 0;
    double max_sigma = 0, max_val = 0;

    for( i = 0; i < N; i++ )
    {
        double p_i, q2, mu2, sigma;

        p_i = h[i]*scale;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if( std::min(q1,q2) < FLT_EPSILON || std::max(q1,q2) > 1. - FLT_EPSILON )
            continue;

        mu1 = (mu1 + i*p_i)/q1;
        mu2 = (mu - q1*mu1)/q2;
        sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
        if( sigma > max_sigma )
        {
            max_sigma = sigma;
            max_val = i;
        }
    }

    return max_val;
}

class ThresholdRunner : public ParallelLoopBody
{
public:
    ThresholdRunner(Mat _src, Mat _dst, double _thresh, double _maxval, int _thresholdType)
    {
        src = _src;
        dst = _dst;

        thresh = _thresh;
        maxval = _maxval;
        thresholdType = _thresholdType;
    }

    void operator () ( const Range& range ) const
    {
        int row0 = range.start;
        int row1 = range.end;

        Mat srcStripe = src.rowRange(row0, row1);
        Mat dstStripe = dst.rowRange(row0, row1);

        if (srcStripe.depth() == CV_8U)
        {
            thresh_8u( srcStripe, dstStripe, (uchar)thresh, (uchar)maxval, thresholdType );
        }
        else if( srcStripe.depth() == CV_16S )
        {
            thresh_16s( srcStripe, dstStripe, (short)thresh, (short)maxval, thresholdType );
        }
        else if( srcStripe.depth() == CV_32F )
        {
            thresh_32f( srcStripe, dstStripe, (float)thresh, (float)maxval, thresholdType );
        }
    }

private:
    Mat src;
    Mat dst;

    double thresh;
    double maxval;
    int thresholdType;
};

}

double cv::threshold( InputArray _src, OutputArray _dst, double thresh, double maxval, int type )
{
    Mat src = _src.getMat();
    bool use_otsu = (type & THRESH_OTSU) != 0;
    type &= THRESH_MASK;

    if( use_otsu )
    {
        CV_Assert( src.type() == CV_8UC1 );
        thresh = getThreshVal_Otsu_8u(src);
    }

    _dst.create( src.size(), src.type() );
    Mat dst = _dst.getMat();

    if( src.depth() == CV_8U )
    {
        int ithresh = cvFloor(thresh);
        thresh = ithresh;
        int imaxval = cvRound(maxval);
        if( type == THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = saturate_cast<uchar>(imaxval);

        if( ithresh < 0 || ithresh >= 255 )
        {
            if( type == THRESH_BINARY || type == THRESH_BINARY_INV ||
                ((type == THRESH_TRUNC || type == THRESH_TOZERO_INV) && ithresh < 0) ||
                (type == THRESH_TOZERO && ithresh >= 255) )
            {
                int v = type == THRESH_BINARY ? (ithresh >= 255 ? 0 : imaxval) :
                        type == THRESH_BINARY_INV ? (ithresh >= 255 ? imaxval : 0) :
                        /*type == THRESH_TRUNC ? imaxval :*/ 0;
                dst.setTo(v);
            }
            else
                src.copyTo(dst);
            return thresh;
        }
        thresh = ithresh;
        maxval = imaxval;
    }
    else if( src.depth() == CV_16S )
    {
        int ithresh = cvFloor(thresh);
        thresh = ithresh;
        int imaxval = cvRound(maxval);
        if( type == THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = saturate_cast<short>(imaxval);

        if( ithresh < SHRT_MIN || ithresh >= SHRT_MAX )
        {
            if( type == THRESH_BINARY || type == THRESH_BINARY_INV ||
               ((type == THRESH_TRUNC || type == THRESH_TOZERO_INV) && ithresh < SHRT_MIN) ||
               (type == THRESH_TOZERO && ithresh >= SHRT_MAX) )
            {
                int v = type == THRESH_BINARY ? (ithresh >= SHRT_MAX ? 0 : imaxval) :
                type == THRESH_BINARY_INV ? (ithresh >= SHRT_MAX ? imaxval : 0) :
                /*type == THRESH_TRUNC ? imaxval :*/ 0;
                dst.setTo(v);
            }
            else
                src.copyTo(dst);
            return thresh;
        }
        thresh = ithresh;
        maxval = imaxval;
    }
    else if( src.depth() == CV_32F )
        ;
    else
        CV_Error( CV_StsUnsupportedFormat, "" );

    parallel_for_(Range(0, dst.rows),
                  ThresholdRunner(src, dst, thresh, maxval, type),
                  dst.total()/(double)(1<<16));
    return thresh;
}



CV_IMPL double
cvThreshold( const void* srcarr, void* dstarr, double thresh, double maxval, int type )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr), dst0 = dst;

    CV_Assert( src.size == dst.size && src.channels() == dst.channels() &&
        (src.depth() == dst.depth() || dst.depth() == CV_8U));

    thresh = cv::threshold( src, dst, thresh, maxval, type );
    if( dst0.data != dst.data )
        dst.convertTo( dst0, dst0.depth() );
    return thresh;
}




/* End of file. */

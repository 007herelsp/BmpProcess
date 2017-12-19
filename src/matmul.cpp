
#include "core.precomp.hpp"

/****************************************************************************************\
*                                        Transform                                       *
\****************************************************************************************/

namespace cv
{

template<typename T, typename WT> static void
transform_( const T* src, T* dst, const WT* m, int len, int scn, int dcn )
{
    int x;

    if( scn == 2 && dcn == 2 )
    {
        for( x = 0; x < len*2; x += 2 )
        {
            WT v0 = src[x], v1 = src[x+1];
            T t0 = saturate_cast<T>(m[0]*v0 + m[1]*v1 + m[2]);
            T t1 = saturate_cast<T>(m[3]*v0 + m[4]*v1 + m[5]);
            dst[x] = t0; dst[x+1] = t1;
        }
    }
    else if( scn == 3 && dcn == 3 )
    {
        for( x = 0; x < len*3; x += 3 )
        {
            WT v0 = src[x], v1 = src[x+1], v2 = src[x+2];
            T t0 = saturate_cast<T>(m[0]*v0 + m[1]*v1 + m[2]*v2 + m[3]);
            T t1 = saturate_cast<T>(m[4]*v0 + m[5]*v1 + m[6]*v2 + m[7]);
            T t2 = saturate_cast<T>(m[8]*v0 + m[9]*v1 + m[10]*v2 + m[11]);
            dst[x] = t0; dst[x+1] = t1; dst[x+2] = t2;
        }
    }
    else if( scn == 3 && dcn == 1 )
    {
        for( x = 0; x < len; x++, src += 3 )
            dst[x] = saturate_cast<T>(m[0]*src[0] + m[1]*src[1] + m[2]*src[2] + m[3]);
    }
    else if( scn == 4 && dcn == 4 )
    {
        for( x = 0; x < len*4; x += 4 )
        {
            WT v0 = src[x], v1 = src[x+1], v2 = src[x+2], v3 = src[x+3];
            T t0 = saturate_cast<T>(m[0]*v0 + m[1]*v1 + m[2]*v2 + m[3]*v3 + m[4]);
            T t1 = saturate_cast<T>(m[5]*v0 + m[6]*v1 + m[7]*v2 + m[8]*v3 + m[9]);
            dst[x] = t0; dst[x+1] = t1;
            t0 = saturate_cast<T>(m[10]*v0 + m[11]*v1 + m[12]*v2 + m[13]*v3 + m[14]);
            t1 = saturate_cast<T>(m[15]*v0 + m[16]*v1 + m[17]*v2 + m[18]*v3 + m[19]);
            dst[x+2] = t0; dst[x+3] = t1;
        }
    }
    else
    {
        for( x = 0; x < len; x++, src += scn, dst += dcn )
        {
            const WT* _m = m;
            int j, k;
            for( j = 0; j < dcn; j++, _m += scn + 1 )
            {
                WT s = _m[scn];
                for( k = 0; k < scn; k++ )
                    s += _m[k]*src[k];
                dst[j] = saturate_cast<T>(s);
            }
        }
    }
}



typedef void (*TransformFunc)( const uchar* src, uchar* dst, const uchar* m, int, int, int );



}

/****************************************************************************************\
*                                  Perspective Transform                                 *
\****************************************************************************************/

namespace cv
{

template<typename T> static void
perspectiveTransform_( const T* src, T* dst, const double* m, int len, int scn, int dcn )
{
    const double eps = FLT_EPSILON;
    int i;

    if( scn == 2 && dcn == 2 )
    {
        for( i = 0; i < len*2; i += 2 )
        {
            T x = src[i], y = src[i + 1];
            double w = x*m[6] + y*m[7] + m[8];

            if( fabs(w) > eps )
            {
                w = 1./w;
                dst[i] = (T)((x*m[0] + y*m[1] + m[2])*w);
                dst[i+1] = (T)((x*m[3] + y*m[4] + m[5])*w);
            }
            else
                dst[i] = dst[i+1] = (T)0;
        }
    }
    else if( scn == 3 && dcn == 3 )
    {
        for( i = 0; i < len*3; i += 3 )
        {
            T x = src[i], y = src[i + 1], z = src[i + 2];
            double w = x*m[12] + y*m[13] + z*m[14] + m[15];

            if( fabs(w) > eps )
            {
                w = 1./w;
                dst[i] = (T)((x*m[0] + y*m[1] + z*m[2] + m[3]) * w);
                dst[i+1] = (T)((x*m[4] + y*m[5] + z*m[6] + m[7]) * w);
                dst[i+2] = (T)((x*m[8] + y*m[9] + z*m[10] + m[11]) * w);
            }
            else
                dst[i] = dst[i+1] = dst[i+2] = (T)0;
        }
    }
    else if( scn == 3 && dcn == 2 )
    {
        for( i = 0; i < len; i++, src += 3, dst += 2 )
        {
            T x = src[0], y = src[1], z = src[2];
            double w = x*m[8] + y*m[9] + z*m[10] + m[11];

            if( fabs(w) > eps )
            {
                w = 1./w;
                dst[0] = (T)((x*m[0] + y*m[1] + z*m[2] + m[3])*w);
                dst[1] = (T)((x*m[4] + y*m[5] + z*m[6] + m[7])*w);
            }
            else
                dst[0] = dst[1] = (T)0;
        }
    }
    else
    {
        for( i = 0; i < len; i++, src += scn, dst += dcn )
        {
            const double* _m = m + dcn*(scn + 1);
            double w = _m[scn];
            int j, k;
            for( k = 0; k < scn; k++ )
                w += _m[k]*src[k];
            if( fabs(w) > eps )
            {
                _m = m;
                for( j = 0; j < dcn; j++, _m += scn + 1 )
                {
                    double s = _m[scn];
                    for( k = 0; k < scn; k++ )
                        s += _m[k]*src[k];
                    dst[j] = (T)(s*w);
                }
            }
            else
                for( j = 0; j < dcn; j++ )
                    dst[j] = 0;
        }
    }
}


static void
perspectiveTransform_32f(const float* src, float* dst, const double* m, int len, int scn, int dcn)
{
    perspectiveTransform_(src, dst, m, len, scn, dcn);
}

static void
perspectiveTransform_64f(const double* src, double* dst, const double* m, int len, int scn, int dcn)
{
    perspectiveTransform_(src, dst, m, len, scn, dcn);
}

}

void cv::perspectiveTransform( InputArray _src, OutputArray _dst, InputArray _mtx )
{
    Mat src = _src.getMat(), m = _mtx.getMat();
    int depth = src.depth(), scn = src.channels(), dcn = m.rows-1;
    CV_Assert( scn + 1 == m.cols && (depth == CV_32F || depth == CV_64F));

    _dst.create( src.size(), CV_MAKETYPE(depth, dcn) );
    Mat dst = _dst.getMat();

    const int mtype = CV_64F;
    AutoBuffer<double> _mbuf;
    double* mbuf = _mbuf;

    if( !m.isContinuous() || m.type() != mtype )
    {
        _mbuf.allocate((dcn+1)*(scn+1));
        Mat tmp(dcn+1, scn+1, mtype, (double*)_mbuf);
        m.convertTo(tmp, mtype);
        m = tmp;
    }
    else
        mbuf = (double*)m.data;

    TransformFunc func = depth == CV_32F ?
        (TransformFunc)perspectiveTransform_32f :
        (TransformFunc)perspectiveTransform_64f;
    CV_Assert( func != 0 );

    const Mat* arrays[] = {&src, &dst, 0};
    uchar* ptrs[2];
    NAryMatIterator it(arrays, ptrs);
    size_t i, total = it.size;

    for( i = 0; i < it.nplanes; i++, ++it )
        func( ptrs[0], ptrs[1], (uchar*)mbuf, (int)total, scn, dcn );
}


/****************************************************************************************\
*                                        MulTransposed                                   *
\****************************************************************************************/

namespace cv
{

template<typename sT, typename dT> static void
MulTransposedR( const Mat& srcmat, Mat& dstmat, const Mat& deltamat, double scale )
{
    int i, j, k;
    const sT* src = (const sT*)srcmat.data;
    dT* dst = (dT*)dstmat.data;
    const dT* delta = (const dT*)deltamat.data;
    size_t srcstep = srcmat.step/sizeof(src[0]);
    size_t dststep = dstmat.step/sizeof(dst[0]);
    size_t deltastep = deltamat.rows > 1 ? deltamat.step/sizeof(delta[0]) : 0;
    int delta_cols = deltamat.cols;
    Size size = srcmat.size();
    dT* tdst = dst;
    dT* col_buf = 0;
    dT* delta_buf = 0;
    int buf_size = size.height*sizeof(dT);
    AutoBuffer<uchar> buf;

    if( delta && delta_cols < size.width )
    {
        assert( delta_cols == 1 );
        buf_size *= 5;
    }
    buf.allocate(buf_size);
    col_buf = (dT*)(uchar*)buf;

    if( delta && delta_cols < size.width )
    {
        delta_buf = col_buf + size.height;
        for( i = 0; i < size.height; i++ )
            delta_buf[i*4] = delta_buf[i*4+1] =
                delta_buf[i*4+2] = delta_buf[i*4+3] = delta[i*deltastep];
        delta = delta_buf;
        deltastep = deltastep ? 4 : 0;
    }

    if( !delta )
        for( i = 0; i < size.width; i++, tdst += dststep )
        {
            for( k = 0; k < size.height; k++ )
                col_buf[k] = src[k*srcstep+i];

            for( j = i; j <= size.width - 4; j += 4 )
            {
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                const sT *tsrc = src + j;

                for( k = 0; k < size.height; k++, tsrc += srcstep )
                {
                    double a = col_buf[k];
                    s0 += a * tsrc[0];
                    s1 += a * tsrc[1];
                    s2 += a * tsrc[2];
                    s3 += a * tsrc[3];
                }

                tdst[j] = (dT)(s0*scale);
                tdst[j+1] = (dT)(s1*scale);
                tdst[j+2] = (dT)(s2*scale);
                tdst[j+3] = (dT)(s3*scale);
            }

            for( ; j < size.width; j++ )
            {
                double s0 = 0;
                const sT *tsrc = src + j;

                for( k = 0; k < size.height; k++, tsrc += srcstep )
                    s0 += (double)col_buf[k] * tsrc[0];

                tdst[j] = (dT)(s0*scale);
            }
        }
    else
        for( i = 0; i < size.width; i++, tdst += dststep )
        {
            if( !delta_buf )
                for( k = 0; k < size.height; k++ )
                    col_buf[k] = src[k*srcstep+i] - delta[k*deltastep+i];
            else
                for( k = 0; k < size.height; k++ )
                    col_buf[k] = src[k*srcstep+i] - delta_buf[k*deltastep];

            for( j = i; j <= size.width - 4; j += 4 )
            {
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                const sT *tsrc = src + j;
                const dT *d = delta_buf ? delta_buf : delta + j;

                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep )
                {
                    double a = col_buf[k];
                    s0 += a * (tsrc[0] - d[0]);
                    s1 += a * (tsrc[1] - d[1]);
                    s2 += a * (tsrc[2] - d[2]);
                    s3 += a * (tsrc[3] - d[3]);
                }

                tdst[j] = (dT)(s0*scale);
                tdst[j+1] = (dT)(s1*scale);
                tdst[j+2] = (dT)(s2*scale);
                tdst[j+3] = (dT)(s3*scale);
            }

            for( ; j < size.width; j++ )
            {
                double s0 = 0;
                const sT *tsrc = src + j;
                const dT *d = delta_buf ? delta_buf : delta + j;

                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep )
                    s0 += (double)col_buf[k] * (tsrc[0] - d[0]);

                tdst[j] = (dT)(s0*scale);
            }
        }
}


template<typename sT, typename dT> static void
MulTransposedL( const Mat& srcmat, Mat& dstmat, const Mat& deltamat, double scale )
{
    int i, j, k;
    const sT* src = (const sT*)srcmat.data;
    dT* dst = (dT*)dstmat.data;
    const dT* delta = (const dT*)deltamat.data;
    size_t srcstep = srcmat.step/sizeof(src[0]);
    size_t dststep = dstmat.step/sizeof(dst[0]);
    size_t deltastep = deltamat.rows > 1 ? deltamat.step/sizeof(delta[0]) : 0;
    int delta_cols = deltamat.cols;
    Size size = srcmat.size();
    dT* tdst = dst;

    if( !delta )
        for( i = 0; i < size.height; i++, tdst += dststep )
            for( j = i; j < size.height; j++ )
            {
                double s = 0;
                const sT *tsrc1 = src + i*srcstep;
                const sT *tsrc2 = src + j*srcstep;

                for( k = 0; k <= size.width - 4; k += 4 )
                    s += (double)tsrc1[k]*tsrc2[k] + (double)tsrc1[k+1]*tsrc2[k+1] +
                         (double)tsrc1[k+2]*tsrc2[k+2] + (double)tsrc1[k+3]*tsrc2[k+3];
                for( ; k < size.width; k++ )
                    s += (double)tsrc1[k] * tsrc2[k];
                tdst[j] = (dT)(s*scale);
            }
    else
    {
        dT delta_buf[4];
        int delta_shift = delta_cols == size.width ? 4 : 0;
        AutoBuffer<uchar> buf(size.width*sizeof(dT));
        dT* row_buf = (dT*)(uchar*)buf;

        for( i = 0; i < size.height; i++, tdst += dststep )
        {
            const sT *tsrc1 = src + i*srcstep;
            const dT *tdelta1 = delta + i*deltastep;

            if( delta_cols < size.width )
                for( k = 0; k < size.width; k++ )
                    row_buf[k] = tsrc1[k] - tdelta1[0];
            else
                for( k = 0; k < size.width; k++ )
                    row_buf[k] = tsrc1[k] - tdelta1[k];

            for( j = i; j < size.height; j++ )
            {
                double s = 0;
                const sT *tsrc2 = src + j*srcstep;
                const dT *tdelta2 = delta + j*deltastep;
                if( delta_cols < size.width )
                {
                    delta_buf[0] = delta_buf[1] =
                        delta_buf[2] = delta_buf[3] = tdelta2[0];
                    tdelta2 = delta_buf;
                }
                for( k = 0; k <= size.width-4; k += 4, tdelta2 += delta_shift )
                    s += (double)row_buf[k]*(tsrc2[k] - tdelta2[0]) +
                         (double)row_buf[k+1]*(tsrc2[k+1] - tdelta2[1]) +
                         (double)row_buf[k+2]*(tsrc2[k+2] - tdelta2[2]) +
                         (double)row_buf[k+3]*(tsrc2[k+3] - tdelta2[3]);
                for( ; k < size.width; k++, tdelta2++ )
                    s += (double)row_buf[k]*(tsrc2[k] - tdelta2[0]);
                tdst[j] = (dT)(s*scale);
            }
        }
    }
}

typedef void (*MulTransposedFunc)(const Mat& src, Mat& dst, const Mat& delta, double scale);

}

void cv::mulTransposed( InputArray _src, OutputArray _dst, bool ata,
                        InputArray _delta, double scale, int dtype )
{
    Mat src = _src.getMat(), delta = _delta.getMat();
    const int gemm_level = 100; // boundary above which GEMM is faster.
    int stype = src.type();
    dtype = std::max(std::max(CV_MAT_DEPTH(dtype >= 0 ? dtype : stype), delta.depth()), CV_32F);
    CV_Assert( src.channels() == 1 );

    if( delta.data )
    {
        CV_Assert( delta.channels() == 1 &&
            (delta.rows == src.rows || delta.rows == 1) &&
            (delta.cols == src.cols || delta.cols == 1));
        if( delta.type() != dtype )
            delta.convertTo(delta, dtype);
    }

    int dsize = ata ? src.cols : src.rows;
    _dst.create( dsize, dsize, dtype );
    Mat dst = _dst.getMat();

    if( src.data == dst.data || (stype == dtype &&
        (dst.cols >= gemm_level && dst.rows >= gemm_level &&
         src.cols >= gemm_level && src.rows >= gemm_level)))
    {
        Mat src2;
        const Mat* tsrc = &src;
        if( delta.data )
        {
            if( delta.size() == src.size() )
                subtract( src, delta, src2 );
            else
            {
                repeat(delta, src.rows/delta.rows, src.cols/delta.cols, src2);
                subtract( src, src2, src2 );
            }
            tsrc = &src2;
        }
        assert("herelsp remove" && 0);//gemm( *tsrc, *tsrc, scale, Mat(), 0, dst, ata ? GEMM_1_T : GEMM_2_T );
    }
    else
    {
        MulTransposedFunc func = 0;
        if(stype == CV_8U && dtype == CV_32F)
        {
            if(ata)
                func = MulTransposedR<uchar,float>;
            else
                func = MulTransposedL<uchar,float>;
        }
        else if(stype == CV_8U && dtype == CV_64F)
        {
            if(ata)
                func = MulTransposedR<uchar,double>;
            else
                func = MulTransposedL<uchar,double>;
        }
        else if(stype == CV_16U && dtype == CV_32F)
        {
            if(ata)
                func = MulTransposedR<ushort,float>;
            else
                func = MulTransposedL<ushort,float>;
        }
        else if(stype == CV_16U && dtype == CV_64F)
        {
            if(ata)
                func = MulTransposedR<ushort,double>;
            else
                func = MulTransposedL<ushort,double>;
        }
        else if(stype == CV_16S && dtype == CV_32F)
        {
            if(ata)
                func = MulTransposedR<short,float>;
            else
                func = MulTransposedL<short,float>;
        }
        else if(stype == CV_16S && dtype == CV_64F)
        {
            if(ata)
                func = MulTransposedR<short,double>;
            else
                func = MulTransposedL<short,double>;
        }
        else if(stype == CV_32F && dtype == CV_32F)
        {
            if(ata)
                func = MulTransposedR<float,float>;
            else
                func = MulTransposedL<float,float>;
        }
        else if(stype == CV_32F && dtype == CV_64F)
        {
            if(ata)
                func = MulTransposedR<float,double>;
            else
                func = MulTransposedL<float,double>;
        }
        else if(stype == CV_64F && dtype == CV_64F)
        {
            if(ata)
                func = MulTransposedR<double,double>;
            else
                func = MulTransposedL<double,double>;
        }
        if( !func )
            CV_Error( CV_StsUnsupportedFormat, "" );

        func( src, dst, delta, scale );
        completeSymm( dst, false );
    }
}

/****************************************************************************************\
*                                      Dot Product                                       *
\****************************************************************************************/

namespace cv
{

	CV_IMPL void
		cvPerspectiveTransform(const CvArr* srcarr, CvArr* dstarr, const CvMat* mat)
	{
		cv::Mat m = cv::cvarrToMat(mat), src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr);

		CV_Assert(dst.type() == src.type() && dst.channels() == m.rows - 1);
		cv::perspectiveTransform(src, dst, m);
	}

}

/* End of file. */

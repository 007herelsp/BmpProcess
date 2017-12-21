#include "_cv.h"

/////////////////////////////////////////////////////////////////////////////////////////


/****************************************************************************************\
*                                 Color to/from Grayscale                                *
\****************************************************************************************/

#define fix(x,n)      (int)((x)*(1 << (n)) + 0.5)
#define descale       CV_DESCALE

#define cscGr_32f  0.299f
#define cscGg_32f  0.587f
#define cscGb_32f  0.114f

/* BGR/RGB -> Gray */
#define csc_shift  14
#define cscGr  fix(cscGr_32f,csc_shift) 
#define cscGg  fix(cscGg_32f,csc_shift)
#define cscGb  /*fix(cscGb_32f,csc_shift)*/ ((1 << csc_shift) - cscGr - cscGg)



static CvStatus CV_STDCALL
icvBGRx2Gray_8u_CnC1R( const uchar* src, int srcstep,
                       uchar* dst, int dststep, CvSize size,
                       int src_cn, int blue_idx )
{
    int i;
    srcstep -= size.width*src_cn;

    if( size.width*size.height >= 1024 )
    {
        int* tab = (int*)cvStackAlloc( 256*3*sizeof(tab[0]) );
        int r = 0, g = 0, b = (1 << (csc_shift-1));
    
        for( i = 0; i < 256; i++ )
        {
            tab[i] = b;
            tab[i+256] = g;
            tab[i+512] = r;
            g += cscGg;
            if( !blue_idx )
                b += cscGb, r += cscGr;
            else
                b += cscGr, r += cscGb;
        }

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = tab[src[0]] + tab[src[1] + 256] + tab[src[2] + 512];
                dst[i] = (uchar)(t0 >> csc_shift);
            }
        }
    }
    else
    {
        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = src[blue_idx]*cscGb + src[1]*cscGg + src[blue_idx^2]*cscGr;
                dst[i] = (uchar)CV_DESCALE(t0, csc_shift);
            }
        }
    }
    return CV_OK;
}


/****************************************************************************************\
*                                   The main function                                    *
\****************************************************************************************/

CV_IMPL void
cvCvtColor( const CvArr* srcarr, CvArr* dstarr, int code )
{
    CV_FUNCNAME( "cvCvtColor" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int src_cn, dst_cn, depth;
    int param[] = { 0, 0, 0, 0 };
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_DEPTHS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    depth = CV_MAT_DEPTH(src->type);
    if( depth != CV_8U && depth != CV_16U && depth != CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_cn = CV_MAT_CN( src->type );
    dst_cn = CV_MAT_CN( dst->type );
    size = cvGetMatSize( src );
    src_step = src->step;
    dst_step = dst->step;

    
    switch( code )
    {
   
    case CV_BGR2GRAY:
    case CV_RGB2GRAY:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

		assert("herelsp remove" && (depth == CV_8U ));
               
        param[0] = src_cn;
        param[1] = code == CV_BGR2GRAY || code == CV_BGRA2GRAY ? 0 : 2;
		  IPPI_CALL( icvBGRx2Gray_8u_CnC1R( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1] ));
        break;

    default:
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported color conversion code" );
    }

    __END__;
}

/* End of file. */



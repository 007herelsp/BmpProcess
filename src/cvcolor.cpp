

#include "_cv.h"

typedef CvStatus (CV_STDCALL * CvColorCvtFunc0)(
    const void* src, int srcstep, void* dst, int dststep, CvSize size );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc1)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc2)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc3)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1, int param2 );



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
    CvColorCvtFunc0 func0 = 0;
    CvColorCvtFunc1 func1 = 0;
    CvColorCvtFunc2 func2 = 0;
    CvColorCvtFunc3 func3 = 0;
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

    if( CV_IS_MAT_CONT(src->type & dst->type) &&
        code != CV_BayerBG2BGR && code != CV_BayerGB2BGR &&
        code != CV_BayerRG2BGR && code != CV_BayerGR2BGR ) 
    {
        size.width *= size.height;
        size.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }

    switch( code )
    {
   
    case CV_BGR2GRAY:
    case CV_RGB2GRAY:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );
	assert("herelsp remove" && (depth == CV_8U ));
        func2 = (CvColorCvtFunc2)icvBGRx2Gray_8u_CnC1R;
               
        param[0] = src_cn;
        param[1] = code == CV_BGR2GRAY || code == CV_BGRA2GRAY ? 0 : 2;
        break;

    default:
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported color conversion code" );
    }

  if( func2 )
    {
        IPPI_CALL( func2( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1] ));
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "The image format is not supported" );

    __END__;
}

/* End of file. */



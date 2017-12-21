

#include "_cv.h"

/****************************************************************************************/

/* lightweight convolution with 3x3 kernel */
void icvSepConvSmall3_32f( float* src, int src_step, float* dst, int dst_step,
            CvSize src_size, const float* kx, const float* ky, float* buffer )
{
    int  dst_width, buffer_step = 0;
    int  x, y;

    assert( src && dst && src_size.width > 2 && src_size.height > 2 &&
            (src_step & 3) == 0 && (dst_step & 3) == 0 &&
            (kx || ky) && (buffer || !kx || !ky));

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    dst_width = src_size.width - 2;

    if( !kx )
    {
        /* set vars, so that vertical convolution
           will write results into destination ROI and
           horizontal convolution won't run */
        src_size.width = dst_width;
        buffer_step = dst_step;
        buffer = dst;
        dst_width = 0;
    }

    assert( src_step >= src_size.width && dst_step >= dst_width );

    src_size.height -= 3;
    if( !ky )
    {
        /* set vars, so that vertical convolution won't run and
           horizontal convolution will write results into destination ROI */
        src_size.height += 3;
        buffer_step = src_step;
        buffer = src;
        src_size.width = 0;
    }

    for( y = 0; y <= src_size.height; y++, src += src_step,
                                           dst += dst_step,
                                           buffer += buffer_step )
    {
        float* src2 = src + src_step;
        float* src3 = src + src_step*2;
        for( x = 0; x < src_size.width; x++ )
        {
            buffer[x] = (float)(ky[0]*src[x] + ky[1]*src2[x] + ky[2]*src3[x]);
        }

        for( x = 0; x < dst_width; x++ )
        {
            dst[x] = (float)(kx[0]*buffer[x] + kx[1]*buffer[x+1] + kx[2]*buffer[x+2]);
        }
    }
}


/****************************************************************************************\
                             Sobel & Scharr Derivative Filters
\****************************************************************************************/

/////////////////////////////// Old IPP derivative filters ///////////////////////////////
// still used in corner detectors (see cvcorner.cpp)

icvFilterSobelVert_8u16s_C1R_t icvFilterSobelVert_8u16s_C1R_p = 0;
icvFilterSobelHoriz_8u16s_C1R_t icvFilterSobelHoriz_8u16s_C1R_p = 0;
icvFilterSobelVertSecond_8u16s_C1R_t icvFilterSobelVertSecond_8u16s_C1R_p = 0;
icvFilterSobelHorizSecond_8u16s_C1R_t icvFilterSobelHorizSecond_8u16s_C1R_p = 0;
icvFilterSobelCross_8u16s_C1R_t icvFilterSobelCross_8u16s_C1R_p = 0;

icvFilterSobelVert_32f_C1R_t icvFilterSobelVert_32f_C1R_p = 0;
icvFilterSobelHoriz_32f_C1R_t icvFilterSobelHoriz_32f_C1R_p = 0;
icvFilterSobelVertSecond_32f_C1R_t icvFilterSobelVertSecond_32f_C1R_p = 0;
icvFilterSobelHorizSecond_32f_C1R_t icvFilterSobelHorizSecond_32f_C1R_p = 0;
icvFilterSobelCross_32f_C1R_t icvFilterSobelCross_32f_C1R_p = 0;

icvFilterScharrVert_8u16s_C1R_t icvFilterScharrVert_8u16s_C1R_p = 0;
icvFilterScharrHoriz_8u16s_C1R_t icvFilterScharrHoriz_8u16s_C1R_p = 0;
icvFilterScharrVert_32f_C1R_t icvFilterScharrVert_32f_C1R_p = 0;
icvFilterScharrHoriz_32f_C1R_t icvFilterScharrHoriz_32f_C1R_p = 0;

///////////////////////////////// New IPP derivative filters /////////////////////////////

#define IPCV_FILTER_PTRS( name )                    \
icvFilter##name##GetBufSize_8u16s_C1R_t             \
    icvFilter##name##GetBufSize_8u16s_C1R_p = 0;    \
icvFilter##name##Border_8u16s_C1R_t                 \
    icvFilter##name##Border_8u16s_C1R_p = 0;        \
icvFilter##name##GetBufSize_32f_C1R_t               \
    icvFilter##name##GetBufSize_32f_C1R_p = 0;      \
icvFilter##name##Border_32f_C1R_t                   \
    icvFilter##name##Border_32f_C1R_p = 0;

IPCV_FILTER_PTRS( ScharrHoriz )
IPCV_FILTER_PTRS( ScharrVert )
IPCV_FILTER_PTRS( SobelHoriz )
IPCV_FILTER_PTRS( SobelNegVert )
IPCV_FILTER_PTRS( SobelHorizSecond )
IPCV_FILTER_PTRS( SobelVertSecond )
IPCV_FILTER_PTRS( SobelCross )
IPCV_FILTER_PTRS( Laplacian )

typedef CvStatus (CV_STDCALL * CvDeriv3x3GetBufSizeIPPFunc)
    ( CvSize roi, int* bufsize );

typedef CvStatus (CV_STDCALL * CvDerivGetBufSizeIPPFunc)
    ( CvSize roi, int masksize, int* bufsize );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, float bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      float bordervalue, void* buffer );

//////////////////////////////////////////////////////////////////////////////////////////

CV_IMPL void
cvSobel( const void* srcarr, void* dstarr, int dx, int dy, int aperture_size )
{
    CvSepFilter filter;
    void* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSobel" );

    __BEGIN__;

    int origin = 0;
    int src_type, dst_type;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub ));
    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( CV_IS_IMAGE_HDR( srcarr ))
        origin = ((IplImage*)srcarr)->origin;

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsBadArg, "src and dst have different sizes" );

    if( ((aperture_size == CV_SCHARR || aperture_size == 3 || aperture_size == 5) &&
        dx <= 2 && dy <= 2 && dx + dy <= 2 && icvFilterSobelNegVertBorder_8u16s_C1R_p) &&
        (src_type == CV_8UC1 && dst_type == CV_16SC1 ||
        src_type == CV_32FC1 && dst_type == CV_32FC1) )
    {
        CvDerivGetBufSizeIPPFunc ipp_sobel_getbufsize_func = 0;
        CvDerivIPPFunc_8u ipp_sobel_func_8u = 0;
        CvDerivIPPFunc_32f ipp_sobel_func_32f = 0;

        CvDeriv3x3GetBufSizeIPPFunc ipp_scharr_getbufsize_func = 0;
        CvDeriv3x3IPPFunc_8u ipp_scharr_func_8u = 0;
        CvDeriv3x3IPPFunc_32f ipp_scharr_func_32f = 0;

        if( aperture_size == CV_SCHARR )
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrVertBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrVertBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrHorizBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrHorizBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_32f_C1R_p;
            }
            else
                CV_ERROR( CV_StsBadArg, "Scharr filter can only be used to compute 1st image derivatives" );
        }
        else
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelNegVertBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelNegVertBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_32f_C1R_p;
            }
            else if( dx == 2 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelVertSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelVertSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 2 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 1 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelCrossBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelCrossBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_32f_C1R_p;
            }
        }

        if( (ipp_sobel_func_8u || ipp_sobel_func_32f) && ipp_sobel_getbufsize_func ||
            (ipp_scharr_func_8u || ipp_scharr_func_32f) && ipp_scharr_getbufsize_func )
        {
            int bufsize = 0, masksize = aperture_size == 3 ? 33 : 55;
            CvSize size = cvGetMatSize( src );
            uchar* src_ptr = src->data.ptr;
            uchar* dst_ptr = dst->data.ptr;
            int src_step = src->step ? src->step : CV_STUB_STEP;
            int dst_step = dst->step ? dst->step : CV_STUB_STEP;
            const int bordertype = 1; // replication border
            CvStatus status;

            status = ipp_sobel_getbufsize_func ?
                ipp_sobel_getbufsize_func( size, masksize, &bufsize ) :
                ipp_scharr_getbufsize_func( size, &bufsize );

            if( status >= 0 )
            {
                if( bufsize <= CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            
                status =
                    ipp_sobel_func_8u ? ipp_sobel_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                           size, masksize, bordertype, 0, buffer ) :
                    ipp_sobel_func_32f ? ipp_sobel_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, masksize, bordertype, 0, buffer ) :
                    ipp_scharr_func_8u ? ipp_scharr_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, bordertype, 0, buffer ) :
                    ipp_scharr_func_32f ? ipp_scharr_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                               size, bordertype, 0, buffer ) : 
                        CV_NOTDEFINED_ERR;
            }

            if( status >= 0 &&
                (dx == 0 && dy == 1 && origin || dx == 1 && dy == 1 && !origin)) // negate the output
                cvSubRS( dst, cvScalarAll(0), dst );

            if( status >= 0 )
                EXIT;
        }
    }

    CV_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    CV_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}


/* End of file. */

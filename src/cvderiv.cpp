

#include "_cv.h"

/****************************************************************************************/


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

    CV_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    CV_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}


/* End of file. */

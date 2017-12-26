#include "_cxcore.h"

CV_IMPL void
cvConvertScale( const void* srcarr, void* dstarr,
                double scale, double shift )
{
    CV_FUNCNAME( "cvConvertScale" );

    __BEGIN__;
    CvMat  *src = (CvMat*)srcarr;
    CvMat  *dst = (CvMat*)dstarr;

    int no_scale = scale == 1 && shift == 0;

    if( (!CV_IS_MAT(src)) || (!CV_IS_MAT(dst)) )
    {
       CV_ERROR( CV_BadCOI, "herelsp remove" );
    }

    if( no_scale && CV_ARE_TYPES_EQ( src, dst ) )
    {
        if( src != dst )
          cvCopy( src, dst );
        EXIT;
    }
    else
    {
         CV_ERROR( CV_BadCOI, "herelsp remove" );
    }

    __END__;
}

/* End of file. */

#include "_cxcore.h"

VOS_IMPL void
cvConvertScale( const void* srcarr, void* dstarr,
                double scale, double shift )
{
    VOS_FUNCNAME( "cvConvertScale" );

    __BEGIN__;
    CvMat  *src = (CvMat*)srcarr;
    CvMat  *dst = (CvMat*)dstarr;

    int no_scale = scale == 1 && shift == 0;

    if( (!VOS_IS_MAT(src)) || (!VOS_IS_MAT(dst)) )
    {
       VOS_ERROR( VOS_BadCOI, "herelsp remove" );
    }

    if( no_scale && VOS_ARE_TYPES_EQ( src, dst ) )
    {
        if( src != dst )
          cvCopy( src, dst );
        EXIT;
    }
    else
    {
         VOS_ERROR( VOS_BadCOI, "herelsp remove" );
    }

    __END__;
}

/* End of file. */

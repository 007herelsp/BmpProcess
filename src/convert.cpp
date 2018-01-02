#include "_cxcore.h"

 void
ConvertScale( const void* srcarr, void* dstarr,
                double scale, double shift )
{
    VOS_FUNCNAME( "ConvertScale" );

    __BEGIN__;
    Mat  *src = (Mat*)srcarr;
    Mat  *dst = (Mat*)dstarr;

    int no_scale = scale == 1 && shift == 0;

    if( (!VOS_IS_MAT(src)) || (!VOS_IS_MAT(dst)) )
    {
       VOS_ERROR( VOS_BadCOI, "herelsp remove" );
    }

    if( no_scale && VOS_ARE_TYPES_EQ( src, dst ) )
    {
        if( src != dst )
          Copy( src, dst );
        EXIT;
    }
    else
    {
         VOS_ERROR( VOS_BadCOI, "herelsp remove" );
    }

    __END__;
}

/* End of file. */

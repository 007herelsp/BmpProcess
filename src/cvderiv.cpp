#include "_cv.h"

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

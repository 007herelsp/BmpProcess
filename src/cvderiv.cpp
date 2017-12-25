#include "_cv.h"

VOS_IMPL void
cvSobel( const void* srcarr, void* dstarr, int dx, int dy, int aperture_size )
{
    CvSepFilter filter;
    void* buffer = 0;
    int local_alloc = 0;

    VOS_FUNCNAME( "cvSobel" );

    __BEGIN__;

    int origin = 0;
    int src_type, dst_type;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;

    if( !VOS_IS_MAT(src) )
        VOS_CALL( src = cvGetMat( src, &srcstub ));
    if( !VOS_IS_MAT(dst) )
        VOS_CALL( dst = cvGetMat( dst, &dststub ));

    if( VOS_IS_IMAGE_HDR( srcarr ))
        origin = ((IplImage*)srcarr)->origin;

    src_type = VOS_MAT_TYPE( src->type );
    dst_type = VOS_MAT_TYPE( dst->type );

    if( !VOS_ARE_SIZES_EQ( src, dst ))
        VOS_ERROR( VOS_StsBadArg, "src and dst have different sizes" );

    VOS_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    VOS_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}


/* End of file. */


#include "_cv.h"

CV_IMPL CvSeq* cvPointSeqFromMat( int seq_kind, const CvArr* arr,
                                  CvContour* contour_header, CvSeqBlock* block )
{
    CvSeq* contour = 0;

    CV_FUNCNAME( "cvPointSeqFromMat" );

    assert( arr != 0 && contour_header != 0 && block != 0 );

    __BEGIN__;
    
    int eltype;
    CvMat* mat = (CvMat*)arr;
    
    if( !CV_IS_MAT( mat ))
        CV_ERROR( CV_StsBadArg, "Input array is not a valid matrix" ); 

    eltype = CV_MAT_TYPE( mat->type );
    if( eltype != CV_32SC2 && eltype != CV_32FC2 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The matrix can not be converted to point sequence because of "
        "inappropriate element type" );

    if( mat->width != 1 && mat->height != 1 || !CV_IS_MAT_CONT(mat->type))
        CV_ERROR( CV_StsBadArg,
        "The matrix converted to point sequence must be "
        "1-dimensional and continuous" );

    CV_CALL( cvMakeSeqHeaderForArray(
            (seq_kind & (CV_SEQ_KIND_MASK|CV_SEQ_FLAG_CLOSED)) | eltype,
            sizeof(CvContour), CV_ELEM_SIZE(eltype), mat->data.ptr,
            mat->width*mat->height, (CvSeq*)contour_header, block ));

    contour = (CvSeq*)contour_header;

    __END__;

    return contour;
}


/* End of file. */

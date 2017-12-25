
//
//  Loading and saving IPL images.
//

#include "_highgui.h"
#include "grfmt_base.h"
#include "grfmt_bmp.h"



/****************************************************************************************\
*                              Image Readers & Writers Class                             *
\****************************************************************************************/

static void*
icvLoadImage( const char* filename, int flags, bool load_as_matrix )
{
    GrFmtBmpReader* reader = 0;
    IplImage* image = 0;
    CvMat hdr, *matrix = 0;
    int depth = 8;

    VOS_FUNCNAME( "cvLoadImage" );

    __BEGIN__;

    CvSize size;
    int iscolor;
    int cn;

    if( !filename || strlen(filename) == 0 )
        VOS_ERROR( VOS_StsNullPtr, "null filename" );

    reader = new GrFmtBmpReader(filename);
    if( !reader )
        EXIT;

    if( !reader->ReadHeader() )
        EXIT;

    size.width = reader->GetWidth();
    size.height = reader->GetHeight();

    if( flags == -1 )
        iscolor = reader->IsColor();
    else
    {
        if( (flags & VOS_LOAD_IMAGE_COLOR) != 0 ||
           ((flags & VOS_LOAD_IMAGE_ANYCOLOR) != 0 && reader->IsColor()) )
            iscolor = 1;
        else
            iscolor = 0;

        if( (flags & VOS_LOAD_IMAGE_ANYDEPTH) != 0 )
        {
            reader->UseNativeDepth(true);
            depth = reader->GetDepth();
        }
    }

    cn = iscolor ? 3 : 1;
    {
        int type;
        if(reader->IsFloat() && depth != 8)
            type = IPL_DEPTH_32F;
        else
            type = ( depth <= 8 ) ? IPL_DEPTH_8U : ( depth <= 16 ) ? IPL_DEPTH_16U : IPL_DEPTH_32S;
        VOS_CALL( image = cvCreateImage( size, type, cn ));
        matrix = cvGetMat( image, &hdr );
    }

    if( !reader->ReadData( matrix->data.ptr, matrix->step, iscolor ))
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
        EXIT;
    }

    __END__;

    delete reader;

    if( cvGetErrStatus() < 0 )
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
    }

    return (void*)image;
}


VOS_IMPL IplImage*
cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)icvLoadImage( filename, iscolor, false );
}



VOS_IMPL int
cvSaveImage( const char* filename, const CvArr* arr )
{
    int origin = 0;
    GrFmtBmpWriter* writer = 0;
    CvMat *temp = 0, *temp2 = 0;

    VOS_FUNCNAME( "cvSaveImage" );

    __BEGIN__;

    CvMat stub, *image;
    int channels, ipl_depth;

    if( !filename || strlen(filename) == 0 )
        VOS_ERROR( VOS_StsNullPtr, "null filename" );

    VOS_CALL( image = cvGetMat( arr, &stub ));

    if( VOS_IS_IMAGE( arr ))
        origin = ((IplImage*)arr)->origin;

    channels = VOS_MAT_CN( image->type );
    if( channels != 1 && channels != 3 && channels != 4 )
        VOS_ERROR( VOS_BadNumChannels, "" );

    writer = new GrFmtBmpWriter(filename);
    if( !writer )
        VOS_ERROR( VOS_StsError, "could not find a filter for the specified extension" );

    if( origin )
    {
       VOS_ERROR( VOS_StsError, "could not find a filter for the specified extension" );
    }

    ipl_depth = cvCvToIplDepth(image->type);



    if( !writer->WriteImage( image->data.ptr, image->step, image->width,
                             image->height, ipl_depth, channels ))
        VOS_ERROR( VOS_StsError, "could not save the image" );

    __END__;

    delete writer;
    cvReleaseMat( &temp );
    cvReleaseMat( &temp2 );

    return cvGetErrStatus() >= 0;
}

/* End of file. */

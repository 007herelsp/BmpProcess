
//
//  Loading and saving IPL images.
//

#include "_highgui.h"
#include "grfmts.h"


/****************************************************************************************\
*                              Image Readers & Writers Class                             *
\****************************************************************************************/

class  CvImageFilters
{
public:

    CvImageFilters();
    ~CvImageFilters();

    GrFmtReader* FindReader( const char* filename ) const;
    GrFmtWriter* FindWriter( const char* filename ) const;

    //const CvFilePath& Path() const { return (const CvFilePath&)m_path; };
    //CvFilePath& Path() { return m_path; };

protected:

    GrFmtFactoriesList*  m_factories;
};


CvImageFilters::CvImageFilters()
{
    m_factories = new GrFmtFactoriesList;

    m_factories->AddFactory( new GrFmtBmp() );
}


CvImageFilters::~CvImageFilters()
{
    delete m_factories;
}


GrFmtReader* CvImageFilters::FindReader( const char* filename ) const
{
    return m_factories->FindReader( filename );
}


GrFmtWriter* CvImageFilters::FindWriter( const char* filename ) const
{
    return m_factories->FindWriter( filename );
}

/****************************************************************************************\
*                         HighGUI loading & saving function implementation               *
\****************************************************************************************/



// global image I/O filters
static CvImageFilters  g_Filters;


static void*
icvLoadImage( const char* filename, int flags, bool load_as_matrix )
{
    GrFmtReader* reader = 0;
    IplImage* image = 0;
    CvMat hdr, *matrix = 0;
    int depth = 8;

    CV_FUNCNAME( "cvLoadImage" );

    __BEGIN__;

    CvSize size;
    int iscolor;
    int cn;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    reader = g_Filters.FindReader( filename );
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
        if( (flags & CV_LOAD_IMAGE_COLOR) != 0 ||
           ((flags & CV_LOAD_IMAGE_ANYCOLOR) != 0 && reader->IsColor()) )
            iscolor = 1;
        else
            iscolor = 0;

        if( (flags & CV_LOAD_IMAGE_ANYDEPTH) != 0 )
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
        CV_CALL( image = cvCreateImage( size, type, cn ));
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


CV_IMPL IplImage*
cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)icvLoadImage( filename, iscolor, false );
}



CV_IMPL int
cvSaveImage( const char* filename, const CvArr* arr )
{
    int origin = 0;
    GrFmtWriter* writer = 0;
    CvMat *temp = 0, *temp2 = 0;

    CV_FUNCNAME( "cvSaveImage" );

    __BEGIN__;

    CvMat stub, *image;
    int channels, ipl_depth;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    CV_CALL( image = cvGetMat( arr, &stub ));

    if( CV_IS_IMAGE( arr ))
        origin = ((IplImage*)arr)->origin;

    channels = CV_MAT_CN( image->type );
    if( channels != 1 && channels != 3 && channels != 4 )
        CV_ERROR( CV_BadNumChannels, "" );

    writer = g_Filters.FindWriter( filename );
    if( !writer )
        CV_ERROR( CV_StsError, "could not find a filter for the specified extension" );

    if( origin )
    {
        CV_CALL( temp = cvCreateMat(image->rows, image->cols, image->type) );
        CV_CALL( cvFlip( image, temp, 0 ));
        image = temp;
    }

    ipl_depth = cvCvToIplDepth(image->type);



    if( !writer->WriteImage( image->data.ptr, image->step, image->width,
                             image->height, ipl_depth, channels ))
        CV_ERROR( CV_StsError, "could not save the image" );

    __END__;

    delete writer;
    cvReleaseMat( &temp );
    cvReleaseMat( &temp2 );

    return cvGetErrStatus() >= 0;
}

/* End of file. */

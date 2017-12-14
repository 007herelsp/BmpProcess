 

//
//  Loading and saving IPL images.
//

#include "highgui/precomp.hpp"
#include "grfmt_base.hpp"
#include "grfmt_bmp.hpp"
#undef min
#undef max

#include <iostream>

/****************************************************************************************\
*                                      Image Codecs                                      *
\****************************************************************************************/
namespace cv
{

// TODO Add runtime configuration
#define CV_IO_MAX_IMAGE_PARAMS (50)
#define CV_IO_MAX_IMAGE_WIDTH (1<<20)
#define CV_IO_MAX_IMAGE_HEIGHT (1<<20)
#define CV_IO_MAX_IMAGE_PIXELS (1<<30) // 1 Gigapixel

static Size validateInputImageSize(const Size& size)
{
    CV_Assert(size.width > 0);
    CV_Assert(size.width <= CV_IO_MAX_IMAGE_WIDTH);
    CV_Assert(size.height > 0);
    CV_Assert(size.height <= CV_IO_MAX_IMAGE_HEIGHT);
    uint64 pixels = (uint64)size.width * (uint64)size.height;
    CV_Assert(pixels <= CV_IO_MAX_IMAGE_PIXELS);
    return size;
}


struct ImageCodecInitializer
{
    ImageCodecInitializer()
    {
        decoders.push_back( new BmpDecoder );
        encoders.push_back( new BmpEncoder );
    }

    vector<ImageDecoder> decoders;
    vector<ImageEncoder> encoders;
};

static ImageCodecInitializer codecs;

static ImageDecoder findDecoder( const string& filename )
{
    size_t i, maxlen = 0;
    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        size_t len = codecs.decoders[i]->signatureLength();
        maxlen = std::max(maxlen, len);
    }

    FILE* f= fopen( filename.c_str(), "rb" );
    if( !f )
        return ImageDecoder();
    string signature(maxlen, ' ');
    maxlen = fread( &signature[0], 1, maxlen, f );
    fclose(f);
    signature = signature.substr(0, maxlen);

    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        if( codecs.decoders[i]->checkSignature(signature) )
            return codecs.decoders[i]->newDecoder();
    }

    return ImageDecoder();
}

static ImageEncoder findEncoder( const string& _ext )
{
    if( _ext.size() <= 1 )
        return ImageEncoder();

    const char* ext = strrchr( _ext.c_str(), '.' );
    if( !ext )
        return ImageEncoder();
    int len = 0;
    for( ext++; isalnum(ext[len]) && len < 128; len++ )
        ;

    for( size_t i = 0; i < codecs.encoders.size(); i++ )
    {
        string description = codecs.encoders[i]->getDescription();
        const char* descr = strchr( description.c_str(), '(' );

        while( descr )
        {
            descr = strchr( descr + 1, '.' );
            if( !descr )
                break;
            int j = 0;
            for( descr++; isalnum(descr[j]) && j < len; j++ )
            {
                int c1 = tolower(ext[j]);
                int c2 = tolower(descr[j]);
                if( c1 != c2 )
                    break;
            }
            if( j == len && !isalnum(descr[j]))
                return codecs.encoders[i]->newEncoder();
            descr += j;
        }
    }

    return ImageEncoder();
}

enum { LOAD_CVMAT=0, LOAD_IMAGE=1, LOAD_MAT=2 };

static void*
imread_( const string& filename, int flags, int hdrtype, Mat* mat=0 )
{
    IplImage* image = 0;
    CvMat *matrix = 0;
    Mat temp, *data = &temp;

    ImageDecoder decoder = findDecoder(filename);
    if( decoder.empty() )
        return 0;
    decoder->setSource(filename);

    try
    {
        // read the header to make sure it succeeds
        if (!decoder->readHeader())
            return 0;
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "imread_('" << filename << "'): can't read header: " << e.what() << std::endl << std::flush;
        return 0;
    }
    catch (...)
    {
        std::cerr << "imread_('" << filename << "'): can't read header: unknown exception" << std::endl << std::flush;
        return 0;
    }


    Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

    int type = decoder->type();
    if( flags != -1 )
    {
        if( (flags & CV_LOAD_IMAGE_ANYDEPTH) == 0 )
            type = CV_MAKETYPE(CV_8U, CV_MAT_CN(type));

        if( (flags & CV_LOAD_IMAGE_COLOR) != 0 ||
           ((flags & CV_LOAD_IMAGE_ANYCOLOR) != 0 && CV_MAT_CN(type) > 1) )
            type = CV_MAKETYPE(CV_MAT_DEPTH(type), 3);
        else
            type = CV_MAKETYPE(CV_MAT_DEPTH(type), 1);
    }

    if( hdrtype == LOAD_CVMAT || hdrtype == LOAD_MAT )
    {
        if( hdrtype == LOAD_CVMAT )
        {
            matrix = cvCreateMat( size.height, size.width, type );
            temp = cvarrToMat(matrix);
        }
        else
        {
            mat->create( size.height, size.width, type );
            data = mat;
        }
    }
    else
    {
        image = cvCreateImage( size, cvIplDepth(type), CV_MAT_CN(type) );
        temp = cvarrToMat(image);
    }

    bool success = false;
    try
    {
        if (decoder->readData(*data))
            success = true;
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "imread_('" << filename << "'): can't read data: " << e.what() << std::endl << std::flush;
    }
    catch (...)
    {
        std::cerr << "imread_('" << filename << "'): can't read data: unknown exception" << std::endl << std::flush;
    }
    if (!success)
    {
        cvReleaseImage( &image );
        cvReleaseMat( &matrix );
        if( mat )
            mat->release();
        return 0;
    }

    return hdrtype == LOAD_CVMAT ? (void*)matrix :
        hdrtype == LOAD_IMAGE ? (void*)image : (void*)mat;
}

static bool imwrite_( const string& filename, const Mat& image,
                      const vector<int>& params, bool flipv )
{
    Mat temp;
    const Mat* pimage = &image;

    CV_Assert( image.channels() == 1 || image.channels() == 3 || image.channels() == 4 );

    ImageEncoder encoder = findEncoder( filename );
    if( encoder.empty() )
        CV_Error( CV_StsError, "could not find a writer for the specified extension" );

    if( !encoder->isFormatSupported(image.depth()) )
    {
        CV_Assert( encoder->isFormatSupported(CV_8U) );
        image.convertTo( temp, CV_8U );
        pimage = &temp;
    }

    if( flipv )
    {
        flip(*pimage, temp, 0);
        pimage = &temp;
    }

    encoder->setDestination( filename );
    CV_Assert(params.size() <= CV_IO_MAX_IMAGE_PARAMS*2);
    bool code = encoder->write( *pimage, params );

    //    CV_Assert( code );
    return code;
}


}



CV_IMPL IplImage*
cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)cv::imread_(filename, iscolor, cv::LOAD_IMAGE );
}


CV_IMPL int
cvSaveImage( const char* filename, const CvArr* arr, const int* _params )
{
    int i = 0;
    if( _params )
    {
        for( ; _params[i] > 0; i += 2 )
            CV_Assert(i < CV_IO_MAX_IMAGE_PARAMS*2); // Limit number of params for security reasons
    }
    return cv::imwrite_(filename, cv::cvarrToMat(arr),
        i > 0 ? cv::vector<int>(_params, _params+i) : cv::vector<int>(),
        CV_IS_IMAGE(arr) && ((const IplImage*)arr)->origin == IPL_ORIGIN_BL );
}


/* End of file. */

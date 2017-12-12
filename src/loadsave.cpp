 

//
//  Loading and saving IPL images.
//

#include "highgui/precomp.hpp"
#include "grfmts.hpp"
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
    // because it is a generic image I/O API, supporting many formats,
    // it should be last in the list.
    #ifdef HAVE_IMAGEIO
        decoders.push_back( new ImageIODecoder );
        encoders.push_back( new ImageIOEncoder );
    #endif
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

static ImageDecoder findDecoder( const Mat& buf )
{
    size_t i, maxlen = 0;

    if( buf.rows*buf.cols < 1 || !buf.isContinuous() )
        return ImageDecoder();

    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        size_t len = codecs.decoders[i]->signatureLength();
        maxlen = std::max(maxlen, len);
    }

    string signature(maxlen, ' ');
    size_t bufSize = buf.rows*buf.cols*buf.elemSize();
    maxlen = std::min(maxlen, bufSize);
    memcpy( &signature[0], buf.data, maxlen );

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

Mat imread( const string& filename, int flags )
{
    Mat img;
    imread_( filename, flags, LOAD_MAT, &img );
    return img;
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

bool imwrite( const string& filename, InputArray _img,
              const vector<int>& params )
{
    Mat img = _img.getMat();
    return imwrite_(filename, img, params, false);
}

static void*
imdecode_( const Mat& buf, int flags, int hdrtype, Mat* mat=0 )
{
    CV_Assert(buf.data && buf.isContinuous());
    IplImage* image = 0;
    CvMat *matrix = 0;
    Mat temp, *data = &temp;
    string filename;

    ImageDecoder decoder = findDecoder(buf);
    if( decoder.empty() )
        return 0;

    if( !decoder->setSource(buf) )
    {
        filename = tempfile();
        FILE* f = fopen( filename.c_str(), "wb" );
        if( !f )
            return 0;
        size_t bufSize = buf.cols*buf.rows*buf.elemSize();
        fwrite( &buf.data[0], 1, bufSize, f );
        fclose(f);
        decoder->setSource(filename);
    }

    bool success = false;
    try
    {
        if (decoder->readHeader())
            success = true;
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "imdecode_('" << filename << "'): can't read header: " << e.what() << std::endl << std::flush;
    }
    catch (...)
    {
        std::cerr << "imdecode_('" << filename << "'): can't read header: unknown exception" << std::endl << std::flush;
    }
    if (!success)
    {
        if (!filename.empty())
        {
            if (0 != remove(filename.c_str()))
            {
                std::cerr << "unable to remove temporary file:" << filename << std::endl << std::flush;
            }
        }
        return 0;
    }

    // established the required input image size
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

    success = false;
    try
    {
        if (decoder->readData(*data))
            success = true;
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "imdecode_('" << filename << "'): can't read data: " << e.what() << std::endl << std::flush;
    }
    catch (...)
    {
        std::cerr << "imdecode_('" << filename << "'): can't read data: unknown exception" << std::endl << std::flush;
    }

    if (!filename.empty())
    {
        if (0 != remove(filename.c_str()))
        {
            std::cerr << "unable to remove temporary file:" << filename << std::endl << std::flush;
        }
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


Mat imdecode( InputArray _buf, int flags )
{
    Mat buf = _buf.getMat(), img;
    imdecode_( buf, flags, LOAD_MAT, &img );
    return img;
}

Mat imdecode( InputArray _buf, int flags, Mat* dst )
{
    Mat buf = _buf.getMat(), img;
    dst = dst ? dst : &img;
    imdecode_( buf, flags, LOAD_MAT, dst );
    return *dst;
}

bool imencode( const string& ext, InputArray _image,
               vector<uchar>& buf, const vector<int>& params )
{
    Mat image = _image.getMat();

    int channels = image.channels();
    CV_Assert( channels == 1 || channels == 3 || channels == 4 );

    ImageEncoder encoder = findEncoder( ext );
    if( encoder.empty() )
        CV_Error( CV_StsError, "could not find encoder for the specified extension" );

    if( !encoder->isFormatSupported(image.depth()) )
    {
        CV_Assert( encoder->isFormatSupported(CV_8U) );
        Mat temp;
        image.convertTo(temp, CV_8U);
        image = temp;
    }

    bool code;
    if( encoder->setDestination(buf) )
    {
        code = encoder->write(image, params);
        encoder->throwOnEror();
        CV_Assert( code );
    }
    else
    {
        string filename = tempfile();
        code = encoder->setDestination(filename);
        CV_Assert( code );

        code = encoder->write(image, params);
        encoder->throwOnEror();
        CV_Assert( code );

        FILE* f = fopen( filename.c_str(), "rb" );
        CV_Assert(f != 0);
        fseek( f, 0, SEEK_END );
        long pos = ftell(f);
        buf.resize((size_t)pos);
        fseek( f, 0, SEEK_SET );
        buf.resize(fread( &buf[0], 1, buf.size(), f ));
        fclose(f);
        remove(filename.c_str());
    }
    return code;
}

}

/****************************************************************************************\
*                         HighGUI loading & saving function implementation               *
\****************************************************************************************/

CV_IMPL int
cvHaveImageReader( const char* filename )
{
    cv::ImageDecoder decoder = cv::findDecoder(filename);
    return !decoder.empty();
}

CV_IMPL int cvHaveImageWriter( const char* filename )
{
    cv::ImageEncoder encoder = cv::findEncoder(filename);
    return !encoder.empty();
}

CV_IMPL IplImage*
cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)cv::imread_(filename, iscolor, cv::LOAD_IMAGE );
}

CV_IMPL CvMat*
cvLoadImageM( const char* filename, int iscolor )
{
    return (CvMat*)cv::imread_( filename, iscolor, cv::LOAD_CVMAT );
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

/* decode image stored in the buffer */
CV_IMPL IplImage*
cvDecodeImage( const CvMat* _buf, int iscolor )
{
    CV_Assert( _buf && CV_IS_MAT_CONT(_buf->type) );
    cv::Mat buf(1, _buf->rows*_buf->cols*CV_ELEM_SIZE(_buf->type), CV_8U, _buf->data.ptr);
    return (IplImage*)cv::imdecode_(buf, iscolor, cv::LOAD_IMAGE );
}

CV_IMPL CvMat*
cvDecodeImageM( const CvMat* _buf, int iscolor )
{
    CV_Assert( _buf && CV_IS_MAT_CONT(_buf->type) );
    cv::Mat buf(1, _buf->rows*_buf->cols*CV_ELEM_SIZE(_buf->type), CV_8U, _buf->data.ptr);
    return (CvMat*)cv::imdecode_(buf, iscolor, cv::LOAD_CVMAT );
}

CV_IMPL CvMat*
cvEncodeImage( const char* ext, const CvArr* arr, const int* _params )
{
    int i = 0;
    if( _params )
    {
        for( ; _params[i] > 0; i += 2 )
            CV_Assert(i < CV_IO_MAX_IMAGE_PARAMS*2); // Limit number of params for security reasons
    }
    cv::Mat img = cv::cvarrToMat(arr);
    if( CV_IS_IMAGE(arr) && ((const IplImage*)arr)->origin == IPL_ORIGIN_BL )
    {
        cv::Mat temp;
        cv::flip(img, temp, 0);
        img = temp;
    }
    cv::vector<uchar> buf;

    bool code = cv::imencode(ext, img, buf,
        i > 0 ? std::vector<int>(_params, _params+i) : std::vector<int>() );
    if( !code )
        return 0;
    CvMat* _buf = cvCreateMat(1, (int)buf.size(), CV_8U);
    memcpy( _buf->data.ptr, &buf[0], buf.size() );

    return _buf;
}

/* End of file. */
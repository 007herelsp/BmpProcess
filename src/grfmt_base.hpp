#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_

#include "bitstrm.hpp"

namespace cv
{

class BaseImageDecoder;
class BaseImageEncoder;
typedef Ptr<BaseImageEncoder> ImageEncoder;
typedef Ptr<BaseImageDecoder> ImageDecoder;

///////////////////////////////// base class for decoders ////////////////////////
class BaseImageDecoder
{
public:
    BaseImageDecoder();
    virtual ~BaseImageDecoder() {};

    int width() const { return m_width; };
    int height() const { return m_height; };
    virtual int type() const { return m_type; };

    virtual bool setSource( const string& filename );
    virtual bool readHeader() = 0;
    virtual bool readData( Mat& img ) = 0;

    virtual size_t signatureLength() const;
    virtual bool checkSignature( const string& signature ) const;
    virtual ImageDecoder newDecoder() const;

protected:
    int  m_width;  // width  of the image ( filled by readHeader )
    int  m_height; // height of the image ( filled by readHeader )
    int  m_type;
    string m_filename;
    string m_signature;
    Mat m_buf;
    bool m_buf_supported;
};


///////////////////////////// base class for encoders ////////////////////////////
class BaseImageEncoder
{
public:
    BaseImageEncoder();
    virtual ~BaseImageEncoder() {};
    virtual bool isFormatSupported( int depth ) const;

    virtual bool setDestination( const string& filename );
    virtual bool write( const Mat& img, const vector<int>& params ) = 0;

    virtual string getDescription() const;
    virtual ImageEncoder newEncoder() const;

    virtual void throwOnEror() const;

protected:
    string m_description;

    string m_filename;
    vector<uchar>* m_buf;
    bool m_buf_supported;

    string m_last_error;
};

}

#endif/*_GRFMT_BASE_H_*/

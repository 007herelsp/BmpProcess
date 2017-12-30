
#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_


#include "utils.h"
#include "bitstrm.h"

#ifndef _MAX_PATH
    #define _MAX_PATH    1024
#endif


///////////////////////////////// base class for readers ////////////////////////
class   GrFmtReader
{
public:
    
    GrFmtReader( const char* filename );
    virtual ~GrFmtReader();

    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    bool  IsColor()   { return m_iscolor; };
    int   GetDepth()  { return m_bit_depth; };
    void  UseNativeDepth( bool yes ) { m_native_depth = yes; };
    bool  IsFloat()   { return m_isfloat; };

    virtual bool  ReadHeader() = 0;
    virtual bool  ReadData( uchar* data, int step, int color ) = 0;
    virtual void  Close();

protected:

    bool    m_iscolor;
    int     m_width;    // width  of the image ( filled by ReadHeader )
    int     m_height;   // height of the image ( filled by ReadHeader )
    int     m_bit_depth;// bit depth per channel (normally 8)
    char    m_filename[_MAX_PATH]; // filename
    bool    m_native_depth;// use the native bit depth of the image
    bool    m_isfloat;  // is image saved as float or double?
};


///////////////////////////// base class for writers ////////////////////////////
class   GrFmtWriter
{
public:

    GrFmtWriter( const char* filename );
    virtual ~GrFmtWriter() {};
    virtual bool  IsFormatSupported( int depth );
    virtual bool  WriteImage( const uchar* data, int step,
                              int width, int height, int depth, int channels ) = 0;
protected:
    char    m_filename[_MAX_PATH]; // filename
};


#endif/*_GRFMT_BASE_H_*/

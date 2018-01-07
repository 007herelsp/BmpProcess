
#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_

#include "bitstrm.h"

#ifndef _MAX_PATH
    #define _MAX_PATH    1024
#endif

class   GrFmtReader
{
public:
    
    GrFmtReader( const char* filename );
    virtual ~GrFmtReader();

    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    virtual bool  ReadHeader() = 0;
    virtual bool  ReadData( uchar* data, int step, int color ) = 0;
    virtual void  Close();

protected:

    bool    m_iscolor;
    int     m_width;    
    int     m_height;  
    int     m_bit_depth;
    char    m_filename[_MAX_PATH]; // filename
};

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

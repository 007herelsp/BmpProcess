#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_
#include "bitstrm.h"

#ifndef _MAX_PATH
#define _MAX_PATH    1024
#endif

enum BmpCompression
{
    BMP_RGB = 0
};


struct PaletteEntry
{
    unsigned char b, g, r, a;
};

class BmpReader 
{
public:
    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    
    BmpReader( const char* filename );
    ~BmpReader();
    
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:
    
    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
    bool    m_iscolor;
    int     m_width;
    int     m_height;
    int     m_bit_depth;
    char    m_filename[_MAX_PATH]; // filename
};

class BmpWriter
{
public:
    
    BmpWriter( const char* filename );
    ~BmpWriter();
    bool  IsFormatSupported( int depth );
    
    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int depth, int channels );
protected:

    WLByteStream  m_strm;
    char    m_filename[_MAX_PATH]; // filename
};


#endif/*_GRFMT_BMP_H_*/

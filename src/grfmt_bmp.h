

#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_

#include "grfmt_base.h"

enum BmpCompression
{
    BMP_RGB = 0
};


struct PaletteEntry
{
    unsigned char b, g, r, a;
};

// Windows Bitmap reader
class GrFmtBmpReader : public GrFmtReader
{
public:
    
    GrFmtBmpReader( const char* filename );
    ~GrFmtBmpReader();
    
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:
    
    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
};


// ... writer
class GrFmtBmpWriter : public GrFmtWriter
{
public:
    
    GrFmtBmpWriter( const char* filename );
    ~GrFmtBmpWriter();
    
    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int depth, int channels );
protected:

    WLByteStream  m_strm;
};


#endif/*_GRFMT_BMP_H_*/


#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_

#include "grfmt_base.hpp"

namespace cv
{
	
	
	int validateToInt(size_t step);
	
	struct PaletteEntry
	{
		unsigned char b, g, r, a;
	};
	
#define WRITE_PIX( ptr, clr )       \
		(((uchar*)(ptr))[0] = (clr).b,	\
		 ((uchar*)(ptr))[1] = (clr).g,	\
		 ((uchar*)(ptr))[2] = (clr).r)
	
#define  descale(x,n)  (((x) + (1 << ((n)-1))) >> (n))
#define  saturate(x)   (uchar)(((x) & ~255) == 0 ? (x) : ~((x)>>31))
	
	void icvCvt_BGR2Gray_8u_C3C1R( const uchar* bgr, int bgr_step,
								   uchar* gray, int gray_step,
								   CvSize size, int swap_rb=0 );
	
	void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );
	void  CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries );
	


enum BmpCompression
{
    BMP_RGB = 0,
    BMP_RLE8 = 1,
    BMP_RLE4 = 2,
    BMP_BITFIELDS = 3
};


// Windows Bitmap reader
class BmpDecoder : public BaseImageDecoder
{
public:

    BmpDecoder();
    ~BmpDecoder();

    bool  readData( Mat& img );
    bool  readHeader();
    void  close();

    ImageDecoder newDecoder() const;

protected:

    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_origin;
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
};


// ... writer
class BmpEncoder : public BaseImageEncoder
{
public:
    BmpEncoder();
    ~BmpEncoder();

    bool  write( const Mat& img, const vector<int>& params );

    ImageEncoder newEncoder() const;
};


}





#endif/*_GRFMT_BMP_H_*/

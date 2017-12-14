
#ifndef _UTILS_H_
#define _UTILS_H_

int validateToInt(size_t step);

struct PaletteEntry
{
    unsigned char b, g, r, a;
};

#define WRITE_PIX( ptr, clr )       \
    (((uchar*)(ptr))[0] = (clr).b,  \
     ((uchar*)(ptr))[1] = (clr).g,  \
     ((uchar*)(ptr))[2] = (clr).r)

#define  descale(x,n)  (((x) + (1 << ((n)-1))) >> (n))
#define  saturate(x)   (uchar)(((x) & ~255) == 0 ? (x) : ~((x)>>31))

void icvCvt_BGR2Gray_8u_C3C1R( const uchar* bgr, int bgr_step,
                               uchar* gray, int gray_step,
                               CvSize size, int swap_rb=0 );
void icvCvt_BGRA2Gray_8u_C4C1R( const uchar* bgra, int bgra_step,
                                uchar* gray, int gray_step,
                                CvSize size, int swap_rb=0 );

void icvCvt_BGRA2BGR_8u_C4C3R( const uchar* bgra, int bgra_step,
                               uchar* bgr, int bgr_step,
                               CvSize size, int swap_rb=0 );

void icvCvt_BGR5652Gray_8u_C2C1R( const uchar* bgr565, int bgr565_step,
                                  uchar* gray, int gray_step, CvSize size );

void icvCvt_BGR5652BGR_8u_C2C3R( const uchar* bgr565, int bgr565_step,
                                 uchar* bgr, int bgr_step, CvSize size );


void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );
bool  IsColorPalette( PaletteEntry* palette, int bpp );
void  CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries );
uchar* FillUniColor( uchar* data, uchar*& line_end, int step, int width3,
                     int& y, int height, int count3, PaletteEntry clr );
uchar* FillUniGray( uchar* data, uchar*& line_end, int step, int width3,
                     int& y, int height, int count3, uchar clr );

uchar* FillColorRow8( uchar* data, uchar* indices, int len, PaletteEntry* palette );
uchar* FillGrayRow8( uchar* data, uchar* indices, int len, uchar* palette );

#endif/*_UTILS_H_*/

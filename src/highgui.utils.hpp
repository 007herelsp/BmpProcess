
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

void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );
void  CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries );

#endif/*_UTILS_H_*/

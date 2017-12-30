

#ifndef _UTILS_H_
#define _UTILS_H_

struct PaletteEntry 
{
    unsigned char b, g, r, a;
};





void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );


#endif/*_UTILS_H_*/

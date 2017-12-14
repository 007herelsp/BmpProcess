

#include "highgui/precomp.hpp"
#include "highgui.utils.hpp"

int validateToInt(size_t sz)
{
    int valueInt = (int)sz;
    CV_Assert((size_t)valueInt == sz);
    return valueInt;
}

#define  SCALE  14
#define  cR  (int)(0.299*(1 << SCALE) + 0.5)
#define  cG  (int)(0.587*(1 << SCALE) + 0.5)
#define  cB  ((1 << SCALE) - cR - cG)

void icvCvt_BGR2Gray_8u_C3C1R( const uchar* rgb, int rgb_step,
                               uchar* gray, int gray_step,
                               CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; gray += gray_step )
    {
        for( i = 0; i < size.width; i++, rgb += 3 )
        {
            int t = descale( rgb[swap_rb]*cB + rgb[1]*cG + rgb[swap_rb^2]*cR, SCALE );
            gray[i] = (uchar)t;
        }

        rgb += rgb_step - size.width*3;
    }
}



void icvCvt_BGRA2Gray_8u_C4C1R( const uchar* rgba, int rgba_step,
                                uchar* gray, int gray_step,
                                CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; gray += gray_step )
    {
        for( i = 0; i < size.width; i++, rgba += 4 )
        {
            int t = descale( rgba[swap_rb]*cB + rgba[1]*cG + rgba[swap_rb^2]*cR, SCALE );
            gray[i] = (uchar)t;
        }

        rgba += rgba_step - size.width*4;
    }
}







void icvCvt_BGRA2BGR_8u_C4C3R( const uchar* bgra, int bgra_step,
                               uchar* bgr, int bgr_step,
                               CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, bgr += 3, bgra += 4 )
        {
            uchar t0 = bgra[swap_rb], t1 = bgra[1];
            bgr[0] = t0; bgr[1] = t1;
            t0 = bgra[swap_rb^2]; bgr[2] = t0;
        }
        bgr += bgr_step - size.width*3;
        bgra += bgra_step - size.width*4;
    }
}



typedef unsigned short ushort;



void icvCvt_BGR5652Gray_8u_C2C1R( const uchar* bgr565, int bgr565_step,
                                  uchar* gray, int gray_step, CvSize size )
{
    int i;
    for( ; size.height--; gray += gray_step, bgr565 += bgr565_step )
    {
        for( i = 0; i < size.width; i++ )
        {
            int t = descale( ((((ushort*)bgr565)[i] << 3) & 0xf8)*cB +
                             ((((ushort*)bgr565)[i] >> 3) & 0xfc)*cG +
                             ((((ushort*)bgr565)[i] >> 8) & 0xf8)*cR, SCALE );
            gray[i] = (uchar)t;
        }
    }
}


void icvCvt_BGR5652BGR_8u_C2C3R( const uchar* bgr565, int bgr565_step,
                                 uchar* bgr, int bgr_step, CvSize size )
{
    int i;
    for( ; size.height--; bgr565 += bgr565_step )
    {
        for( i = 0; i < size.width; i++, bgr += 3 )
        {
            int t0 = (((ushort*)bgr565)[i] << 3) & 0xf8;
            int t1 = (((ushort*)bgr565)[i] >> 3) & 0xfc;
            int t2 = (((ushort*)bgr565)[i] >> 8) & 0xf8;
            bgr[0] = (uchar)t0; bgr[1] = (uchar)t1; bgr[2] = (uchar)t2;
        }
        bgr += bgr_step - size.width*3;
    }
}




void CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries )
{
    int i;
    for( i = 0; i < entries; i++ )
    {
        icvCvt_BGR2Gray_8u_C3C1R( (uchar*)(palette + i), 0, grayPalette + i, 0, cvSize(1,1) );
    }
}


void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative )
{
    int i, length = 1 << bpp;
    int xor_mask = negative ? 255 : 0;

    for( i = 0; i < length; i++ )
    {
        int val = (i * 255/(length - 1)) ^ xor_mask;
        palette[i].b = palette[i].g = palette[i].r = (uchar)val;
        palette[i].a = 0;
    }
}


bool  IsColorPalette( PaletteEntry* palette, int bpp )
{
    int i, length = 1 << bpp;

    for( i = 0; i < length; i++ )
    {
        if( palette[i].b != palette[i].g ||
            palette[i].b != palette[i].r )
            return true;
    }

    return false;
}


uchar* FillUniColor( uchar* data, uchar*& line_end,
                     int step, int width3,
                     int& y, int height,
                     int count3, PaletteEntry clr )
{
    do
    {
        uchar* end = data + count3;

        if( end > line_end )
            end = line_end;

        count3 -= (int)(end - data);

        for( ; data < end; data += 3 )
        {
            WRITE_PIX( data, clr );
        }

        if( data >= line_end )
        {
            line_end += step;
            data = line_end - width3;
            if( ++y >= height  ) break;
        }
    }
    while( count3 > 0 );

    return data;
}


uchar* FillUniGray( uchar* data, uchar*& line_end,
                    int step, int width,
                    int& y, int height,
                    int count, uchar clr )
{
    do
    {
        uchar* end = data + count;

        if( end > line_end )
            end = line_end;

        count -= (int)(end - data);

        for( ; data < end; data++ )
        {
            *data = clr;
        }

        if( data >= line_end )
        {
            line_end += step;
            data = line_end - width;
            if( ++y >= height  ) break;
        }
    }
    while( count > 0 );

    return data;
}


uchar* FillColorRow8( uchar* data, uchar* indices, int len, PaletteEntry* palette )
{
    uchar* end = data + len*3;
    while( (data += 3) < end )
    {
        *((PaletteEntry*)(data-3)) = palette[*indices++];
    }
    PaletteEntry clr = palette[indices[0]];
    WRITE_PIX( data - 3, clr );
    return data;
}


uchar* FillGrayRow8( uchar* data, uchar* indices, int len, uchar* palette )
{
    int i;
    for( i = 0; i < len; i++ )
    {
        data[i] = palette[indices[i]];
    }
    return data + len;
}



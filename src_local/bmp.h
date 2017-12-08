#ifndef BMP_H
#define BMP_H

#include "vostype.h"
#pragma pack(1)
typedef struct bmp_header
{
    //bmp header
    U8 Signatue[2]; // B  M
    U32 FileSize;   //文件大小
    U16 Reserv1;
    U16 Reserv2;
    U32 FileOffset; //文件头偏移量

    //DIB header
    U32 DIBHeaderSize; //DIB头大小
    U32 ImageWidth;    //文件宽度
    U32 ImageHight;    //文件高度
    U16 Planes;
    U16 BPP; //每个相素点的位数
    U32 Compression;
    U32 ImageSize; //图文件大小
    U32 XPPM;
    U32 YPPM;
    U32 CCT;
    U32 ICC;
}BmpHeader, *lpBmpHeader;


#pragma pack()

#endif // BMP_H

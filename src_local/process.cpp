#include "common.h"
#include "bmp.h"
#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>

using namespace std;
//直接查找彩图中颜色一致的能构成水平直线
std::vector<Line> find_horizontal_line(U8 *pSrcImg, const BmpHeader *ptrBmpHdr, U32 minlength)
{
    vector<Line> ret;
    VOS_ASSERT(NULL != pSrcImg);
    VOS_ASSERT(NULL != ptrBmpHdr);
    size_t height = ptrBmpHdr->ImageHight;
    size_t width = ptrBmpHdr->ImageWidth;
    U32 step = ptrBmpHdr->BPP >> 3;
    int srclineByte = (width * ptrBmpHdr->BPP / 8 + 3) / 4 * 4;
    int lineByte = (width + 3) / 4 * 4;

    minlength = minlength >= 2 ? minlength : 2;
    size_t rowoffset;
    size_t nextindex;
    size_t index;
    U8 B,G,R,B1,G1,R1;
    int iCmpRet = 0;
    for (size_t row = 0; row < height; row++)
    {
        rowoffset = row * srclineByte;
        for (size_t col = 0; col < width; col++)
        {
            index = rowoffset + step * col;

            B = pSrcImg[index];
            G = pSrcImg[index + 1];
            R = pSrcImg[index + 2];
            int cun = 0;
            int next = 0;
            for (next = col+1; next + col < width; next++)
            {
                nextindex = rowoffset+ step * next;

                iCmpRet = memcmp((pSrcImg+index), pSrcImg+nextindex, step);
                if(0 == iCmpRet)
                {
                    cun++;
                    continue;
                }
                else
                {
                    break; 
                }
            }

            //满足直线条件
            if(cun >=minlength)
            {
                Line line;
                line.x0 = col;
                line.y0 = row;
                line.x1 = next;
                line.y1 = row;
                ret.push_back(line);

                col += cun;
            }
            else
            {
                //不满足找下一个点
            }
        }
    }

    return ret;
}

U8 *bgr24_2_gray(U8 *pSrcImg, const BmpHeader *ptrBmpHdr, U32 &imgSize)
{
    VOS_ASSERT(NULL != pSrcImg);
    VOS_ASSERT(NULL != ptrBmpHdr);

    size_t height = ptrBmpHdr->ImageHight;
    size_t width = ptrBmpHdr->ImageWidth;

    //int lineByte=(width * ptrBmpHdr->BPP/8+3)/4*4;
    int srclineByte = (width * ptrBmpHdr->BPP / 8 + 3) / 4 * 4;
    int lineByte = (width + 3) / 4 * 4;
    imgSize = ptrBmpHdr->ImageHight * lineByte;
    U8 *pGray8Buf = (U8 *)VOS_MALLOC(imgSize);
    U32 step = ptrBmpHdr->BPP >> 3;
    size_t offset = 0;
    U8 data = 0;
    U8 R;
    U8 G;
    U8 B;
    if (NULL != pGray8Buf)
    {
        size_t index = 0;

        for (size_t row = 0; row < height; row++)
        {
            for (size_t col = 0; col < width; col++)
            {
                index = row * srclineByte + step * col;

                B = pSrcImg[index];
                G = pSrcImg[index + 1];
                R = pSrcImg[index + 2];

                //灰度
                // data = (U8)((ptBgrHdr->R * 299 + ptBgrHdr->G * 587 + ptBgrHdr->B * 114) / 1000);
                //为了利于阀值分割，需要使用MIN方法
                data = MAX(B, MAX(G, R));

                pGray8Buf[row * lineByte + col] = data;
            }
        }
    }
    else
    {
        imgSize = 0;
    }

    return pGray8Buf;
}

U32 write_bmp_8(const char *name, U8 *data, U32 width, U32 height, U32 size)
{
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (0 > fd)
    {
        perror("open file fail");
        return ERR_1;
    }
    BmpHeader hdr = {0};
    hdr.Signatue[0] = 'B';
    hdr.Signatue[1] = 'M';
    hdr.ImageHight = height;
    hdr.ImageWidth = width;
    hdr.BPP = 8;
    hdr.Planes = 1;
    //hdr.ImageSize = height* width;
    hdr.DIBHeaderSize = 40;
    hdr.FileOffset = sizeof(BmpHeader) + (256 << 2);
    hdr.FileSize = hdr.FileOffset + size;
    hdr.CCT = 0;
    hdr.ICC = 0;
    int wc = write(fd, &hdr, sizeof(BmpHeader));

    //写入调色板信息

    unsigned char j = 0;
    unsigned char z = 0;
    for (int i = 0; i < 1024; i += 4)
    {
        write(fd, (char *)&j, sizeof(unsigned char));
        write(fd, (char *)&j, sizeof(unsigned char));
        write(fd, (char *)&j, sizeof(unsigned char));
        write(fd, (char *)&z, sizeof(unsigned char));
        j++;
    }

    write(fd, data, size);

    VOS_CLOSE(fd);
}

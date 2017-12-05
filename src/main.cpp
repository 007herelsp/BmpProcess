#include "common.h"
#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

int otsu3(unsigned char *data, int width, int height)
{
    //histogram
    float histogram[256] = {0};
    for (int i = 0; i < height; i++)
    {
        unsigned char *p = (unsigned char *)data + width * i;
        for (int j = 0; j < width; j++)
        {
            histogram[*p++]++;
        }
    }
    //normalize histogram
    int size = height * width;
    for (int i = 0; i < 256; i++)
    {
        histogram[i] = histogram[i] / size;
    }

    //average pixel value
    float avgValue = 0;
    for (int i = 0; i < 256; i++)
    {
        avgValue += i * histogram[i]; //整幅图像的平均灰度
    }

    int threshold;
    float maxVariance = 0;
    float w = 0, u = 0;
    for (int i = 0; i < 256; i++)
    {
        w += histogram[i];     //假设当前灰度i为阈值, 0~i 灰度的像素(假设像素值在此范围的像素叫做前景像素) 所占整幅图像的比例
        u += i * histogram[i]; // 灰度i 之前的像素(0~i)的平均灰度值： 前景像素的平均灰度值

        float t = avgValue * w - u;
        float variance = t * t / (w * (1 - w));
        if (variance > maxVariance)
        {
            maxVariance = variance;
            threshold = i;
        }
    }

    return threshold;
}

int otsu2(unsigned char *data, int step, int width, int height)
{
    assert(NULL != data);
    int x = 0, y = 0;
    int pixelCount[256] = {0};
    float pixelPro[256] = {0};
    int i, j, pixelSum = width * height, threshold = 0;
    int index = 0;
    pBGRHDR ptBgrHdr;
    //统计灰度级中每个像素在整幅图像中的个数
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            // pixelCount[data[i * step + j]]++;

            // index = (i * width + j) * 3;

            // ptBgrHdr = (pBGRHDR)(data + index);
            // pixelCount[ptBgrHdr->R]++;

            pixelCount[data[i * width + j]]++;
        }
    }

    //计算每个像素在整幅图像中的比例
    for (i = 0; i < 256; i++)
    {
        pixelPro[i] = (float)(pixelCount[i]) / (float)(pixelSum);
    }

    //经典ostu算法,得到前景和背景的分割
    //遍历灰度级[0,255],计算出方差最大的灰度值,为最佳阈值
    float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
    for (i = 0; i < 256; i++)
    {
        w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;

        for (j = 0; j < 256; j++)
        {
            if (j <= i) //背景部分
            {
                //以i为阈值分类，第一类总的概率
                w0 += pixelPro[j];
                u0tmp += j * pixelPro[j];
            }
            else //前景部分
            {
                //以i为阈值分类，第二类总的概率
                w1 += pixelPro[j];
                u1tmp += j * pixelPro[j];
            }
        }

        u0 = u0tmp / w0;   //第一类的平均灰度
        u1 = u1tmp / w1;   //第二类的平均灰度
        u = u0tmp + u1tmp; //整幅图像的平均灰度
        //计算类间方差
        deltaTmp = w0 * (u0 - u) * (u0 - u) + w1 * (u1 - u) * (u1 - u);
        //找出最大类间方差以及对应的阈值
        if (deltaTmp > deltaMax)
        {
            deltaMax = deltaTmp;
            threshold = i;
        }
    }
    //返回最佳阈值;
    return threshold;
}

int otsu(unsigned char *image, int rows, int cols, int x0, int y0, int dx, int dy, int vvv)
{

    unsigned char *np;      // 图像指针
    int thresholdValue = 1; // 阈值
    int ihist[256] = {0};   // 图像直方图，256个点

    int i, j, k; // various counters
    int n, n1, n2, gmin, gmax;
    double m1, m2, sum, csum, fmax, sb;

    gmin = 255;
    gmax = 0;
    // 生成直方图
    for (i = y0 + 1; i < y0 + dy - 1; i++)
    {
        np = &image[i * cols + x0 + 1];
        for (j = x0 + 1; j < x0 + dx - 1; j++)
        {
            ihist[*np]++;
            if (*np > gmax)
                gmax = *np;
            if (*np < gmin)
                gmin = *np;
            np++; /* next pixel */
        }
    }

    // set up everything
    sum = csum = 0.0;
    n = 0;

    for (k = 0; k <= 255; k++)
    {
        sum += (double)k * (double)ihist[k]; /* x*f(x) 质量矩*/
        n += ihist[k];                       /* f(x)   质量  */
    }

    if (!n)
    {
        // if n has no value, there is problems...
        fprintf(stderr, "NOT NORMAL thresholdValue = 160\n");
        return (160);
    }

    // do the otsu global thresholding method
    fmax = -1.0;
    n1 = 0;
    for (k = 0; k < 255; k++)
    {
        n1 += ihist[k];
        if (!n1)
        {
            continue;
        }
        n2 = n - n1;
        if (n2 == 0)
        {
            break;
        }
        csum += (double)k * ihist[k];
        m1 = csum / n1;
        m2 = (sum - csum) / n2;
        sb = (double)n1 * (double)n2 * (m1 - m2) * (m1 - m2);
        /* bbg: note: can be optimized. */
        if (sb > fmax)
        {
            fmax = sb;
            thresholdValue = k;
        }
    }

    // at this point we have our thresholding value

    // debug code to display thresholding values
    if (vvv & 1)
        fprintf(stderr, "# OTSU: thresholdValue = %d gmin=%d gmax=%d\n",
                thresholdValue, gmin, gmax);

    return (thresholdValue);
}

#define XMAXHISTLEVEL 256
void GetMaxHistDiff(const U32 *pHist, float fArea, int *nMaxIndex, float *fMaxDiff)
{
    *nMaxIndex = -1;
    *fMaxDiff = -1;
    float fCount, fSumPels, fCountPels = 0;
    float w0, w1, u0, u1, fRlTmp;
    long SumPelsArray[XMAXHISTLEVEL] = {0};
    int i;

    if (pHist == NULL || fArea < 0)
        return;

    for (i = 1; i < XMAXHISTLEVEL; ++i)
    {
        SumPelsArray[i] = pHist[i] * i;
        fCountPels += SumPelsArray[i];
    }

    fCount = fSumPels = 0;
    for (i = 1; i < XMAXHISTLEVEL; ++i)
    {
        if (pHist[i])
        {
            fCount += pHist[i];
            fSumPels += SumPelsArray[i];
            w0 = fCount / fArea;
            u0 = fSumPels / fCount;
            w1 = 1 - w0;
            if (fArea > fCount)
                u1 = (fCountPels - fSumPels) / (fArea - fCount);
            else
                u1 = 0;
            fRlTmp = w0 * w1 * (u0 - u1) * (u0 - u1);
            if (*fMaxDiff < fRlTmp)
            {
                *fMaxDiff = fRlTmp;
                *nMaxIndex = i;
            }
        }
    }
}

void writeBmp8(const char *name, struct bmp_header *header, U8 *data, int len)
{
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0)
    {
        perror("open rgb565 fail");
        return;
    }

    int wc = write(fd, header, sizeof(struct bmp_header));

    //灰度图像需要调色板

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

    write(fd, data, len);

    VOS_CLOSE(fd);
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        return -1;
    }

    int fd;
    int dest_fd;
    fd = open(argv[1], O_RDONLY);
    if (-1 == fd)
    {
        perror("open bmp file fail");
        return -2;
    }

    dest_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (dest_fd < 0)
    {
        perror("open rgb565 fail");
        return -3;
    }
    int value = atoi(argv[3]);

    struct bmp_header header = {0};

    int ret;

    ret = read(fd, &header, sizeof(BmpHeader));
    assert(ret == sizeof(BmpHeader));

    printf(" Signatue[0]      : %c  \n ", header.Signatue[0]);
    printf(" Signatue[1]      : %c  \n ", header.Signatue[1]);
    printf(" FileSize         : %d  \n ", header.FileSize);
    printf(" Reserv1          : %d  \n ", header.Reserv1);
    printf(" Reserv2          : %d  \n ", header.Reserv2);
    printf(" FileOffset       : %d  \n ", header.FileOffset);
    printf(" DIBHeaderSize    : %d  \n ", header.DIBHeaderSize);
    printf(" ImageWidth       : %d  \n ", header.ImageWidth);
    printf(" ImageHight       : %d  \n ", header.ImageHight);
    printf(" Planes           : %d  \n ", header.Planes);
    printf(" BPP              : %d  \n ", header.BPP);
    printf(" Compression      : %d  \n ", header.Compression);
    printf(" ImageSize        : %d  \n ", header.ImageSize);
    printf(" XPPM             : %d  \n ", header.XPPM);
    printf(" YPPM             : %d  \n ", header.YPPM);
    printf(" CCT              : %d  \n ", header.CCT);
    printf(" ICC              : %d  \n ", header.ICC);

    //改变地方

    int wc = write(dest_fd, &header, sizeof(BmpHeader));
    assert(wc == sizeof(BmpHeader));
    if (header.ImageSize == 0)
    {
        header.ImageSize = header.FileSize - header.FileOffset;
    }

    U8 *buffer = (U8 *)malloc(header.ImageSize);
    if (NULL == buffer)
    {
        VOS_CLOSE(fd);
        return -1;
    }
    memset(buffer, 0, header.ImageSize);

    int rc = read(fd, buffer, header.ImageSize);
    VOS_ASSERT(rc == header.ImageSize);

    VOS_CLOSE(fd);

    wc = write(dest_fd, buffer, header.ImageSize);

    U32 size;
    U8 *ptrgb = bgr24_2_gray(buffer, &header, size);
    if (NULL != ptrgb)
    {
        write_bmp_8("ct.bmp", ptrgb, header.ImageWidth, header.ImageHight, size);


        VOS_FREE(ptrgb);
    }

    std::vector<Line> lines = find_horizontal_line(buffer, &header,10);
 
printf("lines:%d\n", lines.size());
      for (auto it = lines.begin(); it != lines.end(); it ++) {
        Line l = *it;
        printf("%d,%d,%d,%d\n", l.x0,l.y0,l.x1,l.y1);
    }
    return 0;

    int row, col;
    char *p = NULL;

    // for (row = 0; row < header.ImageHight; row++)
    // {
    //     for (col = 0; col <header.ImageWidth; col++)
    //     {
    //         p = buffer + (row * header.ImageWidth + col) * 3;
    //         *p = 255;
    //         *(p + 1) = 255;
    //         *(p + 2) = 255;
    //     }
    // }

    U8 data;
    pBGRHDR ptBgrHdr = NULL;
    U32 offset = 0;
    U32 cun = 0;
    int index = 0;
    U8 *bitmap = (U8 *)malloc(header.ImageHight * header.ImageWidth);
    assert(NULL != bitmap);
    U32 bitmaplen = header.ImageHight * header.ImageWidth;

    for (row = 0; row < header.ImageHight; row++)
    {
        for (col = 0; col < header.ImageWidth; col++)
        {
            index = 3 * (row * header.ImageWidth + col);

            ptBgrHdr = (pBGRHDR)(buffer + index);

            //灰度
            // data = (U8)((ptBgrHdr->R * 299 + ptBgrHdr->G * 587 + ptBgrHdr->B * 114) / 1000);
            //为了利于阀值分割，需要使用MIN方法
            data = MIN(ptBgrHdr->R, MIN(ptBgrHdr->G, ptBgrHdr->B));
            //data = ptBgrHdr->B;
            // ptBgrHdr->B = ptBgrHdr->G = ptBgrHdr->R = data;
            //bitmap[offset++] = data;
            //二值化
            // data = (ptBgrHdr->B + ptBgrHdr->G + ptBgrHdr->R) / 3;
            // char r = (char)(0.7 * ptBgrHdr->R + 0.2 * ptBgrHdr->G + 0.1 * ptBgrHdr->B);
            // ptBgrHdr->B = ptBgrHdr->G = ptBgrHdr->R = data;
            // if (data > r)
            // {
            //     ptBgrHdr->B = ptBgrHdr->G = ptBgrHdr->R = 255;
            // }
            // else
            // {
            //     ptBgrHdr->B = ptBgrHdr->G = ptBgrHdr->R = 0;
            // }

            // cun += write(dest_fd, ptBgrHdr, 3);
        }
        //break;
    }

    int width = header.ImageWidth;
    int height = header.ImageHight;
    int threshold = otsu2(bitmap, 3, width, height);
    threshold = otsu3(bitmap, width, height);

    // threshold = otsu2(bitmap, 1, width, height);
    threshold = otsu3(bitmap, width, height);

    // 二值化
    printf("final threshold value : %d", threshold);
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            index = (row * width + col) * 3;
            ptBgrHdr = (pBGRHDR)(buffer + index);

            //灰度

            //为了利于阀值分割，需要使用MIN方法
            int gray = MIN(ptBgrHdr->R, MIN(ptBgrHdr->G, ptBgrHdr->B));

            if (gray >= threshold)
            {
                gray = 255;
            }
            else
            {
                gray = 0;
            }
            //buffer[index] = buffer[index + 1] = buffer[index + 2] = gray;

            //ptBgrHdr->B = ptBgrHdr->G = ptBgrHdr->R = gray;
        }
    }

    ////////////

    write(dest_fd, buffer, header.ImageSize);
    // if (header.ImageSize - header.ImageHight * header.ImageWidth * 3 > 0)
    // {
    //     write(dest_fd, ((char *)buffer) + header.ImageHight * header.ImageWidth * 3, header.ImageSize - header.ImageHight * header.ImageWidth * 3);
    // }

    VOS_FREE(buffer);
    VOS_CLOSE(dest_fd);
    // bitmaplen = 512*512;
    header.FileOffset = header.FileOffset + 256 * 4;
    header.FileSize = header.FileOffset + bitmaplen; //256*4是调色板占的字节数
    //263222
    header.Planes = 1;
    header.BPP = 8;
    //header.ImageWidth = 512;
    //header.ImageHight = 512;
    header.XPPM = 0;
    header.YPPM = 0;

    header.ImageSize = bitmaplen;
    header.CCT = 256;
    header.ICC = 256;

    writeBmp8("D:\\opensrc\\bmp_process\\build\\8.bmp", &header, bitmap, bitmaplen);

    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            index = (row * width + col);

            //为了利于阀值分割，需要使用MIN方法
            int gray = bitmap[index];

            if (gray >= threshold)
            {
                gray = 255;
            }
            else
            {
                gray = 0;
            }
            //buffer[index] = buffer[index + 1] = buffer[index + 2] = gray;

            bitmap[index] = gray;
        }
    }

    writeBmp8("D:\\opensrc\\bmp_process\\build\\8_b.bmp", &header, bitmap, bitmaplen);

    return 0;
}

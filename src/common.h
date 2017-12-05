#ifndef __COMMON_H__
#define __COMMON_H__

#include "vostype.h"

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#define VOS_ASSERT assert
#define VOS_MALLOC malloc

#define RGB(r, g, b) ((r + g + b) / 3)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define ERR_OK 0
#define ERR_1 1
#define IVAILDFD -1

#ifndef VOS_CLOSE
#define VOS_CLOSE(fd)  \
    {                  \
        close(fd);     \
        fd = IVAILDFD; \
    }
#endif /*VOS_CLOSE*/

#ifndef VOS_FREE
#define VOS_FREE(addr)    \
    {                     \
        if (NULL != addr) \
        {                 \
            free(addr);   \
            addr = NULL;  \
        }                 \
    }

#endif /*VOS_FREE*/

#pragma pack(1)
typedef struct stBGRHdr
{
    U8 B;
    U8 G;
    U8 R;
} BGRHDR, *pBGRHDR;

typedef struct stPoint
{
    U32 x;
    U32 y;
} Point, *lpPoint;

typedef struct stLine
{
    U32 x0;
    U32 y0;
    U32 x1;
    U32 y1;
} Line, *lpLine;

#pragma pack()
#endif /*__COMMON_H__*/

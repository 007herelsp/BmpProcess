#ifndef __COMMON_H__
#define __COMMON_H__

#include "vostype.h"

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#define VOS_ASSERT assert
#define VOS_MALLOC malloc
#define VOS_ALLOC alloca

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



#ifndef CV_EXTERN_C
#  ifdef __cplusplus
#    define CV_EXTERN_C extern "C"
#    define CV_DEFAULT(val) = val
#  else
#    define CV_EXTERN_C
#    define CV_DEFAULT(val)
#  endif
#endif
 


#pragma pack()
#endif /*__COMMON_H__*/

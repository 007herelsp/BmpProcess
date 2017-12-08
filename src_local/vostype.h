#ifndef VOS_TYPE_H
#define VOS_TYPE_H

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef int INT32;
typedef signed char schar;
#define VOID void

typedef U8 *LPBYTE;

typedef struct stPoint
{
    INT32 x;
    INT32 y;
} Point, *lpPoint;

typedef struct stLine
{
    INT32 x0;
    INT32 y0;
    INT32 x1;
    INT32 y1;
} Line, *lpLine;
 
#endif

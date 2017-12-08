#ifndef __CANNY_H__
#define __CANNY_H__
#include "vostype.h"

void GaussianSmooth(size_t sz, U8* pGray, U8* pResult, double sigma);
#endif
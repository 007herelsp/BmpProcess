#ifndef PROCESS_H
#define PROCESS_H
#include "vostype.h"
#include "bmp.h"
#include "common.h"
#include <vector>

U8* bgr24_2_gray(U8* pSrcImg, const BmpHeader* ptrBmpHdr, size_t& imgSize);
U32 write_bmp_8(const char *name, U8 *data, U32 width, U32 height, U32 size);

std::vector<Line> find_horizontal_line(U8* pSrcImg, const BmpHeader *ptrBmpHdr, U32 minlength);

#endif // PROCESS_H

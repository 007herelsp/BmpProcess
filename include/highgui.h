#ifndef _HIGH_GUI_
#define _HIGH_GUI_

#include "cxcore.h"
#include "cxmisc.h"

#define VOS_LOAD_IMAGE_COLOR       1

IplImage* LoadImage( const char* filename, int iscolor VOS_DEFAULT(VOS_LOAD_IMAGE_COLOR));

/* save image to file */
int SaveImage( const char* filename, const VOID* image );


#endif /* _HIGH_GUI_ */

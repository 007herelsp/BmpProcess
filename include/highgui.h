#ifndef _HIGH_GUI_
#define _HIGH_GUI_

#include "core.h"
#include "misc.h"

#define VOS_LOAD_IMAGE_COLOR       1

IplImage* LoadImage( const char* filename);

/* save image to file */
int SaveImage( const char* filename, const VOID* image );


#endif /* _HIGH_GUI_ */

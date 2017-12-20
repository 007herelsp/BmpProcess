
#ifndef __OPENCV_HIGHGUI_H__
#define __OPENCV_HIGHGUI_H__

#include "core_c.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


enum
{
/* 8bit, color or not */
    CV_LOAD_IMAGE_UNCHANGED  =-1,
/* 8bit, gray */
    CV_LOAD_IMAGE_GRAYSCALE  =0,
/* ?, color */
    CV_LOAD_IMAGE_COLOR      =1,
/* any depth, ? */
    CV_LOAD_IMAGE_ANYDEPTH   =2,
/* ?, any color */
    CV_LOAD_IMAGE_ANYCOLOR   =4
};

/* load image from file
  iscolor can be a combination of above flags where CV_LOAD_IMAGE_UNCHANGED
  overrides the other flags
  using CV_LOAD_IMAGE_ANYCOLOR alone is equivalent to CV_LOAD_IMAGE_UNCHANGED
  unless CV_LOAD_IMAGE_ANYDEPTH is specified images are converted to 8bit
*/
CVAPI(IplImage*) cvLoadImage( const char* filename, int iscolor CV_DEFAULT(CV_LOAD_IMAGE_COLOR));


/* save image to file */
CVAPI(int) cvSaveImage( const char* filename, const CvArr* image,
                        const int* params CV_DEFAULT(0) );


#ifdef __cplusplus
}
#endif

#endif

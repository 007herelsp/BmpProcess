#ifndef __OPENCV_IMGPROC_TYPES_C_H__
#define __OPENCV_IMGPROC_TYPES_C_H__

#include "opencv2/core/core_c.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Image smooth methods */
enum
{
    CV_BLUR_NO_SCALE =0,
    CV_BLUR  =1,
    CV_GAUSSIAN  =2,
    CV_MEDIAN =3,
    CV_BILATERAL =4
};

/* Filters used in pyramid decomposition */
enum
{
    CV_GAUSSIAN_5x5 = 7
};

/* Special filters */
enum
{
    CV_SCHARR =-1,
    CV_MAX_SOBEL_KSIZE =7
};

/* Constants for color conversion */
enum
{
  

    CV_BGR2GRAY    =6,
    CV_RGB2GRAY    =7,
    CV_GRAY2BGR    =8,
    CV_GRAY2RGB    =CV_GRAY2BGR,
 

    CV_COLORCVT_MAX  = CV_GRAY2RGB+1
};


/* Sub-pixel interpolation methods */
enum
{
    CV_INTER_NN        =0,
    CV_INTER_LINEAR    =1,
    CV_INTER_CUBIC     =2,
    CV_INTER_AREA      =3,
    CV_INTER_LANCZOS4  =4
};

/* ... and other image warping flags */
enum
{
    CV_WARP_FILL_OUTLIERS =8,
    CV_WARP_INVERSE_MAP  =16
};

/* Shapes of a structuring element for morphological operations */
enum
{
    CV_SHAPE_RECT      =0,
    CV_SHAPE_CROSS     =1,
    CV_SHAPE_ELLIPSE   =2,
    CV_SHAPE_CUSTOM    =100
};

/* Morphological operations */
enum
{
    CV_MOP_ERODE        =0,
    CV_MOP_DILATE       =1,
    CV_MOP_OPEN         =2,
    CV_MOP_CLOSE        =3,
    CV_MOP_GRADIENT     =4,
    CV_MOP_TOPHAT       =5,
    CV_MOP_BLACKHAT     =6
};



/* Contour retrieval modes */
enum
{
    CV_RETR_EXTERNAL=0,
    CV_RETR_LIST=1,
    CV_RETR_CCOMP=2,
    CV_RETR_TREE=3,
    CV_RETR_FLOODFILL=4
};

/* Contour approximation methods */
enum
{
    CV_CHAIN_CODE=0,
    CV_CHAIN_APPROX_NONE=1,
    CV_CHAIN_APPROX_SIMPLE=2,
    CV_CHAIN_APPROX_TC89_L1=3,
    CV_CHAIN_APPROX_TC89_KCOS=4,
    CV_LINK_RUNS=5
};

/*
Internal structure that is used for sequental retrieving contours from the image.
It supports both hierarchical and plane variants of Suzuki algorithm.
*/
typedef struct _CvContourScanner* CvContourScanner;



/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define  CV_INIT_3X3_DELTAS( deltas, step, nch )            \
    ((deltas)[0] =  (nch),  (deltas)[1] = -(step) + (nch),  \
     (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch),  \
     (deltas)[4] = -(nch),  (deltas)[5] =  (step) - (nch),  \
     (deltas)[6] =  (step), (deltas)[7] =  (step) + (nch))





/* Contour approximation algorithms */
enum
{
    CV_POLY_APPROX_DP = 0
};



/* Shape orientation */
enum
{
    CV_CLOCKWISE         =1,
    CV_COUNTER_CLOCKWISE =2
};




/* Threshold types */
enum
{
    CV_THRESH_BINARY      =0,  /* value = value > threshold ? max_value : 0       */
    CV_THRESH_BINARY_INV  =1,  /* value = value > threshold ? 0 : max_value       */
    CV_THRESH_TRUNC       =2,  /* value = value > threshold ? threshold : value   */
    CV_THRESH_TOZERO      =3,  /* value = value > threshold ? value : 0           */
    CV_THRESH_TOZERO_INV  =4,  /* value = value > threshold ? 0 : value           */
    CV_THRESH_MASK        =7,
    CV_THRESH_OTSU        =8  /* use Otsu algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values */
};

/* Adaptive threshold methods */
enum
{
    CV_ADAPTIVE_THRESH_MEAN_C  =0,
    CV_ADAPTIVE_THRESH_GAUSSIAN_C  =1
};


/* Canny edge detector flags */
enum
{
    CV_CANNY_L2_GRADIENT  =(1 << 31)
};


#ifdef __cplusplus
}
#endif

#endif

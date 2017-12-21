/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/


#ifndef _CV_H_
#define _CV_H_



#ifndef SKIP_INCLUDES
  #if defined(_CH_)
    #pragma package <chopencv>
    #include <chdl.h>
    LOAD_CHDL(cv)
  #endif
#endif

#include "cxcore.h"
#include "cvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************\
*                                    Image Processing                                    *
\****************************************************************************************/

/* Copies source 2D array inside of the larger destination array and
   makes a border of the specified type (IPL_BORDER_*) around the copied area. */
CVAPI(void) cvCopyMakeBorder( const CvArr* src, CvArr* dst, CvPoint offset,
                              int bordertype, CvScalar value CV_DEFAULT(cvScalarAll(0)));

#define CV_BLUR_NO_SCALE 0
#define CV_BLUR  1
#define CV_GAUSSIAN  2
#define CV_MEDIAN 3
#define CV_BILATERAL 4

/* Smoothes array (removes noise) */
CVAPI(void) cvSmooth( const CvArr* src, CvArr* dst,
                      int smoothtype CV_DEFAULT(CV_GAUSSIAN),
                      int param1 CV_DEFAULT(3),
                      int param2 CV_DEFAULT(0),
                      double param3 CV_DEFAULT(0),
                      double param4 CV_DEFAULT(0));



/* Finds integral image: SUM(X,Y) = sum(x<X,y<Y)I(x,y) */
CVAPI(void) cvIntegral( const CvArr* image, CvArr* sum,
                       CvArr* sqsum CV_DEFAULT(NULL),
                       CvArr* tilted_sum CV_DEFAULT(NULL));

/*
   Smoothes the input image with gaussian kernel and then down-samples it.
   dst_width = floor(src_width/2)[+1],
   dst_height = floor(src_height/2)[+1]
*/
CVAPI(void)  cvPyrDown( const CvArr* src, CvArr* dst,
                        int filter CV_DEFAULT(CV_GAUSSIAN_5x5) );

/* 
   Up-samples image and smoothes the result with gaussian kernel.
   dst_width = src_width*2,
   dst_height = src_height*2
*/
CVAPI(void)  cvPyrUp( const CvArr* src, CvArr* dst,
                      int filter CV_DEFAULT(CV_GAUSSIAN_5x5) );


/* Builds the whole pyramid at once. Output array of CvMat headers (levels[*])
   is initialized with the headers of subsequent pyramid levels */
/*CVAPI  void  cvCalcPyramid( const CvArr* src, CvArr* container,
                              CvMat* levels, int level_count,
                              int filter CV_DEFAULT(CV_GAUSSIAN_5x5) );*/


/* Splits color or grayscale image into multiple connected components
   of nearly the same color/brightness using modification of Burt algorithm.
   comp with contain a pointer to sequence (CvSeq)
   of connected components (CvConnectedComp) */
CVAPI(void) cvPyrSegmentation( IplImage* src, IplImage* dst,
                              CvMemStorage* storage, CvSeq** comp,
                              int level, double threshold1,
                              double threshold2 );

/* Filters image using meanshift algorithm */
CVAPI(void) cvPyrMeanShiftFiltering( const CvArr* src, CvArr* dst, 
    double sp, double sr, int max_level CV_DEFAULT(1),
    CvTermCriteria termcrit CV_DEFAULT(cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,5,1)));

/* Segments image using seed "markers" */
CVAPI(void) cvWatershed( const CvArr* image, CvArr* markers );

#define CV_INPAINT_NS      0
#define CV_INPAINT_TELEA   1

/* Inpaints the selected region in the image */
CVAPI(void) cvInpaint( const CvArr* src, const CvArr* inpaint_mask,
                       CvArr* dst, double inpaintRange, int flags );

#define CV_SCHARR -1
#define CV_MAX_SOBEL_KSIZE 7

/* Calculates an image derivative using generalized Sobel
   (aperture_size = 1,3,5,7) or Scharr (aperture_size = -1) operator.
   Scharr can be used only for the first dx or dy derivative */
CVAPI(void) cvSobel( const CvArr* src, CvArr* dst,
                    int xorder, int yorder,
                    int aperture_size CV_DEFAULT(3));

/* Calculates the image Laplacian: (d2/dx + d2/dy)I */
CVAPI(void) cvLaplace( const CvArr* src, CvArr* dst,
                      int aperture_size CV_DEFAULT(3) );

/* Constants for color conversion */
#define  CV_BGR2BGRA    0
#define  CV_RGB2RGBA    CV_BGR2BGRA

#define  CV_BGRA2BGR    1
#define  CV_RGBA2RGB    CV_BGRA2BGR

#define  CV_BGR2RGBA    2
#define  CV_RGB2BGRA    CV_BGR2RGBA

#define  CV_RGBA2BGR    3
#define  CV_BGRA2RGB    CV_RGBA2BGR

#define  CV_BGR2RGB     4
#define  CV_RGB2BGR     CV_BGR2RGB

#define  CV_BGRA2RGBA   5
#define  CV_RGBA2BGRA   CV_BGRA2RGBA

#define  CV_BGR2GRAY    6
#define  CV_RGB2GRAY    7
#define  CV_GRAY2BGR    8
#define  CV_GRAY2RGB    CV_GRAY2BGR
#define  CV_GRAY2BGRA   9
#define  CV_GRAY2RGBA   CV_GRAY2BGRA
#define  CV_BGRA2GRAY   10
#define  CV_RGBA2GRAY   11

#define  CV_BGR2BGR565  12
#define  CV_RGB2BGR565  13
#define  CV_BGR5652BGR  14
#define  CV_BGR5652RGB  15
#define  CV_BGRA2BGR565 16
#define  CV_RGBA2BGR565 17
#define  CV_BGR5652BGRA 18
#define  CV_BGR5652RGBA 19

#define  CV_GRAY2BGR565 20
#define  CV_BGR5652GRAY 21

#define  CV_BGR2BGR555  22
#define  CV_RGB2BGR555  23
#define  CV_BGR5552BGR  24
#define  CV_BGR5552RGB  25
#define  CV_BGRA2BGR555 26
#define  CV_RGBA2BGR555 27
#define  CV_BGR5552BGRA 28
#define  CV_BGR5552RGBA 29

#define  CV_GRAY2BGR555 30
#define  CV_BGR5552GRAY 31

#define  CV_BGR2XYZ     32
#define  CV_RGB2XYZ     33
#define  CV_XYZ2BGR     34
#define  CV_XYZ2RGB     35

#define  CV_BGR2YCrCb   36
#define  CV_RGB2YCrCb   37
#define  CV_YCrCb2BGR   38
#define  CV_YCrCb2RGB   39

#define  CV_BGR2HSV     40
#define  CV_RGB2HSV     41

#define  CV_BGR2Lab     44
#define  CV_RGB2Lab     45

#define  CV_BayerBG2BGR 46
#define  CV_BayerGB2BGR 47
#define  CV_BayerRG2BGR 48
#define  CV_BayerGR2BGR 49

#define  CV_BayerBG2RGB CV_BayerRG2BGR
#define  CV_BayerGB2RGB CV_BayerGR2BGR
#define  CV_BayerRG2RGB CV_BayerBG2BGR
#define  CV_BayerGR2RGB CV_BayerGB2BGR

#define  CV_BGR2Luv     50
#define  CV_RGB2Luv     51
#define  CV_BGR2HLS     52
#define  CV_RGB2HLS     53

#define  CV_HSV2BGR     54
#define  CV_HSV2RGB     55

#define  CV_Lab2BGR     56
#define  CV_Lab2RGB     57
#define  CV_Luv2BGR     58
#define  CV_Luv2RGB     59
#define  CV_HLS2BGR     60
#define  CV_HLS2RGB     61

#define  CV_COLORCVT_MAX  100

/* Converts input array pixels from one color space to another */
CVAPI(void)  cvCvtColor( const CvArr* src, CvArr* dst, int code );

#define  CV_INTER_NN        0
#define  CV_INTER_LINEAR    1
#define  CV_INTER_CUBIC     2
#define  CV_INTER_AREA      3

#define  CV_WARP_FILL_OUTLIERS 8
#define  CV_WARP_INVERSE_MAP  16


/* Computes affine transform matrix for mapping src[i] to dst[i] (i=0,1,2) */
CVAPI(CvMat*) cvGetAffineTransform( const CvPoint2D32f * src, 
                                    const CvPoint2D32f * dst, 
                                    CvMat * map_matrix );



/* Warps image with perspective (projective) transform */
CVAPI(void)  cvWarpPerspective( const CvArr* src, CvArr* dst, const CvMat* map_matrix,
                                int flags CV_DEFAULT(CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS),
                                CvScalar fillval CV_DEFAULT(cvScalarAll(0)) );

/* Computes perspective transform matrix for mapping src[i] to dst[i] (i=0,1,2,3) */
CVAPI(CvMat*) cvGetPerspectiveTransform( const CvPoint2D32f* src,
                                         const CvPoint2D32f* dst,
                                         CvMat* map_matrix );

/* Performs generic geometric transformation using the specified coordinate maps */
CVAPI(void)  cvRemap( const CvArr* src, CvArr* dst,
                      const CvArr* mapx, const CvArr* mapy,
                      int flags CV_DEFAULT(CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS),
                      CvScalar fillval CV_DEFAULT(cvScalarAll(0)) );


#define  CV_SHAPE_RECT      0
#define  CV_SHAPE_CROSS     1
#define  CV_SHAPE_ELLIPSE   2
#define  CV_SHAPE_CUSTOM    100

/* creates structuring element used for morphological operations */
CVAPI(IplConvKernel*)  cvCreateStructuringElementEx(
            int cols, int  rows, int  anchor_x, int  anchor_y,
            int shape, int* values CV_DEFAULT(NULL) );

/* releases structuring element */
CVAPI(void)  cvReleaseStructuringElement( IplConvKernel** element );

/* erodes input image (applies minimum filter) one or more times.
   If element pointer is NULL, 3x3 rectangular element is used */
CVAPI(void)  cvErode( const CvArr* src, CvArr* dst,
                      IplConvKernel* element CV_DEFAULT(NULL),
                      int iterations CV_DEFAULT(1) );

/* dilates input image (applies maximum filter) one or more times.
   If element pointer is NULL, 3x3 rectangular element is used */
CVAPI(void)  cvDilate( const CvArr* src, CvArr* dst,
                       IplConvKernel* element CV_DEFAULT(NULL),
                       int iterations CV_DEFAULT(1) );

#define CV_MOP_OPEN         2
#define CV_MOP_CLOSE        3
#define CV_MOP_GRADIENT     4
#define CV_MOP_TOPHAT       5
#define CV_MOP_BLACKHAT     6






/* Methods for comparing two array */
#define  CV_TM_SQDIFF        0
#define  CV_TM_SQDIFF_NORMED 1
#define  CV_TM_CCORR         2
#define  CV_TM_CCORR_NORMED  3
#define  CV_TM_CCOEFF        4
#define  CV_TM_CCOEFF_NORMED 5



/* Computes earth mover distance between
   two weighted point sets (called signatures) */
CVAPI(float)  cvCalcEMD2( const CvArr* signature1,
                          const CvArr* signature2,
                          int distance_type,
                          CvDistanceFunction distance_func CV_DEFAULT(NULL),
                          const CvArr* cost_matrix CV_DEFAULT(NULL),
                          CvArr* flow CV_DEFAULT(NULL),
                          float* lower_bound CV_DEFAULT(NULL),
                          void* userdata CV_DEFAULT(NULL));

/****************************************************************************************\
*                              Contours retrieving                                       *
\****************************************************************************************/

/* Retrieves outer and optionally inner boundaries of white (non-zero) connected
   components in the black (zero) background */
CVAPI(int)  cvFindContours( CvArr* image, CvMemStorage* storage, CvSeq** first_contour,
                            int header_size CV_DEFAULT(sizeof(CvContour)),
                            int mode CV_DEFAULT(CV_RETR_LIST),
                            int method CV_DEFAULT(CV_CHAIN_APPROX_SIMPLE),
                            CvPoint offset CV_DEFAULT(cvPoint(0,0)));


/* Initalizes contour retrieving process.
   Calls cvStartFindContours.
   Calls cvFindNextContour until null pointer is returned
   or some other condition becomes true.
   Calls cvEndFindContours at the end. */
CVAPI(CvContourScanner)  cvStartFindContours( CvArr* image, CvMemStorage* storage,
                            int header_size CV_DEFAULT(sizeof(CvContour)),
                            int mode CV_DEFAULT(CV_RETR_LIST),
                            int method CV_DEFAULT(CV_CHAIN_APPROX_SIMPLE),
                            CvPoint offset CV_DEFAULT(cvPoint(0,0)));

/* Retrieves next contour */
CVAPI(CvSeq*)  cvFindNextContour( CvContourScanner scanner );


/* Releases contour scanner and returns pointer to the first outer contour */
CVAPI(CvSeq*)  cvEndFindContours( CvContourScanner* scanner );


/* Initalizes Freeman chain reader.
   The reader is used to iteratively get coordinates of all the chain points.
   If the Freeman codes should be read as is, a simple sequence reader should be used */
CVAPI(void) cvStartReadChainPoints( CvChain* chain, CvChainPtReader* reader );



/****************************************************************************************\
*                                  Motion Analysis                                       *
\****************************************************************************************/



/************ Basic quad-edge navigation and operations ************/


/****************************************************************************************\
*                            Contour Processing and Shape Analysis                       *
\****************************************************************************************/

#define CV_POLY_APPROX_DP 0

/* Approximates a single polygonal curve (contour) or
   a tree of polygonal curves (contours) */
CVAPI(CvSeq*)  cvApproxPoly( const void* src_seq,
                             int header_size, CvMemStorage* storage,
                             int method, double parameter,
                             int parameter2 CV_DEFAULT(0));

#define CV_DOMINANT_IPAN 1


/* Calculates perimeter of a contour or length of a part of contour */
CVAPI(double)  cvArcLength( const void* curve,
                            CvSlice slice CV_DEFAULT(CV_WHOLE_SEQ),
                            int is_closed CV_DEFAULT(-1));
#define cvContourPerimeter( contour ) cvArcLength( contour, CV_WHOLE_SEQ, 1 )

/* Calculates contour boundning rectangle (update=1) or
   just retrieves pre-calculated rectangle (update=0) */
CVAPI(CvRect)  cvBoundingRect( CvArr* points, int update CV_DEFAULT(0) );

/* Calculates area of a contour or contour segment */
CVAPI(double)  cvContourArea( const CvArr* contour,
                              CvSlice slice CV_DEFAULT(CV_WHOLE_SEQ));

/* Finds minimum area rotated rectangle bounding a set of points */
CVAPI(CvBox2D)  cvMinAreaRect2( const CvArr* points,
                                CvMemStorage* storage CV_DEFAULT(NULL));



#define CV_CONTOURS_MATCH_I1  1
#define CV_CONTOURS_MATCH_I2  2
#define CV_CONTOURS_MATCH_I3  3



#define CV_CLOCKWISE         1
#define CV_COUNTER_CLOCKWISE 2

/* Calculates exact convex hull of 2d point set */
CVAPI(CvSeq*) cvConvexHull2( const CvArr* input,
                             void* hull_storage CV_DEFAULT(NULL),
                             int orientation CV_DEFAULT(CV_CLOCKWISE),
                             int return_points CV_DEFAULT(0));

/* Checks whether the contour is convex or not (returns 1 if convex, 0 if not) */
CVAPI(int)  cvCheckContourConvexity( const CvArr* contour );

/* Finds minimum rectangle containing two given rectangles */
CVAPI(CvRect)  cvMaxRect( const CvRect* rect1, const CvRect* rect2 );

/* Finds coordinates of the box vertices */
CVAPI(void) cvBoxPoints( CvBox2D box, CvPoint2D32f pt[4] );

/* Initializes sequence header for a matrix (column or row vector) of points -
   a wrapper for cvMakeSeqHeaderForArray (it does not initialize bounding rectangle!!!) */
CVAPI(CvSeq*) cvPointSeqFromMat( int seq_kind, const CvArr* mat,
                                 CvContour* contour_header,
                                 CvSeqBlock* block );




#define CV_DIST_MASK_3   3
#define CV_DIST_MASK_5   5
#define CV_DIST_MASK_PRECISE 0



/* Types of thresholding */
#define CV_THRESH_BINARY      0  /* value = value > threshold ? max_value : 0       */
#define CV_THRESH_BINARY_INV  1  /* value = value > threshold ? 0 : max_value       */
#define CV_THRESH_TRUNC       2  /* value = value > threshold ? threshold : value   */
#define CV_THRESH_TOZERO      3  /* value = value > threshold ? value : 0           */
#define CV_THRESH_TOZERO_INV  4  /* value = value > threshold ? 0 : value           */
#define CV_THRESH_MASK        7

#define CV_THRESH_OTSU        8  /* use Otsu algorithm to choose the optimal threshold value;
                                    combine the flag with one of the above CV_THRESH_* values */

/* Applies fixed-level threshold to grayscale image.
   This is a basic operation applied before retrieving contours */
CVAPI(void)  cvThreshold( const CvArr*  src, CvArr*  dst,
                          double  threshold, double  max_value,
                          int threshold_type );

#define CV_ADAPTIVE_THRESH_MEAN_C  0
#define CV_ADAPTIVE_THRESH_GAUSSIAN_C  1

/* Applies adaptive threshold to grayscale image.
   The two parameters for methods CV_ADAPTIVE_THRESH_MEAN_C and
   CV_ADAPTIVE_THRESH_GAUSSIAN_C are:
   neighborhood size (3, 5, 7 etc.),
   and a constant subtracted from mean (...,-3,-2,-1,0,1,2,3,...) */
CVAPI(void)  cvAdaptiveThreshold( const CvArr* src, CvArr* dst, double max_value,
                                  int adaptive_method CV_DEFAULT(CV_ADAPTIVE_THRESH_MEAN_C),
                                  int threshold_type CV_DEFAULT(CV_THRESH_BINARY),
                                  int block_size CV_DEFAULT(3),
                                  double param1 CV_DEFAULT(5));

#define CV_FLOODFILL_FIXED_RANGE (1 << 16)
#define CV_FLOODFILL_MASK_ONLY   (1 << 17)



/****************************************************************************************\
*                                  Feature detection                                     *
\****************************************************************************************/

#define CV_CANNY_L2_GRADIENT  (1 << 31)

/* Runs canny edge detector */
CVAPI(void)  cvCanny( const CvArr* image, CvArr* edges, double threshold1,
                      double threshold2, int  aperture_size CV_DEFAULT(3) );



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "cv.hpp"
#endif


#endif /*_CV_H_*/

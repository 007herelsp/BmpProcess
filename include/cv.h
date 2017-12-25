#ifndef _VOS_H_
#define _VOS_H_

#include "cxcore.h"
#include "cvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************\
*                                    Image Processing                                    *
\****************************************************************************************/

#define VOS_BLUR_NO_SCALE 0
#define VOS_BLUR  1
#define VOS_GAUSSIAN  2
#define VOS_MEDIAN 3
#define VOS_BILATERAL 4

/* Smoothes array (removes noise) */
CVAPI(void) cvSmooth( const CvArr* src, CvArr* dst,
                      int smoothtype VOS_DEFAULT(VOS_GAUSSIAN),
                      int param1 VOS_DEFAULT(3),
                      int param2 VOS_DEFAULT(0),
                      double param3 VOS_DEFAULT(0),
                      double param4 VOS_DEFAULT(0));
#define VOS_SCHARR -1
#define VOS_MAX_SOBEL_KSIZE 7

/* Calculates an image derivative using generalized Sobel
   (aperture_size = 1,3,5,7) or Scharr (aperture_size = -1) operator.
   Scharr can be used only for the first dx or dy derivative */
CVAPI(void) cvSobel( const CvArr* src, CvArr* dst,
                    int xorder, int yorder,
                    int aperture_size VOS_DEFAULT(3));


/* Constants for color conversion */
#define  VOS_BGR2GRAY    6
#define  VOS_RGB2GRAY    7

/* Converts input array pixels from one color space to another */
CVAPI(void)  cvCvtColor( const CvArr* src, CvArr* dst, int code );

#define  VOS_INTER_LINEAR    1

#define  VOS_WARP_FILL_OUTLIERS 8
#define  VOS_WARP_INVERSE_MAP  16


/* Warps image with perspective (projective) transform */
CVAPI(void)  cvWarpPerspective( const CvArr* src, CvArr* dst, const CvMat* map_matrix,
                                int flags VOS_DEFAULT(VOS_INTER_LINEAR+VOS_WARP_FILL_OUTLIERS),
                                CvScalar fillval VOS_DEFAULT(cvScalarAll(0)) );

/* Computes perspective transform matrix for mapping src[i] to dst[i] (i=0,1,2,3) */
CVAPI(CvMat*) cvGetPerspectiveTransform( const CvPoint2D32f* src,
                                         const CvPoint2D32f* dst,
                                         CvMat* map_matrix );




#define  VOS_SHAPE_RECT      0
#define  VOS_SHAPE_CROSS     1
#define  VOS_SHAPE_ELLIPSE   2
#define  VOS_SHAPE_CUSTOM    100

/* creates structuring element used for morphological operations */
CVAPI(IplConvKernel*)  cvCreateStructuringElementEx(
            int cols, int  rows, int  anchor_x, int  anchor_y,
            int shape, int* values VOS_DEFAULT(NULL) );

/* releases structuring element */
CVAPI(void)  cvReleaseStructuringElement( IplConvKernel** element );

/* erodes input image (applies minimum filter) one or more times.
   If element pointer is NULL, 3x3 rectangular element is used */
CVAPI(void)  cvErode( const CvArr* src, CvArr* dst,
                      IplConvKernel* element VOS_DEFAULT(NULL),
                      int iterations VOS_DEFAULT(1) );

/* dilates input image (applies maximum filter) one or more times.
   If element pointer is NULL, 3x3 rectangular element is used */
CVAPI(void)  cvDilate( const CvArr* src, CvArr* dst,
                       IplConvKernel* element VOS_DEFAULT(NULL),
                       int iterations VOS_DEFAULT(1) );


/****************************************************************************************\
*                              Contours retrieving                                       *
\****************************************************************************************/

CVAPI(int)  cvFindContours( CvArr* image, CvMemStorage* storage, CvSeq** first_contour,
                            int header_size VOS_DEFAULT(sizeof(CvContour)),
                            int mode VOS_DEFAULT(VOS_RETR_LIST),
                            int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                            CvPoint offset VOS_DEFAULT(cvPoint(0,0)));


CVAPI(CvContourScanner)  cvStartFindContours( CvArr* image, CvMemStorage* storage,
                            int header_size VOS_DEFAULT(sizeof(CvContour)),
                            int mode VOS_DEFAULT(VOS_RETR_LIST),
                            int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                            CvPoint offset VOS_DEFAULT(cvPoint(0,0)));

/* Retrieves next contour */
CVAPI(CvSeq*)  cvFindNextContour( CvContourScanner scanner );


/* Releases contour scanner and returns pointer to the first outer contour */
CVAPI(CvSeq*)  cvEndFindContours( CvContourScanner* scanner );


/****************************************************************************************\
*                            Contour Processing and Shape Analysis                       *
\****************************************************************************************/

#define VOS_POLY_APPROX_DP 0

/* Approximates a single polygonal curve (contour) or
   a tree of polygonal curves (contours) */
CVAPI(CvSeq*)  cvApproxPoly( const void* src_seq,
                             int header_size, CvMemStorage* storage,
                             int method, double parameter,
                             int parameter2 VOS_DEFAULT(0));


/* Calculates perimeter of a contour or length of a part of contour */
CVAPI(double)  cvArcLength( const void* curve,
                            CvSlice slice VOS_DEFAULT(VOS_WHOLE_SEQ),
                            int is_closed VOS_DEFAULT(-1));
#define cvContourPerimeter( contour ) cvArcLength( contour, VOS_WHOLE_SEQ, 1 )

/* Calculates contour boundning rectangle (update=1) or
   just retrieves pre-calculated rectangle (update=0) */
CVAPI(CvRect)  cvBoundingRect( CvArr* points, int update VOS_DEFAULT(0) );

/* Calculates area of a contour or contour segment */
CVAPI(double)  cvContourArea( const CvArr* contour,
                              CvSlice slice VOS_DEFAULT(VOS_WHOLE_SEQ));

/* Finds minimum area rotated rectangle bounding a set of points */
CVAPI(CvBox2D)  cvMinAreaRect2( const CvArr* points,
                                CvMemStorage* storage VOS_DEFAULT(NULL));



#define VOS_CONTOURS_MATCH_I1  1
#define VOS_CONTOURS_MATCH_I2  2
#define VOS_CONTOURS_MATCH_I3  3



#define VOS_CLOCKWISE         1
#define VOS_COUNTER_CLOCKWISE 2

/* Calculates exact convex hull of 2d point set */
CVAPI(CvSeq*) cvConvexHull2( const CvArr* input,
                             void* hull_storage VOS_DEFAULT(NULL),
                             int orientation VOS_DEFAULT(VOS_CLOCKWISE),
                             int return_points VOS_DEFAULT(0));

/* Checks whether the contour is convex or not (returns 1 if convex, 0 if not) */
CVAPI(int)  cvCheckContourConvexity( const CvArr* contour );

/* Finds minimum rectangle containing two given rectangles */

/* Types of thresholding */
#define VOS_THRESH_BINARY      0  /* value = value > threshold ? max_value : 0       */
#define VOS_THRESH_BINARY_INV  1  /* value = value > threshold ? 0 : max_value       */
#define VOS_THRESH_TRUNC       2  /* value = value > threshold ? threshold : value   */
#define VOS_THRESH_TOZERO      3  /* value = value > threshold ? value : 0           */
#define VOS_THRESH_TOZERO_INV  4  /* value = value > threshold ? 0 : value           */
#define VOS_THRESH_MASK        7

#define VOS_THRESH_OTSU        8  /* use Otsu algorithm to choose the optimal threshold value;
                                    combine the flag with one of the above VOS_THRESH_* values */

/* Applies fixed-level threshold to grayscale image.
   This is a basic operation applied before retrieving contours */
CVAPI(void)  cvThreshold( const CvArr*  src, CvArr*  dst,
                          double  threshold, double  max_value,
                          int threshold_type );


/****************************************************************************************\
*                                  Feature detection                                     *
\****************************************************************************************/

#define VOS_CANNY_L2_GRADIENT  (1 << 31)

/* Runs canny edge detector */
CVAPI(void)  cvCanny( const CvArr* image, CvArr* edges, double threshold1,
                      double threshold2, int  aperture_size VOS_DEFAULT(3) );



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "cv.hpp"
#endif


#endif /*_VOS_H_*/

#ifndef _VOS_H_
#define _VOS_H_

#include "cxcore.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ContourScanner* ContourScanner;

/* contour retrieval mode */
#define VOS_RETR_EXTERNAL 0
#define VOS_RETR_LIST     1
#define VOS_RETR_CCOMP    2
#define VOS_RETR_TREE     3

/* contour approximation method */
#define VOS_CHAIN_APPROX_NONE        1
#define VOS_CHAIN_APPROX_SIMPLE      2
#define VOS_LINK_RUNS                3

#define VOS_BLUR_NO_SCALE 0
#define VOS_BLUR  1
#define VOS_GAUSSIAN  2
#define VOS_MEDIAN 3
#define VOS_BILATERAL 4

void Smooth( const VOID* src, VOID* dst,
               int smoothtype VOS_DEFAULT(VOS_GAUSSIAN),
               int param1 VOS_DEFAULT(3),
               int param2 VOS_DEFAULT(0),
               double param3 VOS_DEFAULT(0),
               double param4 VOS_DEFAULT(0));
#define VOS_SCHARR -1
#define VOS_MAX_SOBEL_KSIZE 7

/* Constants for color conversion */
#define  VOS_BGR2GRAY    6
#define  VOS_RGB2GRAY    7

/* Converts input array pixels from one color space to another */
void  CvtColor( const VOID* src, VOID* dst, int code );

#define  VOS_INTER_LINEAR    1

#define  VOS_WARP_FILL_OUTLIERS 8
#define  VOS_WARP_INVERSE_MAP  16


/* Warps image with perspective (projective) transform */
void  WarpPerspective( const VOID* src, VOID* dst, const Mat* map_matrix,
                         int flags VOS_DEFAULT(VOS_INTER_LINEAR+VOS_WARP_FILL_OUTLIERS),
                         Scalar fillval VOS_DEFAULT(ScalarAll(0)) );

/* Computes perspective transform matrix for mapping src[i] to dst[i] (i=0,1,2,3) */
Mat* GetPerspectiveTransform( const Point2D32f* src,
                                const Point2D32f* dst,
                                Mat* map_matrix );

#define  VOS_SHAPE_RECT      0
#define  VOS_SHAPE_CROSS     1
#define  VOS_SHAPE_ELLIPSE   2
#define  VOS_SHAPE_CUSTOM    100

/* creates structuring element used for morphological operations */
IplConvKernel*  CreateStructuringElementEx(
        int cols, int  rows, int  anchor_x, int  anchor_y,
        int shape, int* values VOS_DEFAULT(NULL) );

/* releases structuring element */
void  ReleaseStructuringElement( IplConvKernel** element );

void  Erode( const VOID* src, VOID* dst,
               IplConvKernel* element VOS_DEFAULT(NULL),
               int iterations VOS_DEFAULT(1) );

void  Dilate( const VOID* src, VOID* dst,
                IplConvKernel* element VOS_DEFAULT(NULL),
                int iterations VOS_DEFAULT(1) );

int FindContours( VOID* image, MemStorage* storage, Seq** first_contour,
                    int header_size VOS_DEFAULT(sizeof(CvContour)),
                    int mode VOS_DEFAULT(VOS_RETR_LIST),
                    int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                    Point offset VOS_DEFAULT(InitPoint(0,0)));

ContourScanner  StartFindContours( VOID* image, MemStorage* storage,
                                       int header_size VOS_DEFAULT(sizeof(CvContour)),
                                       int mode VOS_DEFAULT(VOS_RETR_LIST),
                                       int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                                       Point offset VOS_DEFAULT(InitPoint(0,0)));

/* Retrieves next contour */
Seq*  FindNextContour( ContourScanner scanner );


/* Releases contour scanner and returns pointer to the first outer contour */
Seq*  EndFindContours( ContourScanner* scanner );


/****************************************************************************************\
*                            Contour Processing and Shape Analysis                       *
\****************************************************************************************/

#define VOS_POLY_APPROX_DP 0

Seq*  ApproxPoly( const void* src_seq,
                      int header_size, MemStorage* storage,
                      int method, double parameter,
                      int parameter2 VOS_DEFAULT(0));


/* Calculates perimeter of a contour or length of a part of contour */
double  ArcLength( const void* curve,
                     Slice slice VOS_DEFAULT(VOS_WHOLE_SEQ),
                     int is_closed VOS_DEFAULT(-1));
#define cvContourPerimeter( contour ) ArcLength( contour, VOS_WHOLE_SEQ, 1 )

Rect  BoundingRect( VOID* points, int update VOS_DEFAULT(0) );

/* Calculates area of a contour or contour segment */
double  ContourArea(  const Seq *contour,
                       Slice slice VOS_DEFAULT(VOS_WHOLE_SEQ));

/* Finds minimum area rotated rectangle bounding a set of points */
Box2D  MinAreaRect2( const Seq* points,
                       MemStorage* storage VOS_DEFAULT(NULL));
#define VOS_CLOCKWISE         1
#define VOS_COUNTER_CLOCKWISE 2

/* Calculates exact convex hull of 2d point set */
Seq* ConvexHull2( const VOID* input,
                      void* hull_storage VOS_DEFAULT(NULL),
                      int orientation VOS_DEFAULT(VOS_CLOCKWISE),
                      int return_points VOS_DEFAULT(0));

/* Checks whether the contour is convex or not (returns 1 if convex, 0 if not) */
int  CheckContourConvexity( const VOID* contour );

/* Types of thresholding */
#define VOS_THRESH_BINARY      0  /* value = value > threshold ? max_value : 0       */


void  Threshold( const VOID*  src, VOID*  dst,
                   double  threshold, double  max_value,
                   int threshold_type );


/****************************************************************************************\
*                                  Feature detection                                     *
\****************************************************************************************/

#define VOS_CANNY_L2_GRADIENT  (1 << 31)

/* Runs canny edge detector */
void  Canny( const VOID* image, VOID* edges, double threshold1,
             double threshold2, int  aperture_size VOS_DEFAULT(3) );



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "cv.hpp"
#endif


#endif /*_VOS_H_*/

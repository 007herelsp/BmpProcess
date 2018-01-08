#ifndef _VOS_H_
#define _VOS_H_

#include "core.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct stContourScanner* ContourScanner;

/* contour retrieval mode */
#define VOS_RETR_LIST     1

/* contour approximation method */
#define VOS_CHAIN_APPROX_NONE        1
#define VOS_CHAIN_APPROX_SIMPLE      2
#define VOS_LINK_RUNS                3

#define VOS_GAUSSIAN  2

void Smooth( const VOID* src, VOID* dst,
               int param1 VOS_DEFAULT(3),
               int param2 VOS_DEFAULT(0),
               double param3 VOS_DEFAULT(0),
               double param4 VOS_DEFAULT(0));
#define VOS_SCHARR -1
#define VOS_MAX_SOBEL_KSIZE 7

void  CvtColor( const VOID* src, VOID* dstr, VOID* dstg,VOID* dstb);

#define  VOS_INTER_LINEAR    1

#define  VOS_WARP_FILL_OUTLIERS 8
#define  VOS_WARP_INVERSE_MAP  16

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
                    int header_size VOS_DEFAULT(sizeof(Contour)),
                    int mode VOS_DEFAULT(VOS_RETR_LIST),
                    int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                    Point offset VOS_DEFAULT(InitPoint(0,0)));

ContourScanner  StartFindContours( VOID* image, MemStorage* storage,
                                       int header_size VOS_DEFAULT(sizeof(Contour)),
                                       int mode VOS_DEFAULT(VOS_RETR_LIST),
                                       int method VOS_DEFAULT(VOS_CHAIN_APPROX_SIMPLE),
                                       Point offset VOS_DEFAULT(InitPoint(0,0)));

/* Retrieves next contour */
Seq*  FindNextContour( ContourScanner scanner );


/* Releases contour scanner and returns pointer to the first outer contour */
Seq*  EndFindContours( ContourScanner* scanner );

#define VOS_POLY_APPROX_DP 0

Seq*  ApproxPoly( const void* src_seq,
                      int header_size, MemStorage* storage,
                       double parameter,
                      int parameter2 VOS_DEFAULT(0));

double  ArcLength( const void* curve,
                     Slice slice VOS_DEFAULT(VOS_WHOLE_SEQ),
                     int is_closed VOS_DEFAULT(-1));
#define ContourPerimeter( contour ) ArcLength( contour, VOS_WHOLE_SEQ, 1 )

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
                      int orientation VOS_DEFAULT(VOS_CLOCKWISE));

/* Checks whether the contour is convex or not (returns 1 if convex, 0 if not) */
int  CheckContourConvexity( const VOID* contour );

/* Types of thresholding */
#define VOS_THRESH_BINARY      0  /* value = value > threshold ? max_value : 0       */

#define VOS_CANNY_L2_GRADIENT  (1 << 31)

/* Runs canny edge detector */
void  Canny( const VOID* image, VOID* edges, double threshold1,
             double threshold2, int  aperture_size VOS_DEFAULT(3) );

extern const uchar iSaturate8u[];
#define VOS_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), iSaturate8u[(t)+256])
#define VOS_CALC_MIN_8U(a,b) (a) -= VOS_FAST_CAST_8U((a) - (b))
#define VOS_CALC_MAX_8U(a,b) (a) += VOS_FAST_CAST_8U((b) - (a))

extern const float i8x32fTab[];
#define VOS_8TO32F(x)  i8x32fTab[(x)+256]
#define  VOS_CALC_MIN(a, b) if((a) > (b)) (a) = (b)
#define  VOS_CALC_MAX(a, b) if((a) < (b)) (a) = (b)


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#define VOS_WHOLE   0
#define VOS_START   1
#define VOS_END     2
#define VOS_MIDDLE  4

#define VOS_ISOLATED_ROI 8

typedef void (*RowFilterFunc)( const uchar* src, uchar* dst, void* params );
typedef void (*ColumnFilterFunc)( uchar** src, uchar* dst, int dst_step, int count, void* params );

class  BaseImageFilter
{
public:
    BaseImageFilter();
   
    virtual ~BaseImageFilter();

    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=InitPoint(-1,-1),
                       int _border_mode=SYS_BORDER_REPLICATE,
                       Scalar _border_value=ScalarAll(0) );

    virtual void clear();

    virtual int process( const Mat* _src, Mat* _dst,
                         Rect _src_roi=InitRect(0,0,-1,-1),
                         Point _dst_origin=InitPoint(0,0), int _flags=0 );
    int get_src_type() const { return src_type; }
    int get_dst_type() const { return dst_type; }
    Size get_kernel_size() const { return ksize; }
    int get_width() const { return prev_x_range.end_index - prev_x_range.start_index; }
    RowFilterFunc get_x_filter_func() const { return x_func; }
    ColumnFilterFunc get_y_filter_func() const { return y_func; }

protected:
    virtual void get_work_params();
    virtual void start_process( Slice x_range, int width );
    virtual void make_y_border( int row_count, int top_rows, int bottom_rows );
    virtual int fill_cyclic_buffer( const uchar* src, int src_step,
                                    int y, int y1, int y2 );

    enum { ALIGN=32 };

    int max_width;
    int min_depth, src_type, dst_type, work_type;
    RowFilterFunc x_func;
    ColumnFilterFunc y_func;

    uchar* buffer;
    uchar** rows;
    int top_rows, bottom_rows, max_rows;
    uchar *buf_start, *buf_end, *buf_head, *buf_tail;
    int buf_size, buf_step, buf_count, buf_max_count;

    bool is_separable;
    Size ksize;
    Point anchor;
    int max_ky, border_mode;
    Scalar border_value;
    uchar* const_row;
    int* border_tab;
    int border_tab_sz1, border_tab_sz;

    Slice prev_x_range;
    int prev_width;
};

class  SepFilter : public BaseImageFilter
{
public:
    SepFilter();

    virtual ~SepFilter();

    virtual void init( int _max_width, int _src_type, int _dst_type,
                       const Mat* _kx, const Mat* _ky,
                       Point _anchor=InitPoint(-1,-1),
                       int _border_mode=SYS_BORDER_REPLICATE,
                       Scalar _border_value=ScalarAll(0) );
    virtual void init_deriv( int _max_width, int _src_type, int _dst_type,
                             int dx, int dy, int aperture_size, int flags=0 );
    virtual void init_gaussian( int _max_width, int _src_type, int _dst_type,
                                int gaussian_size, double sigma );

    /* dummy method to avoid compiler warnings */
    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=InitPoint(-1,-1),
                       int _border_mode=SYS_BORDER_REPLICATE,
                       Scalar _border_value=ScalarAll(0) );

    virtual void clear();
    const Mat* get_x_kernel() const { return kx; }
    const Mat* get_y_kernel() const { return ky; }
    int get_x_kernel_flags() const { return kx_flags; }
    int get_y_kernel_flags() const { return ky_flags; }

    enum { GENERIC=0, ASYMMETRICAL=1, SYMMETRICAL=2, POSITIVE=4, SUM_TO_1=8, INTEGER=16 };
    enum { NORMALIZE_KERNEL=1, FLIP_KERNEL=2 };

    static void init_gaussian_kernel( Mat* kernel, double sigma=-1 );
    static void init_sobel_kernel( Mat* _kx, Mat* _ky, int dx, int dy, int flags=0 );

protected:
    Mat *kx, *ky;
    int kx_flags, ky_flags;
};

class  Morphology : public BaseImageFilter
{
public:
    Morphology();
    Morphology( int _operation, int _max_width, int _src_dst_type,
                  int _element_shape, Mat* _element,
                  Size _ksize=GetSize(0,0), Point _anchor=InitPoint(-1,-1),
                  int _border_mode=SYS_BORDER_REPLICATE,
                  Scalar _border_value=ScalarAll(0) );
    virtual ~Morphology();
    virtual void init( int _operation, int _max_width, int _src_dst_type,
                       int _element_shape, Mat* _element,
                       Size _ksize=GetSize(0,0), Point _anchor=InitPoint(-1,-1),
                       int _border_mode=SYS_BORDER_REPLICATE,
                       Scalar _border_value=ScalarAll(0) );

    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=InitPoint(-1,-1),
                       int _border_mode=SYS_BORDER_REPLICATE,
                       Scalar _border_value=ScalarAll(0) );

    virtual void clear();
    const Mat* get_element() const { return element; }
    int get_element_shape() const { return el_shape; }
    int get_operation() const { return operation; }
    uchar* get_element_sparse_buf() { return el_sparse; }
    int get_element_sparse_count() const { return el_sparse_count; }

    enum { RECT=0, CROSS=1, ELLIPSE=2, CUSTOM=100, BINARY = 0, GRAYSCALE=256 };
    enum { ERODE=0, DILATE=1 };

    static void init_binary_element( Mat* _element, int _element_shape,
                                     Point _anchor=InitPoint(-1,-1) );
protected:

    void start_process( Slice x_range, int width );
    int fill_cyclic_buffer( const uchar* src, int src_step,
                            int y0, int y1, int y2 );
    uchar* el_sparse;
    int el_sparse_count;

    Mat *element;
    int el_shape;
    int operation;
};


#endif /* __cplusplus */
#endif /*_VOS_H_*/

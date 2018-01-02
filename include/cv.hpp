
#ifndef _VOS_HPP_
#define _VOS_HPP_

#ifdef __cplusplus

/****************************************************************************************\
*                    BaseImageFilter: Base class for filtering operations              *
\****************************************************************************************/

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
    /* calls init() */
    
    virtual ~BaseImageFilter();

    /* initializes the class for processing an image of maximal width _max_width,
       input image has data type _src_type, the output will have _dst_type.
       _is_separable != 0 if the filter is separable
       (specific behaviour is defined in a derived class), 0 otherwise.
       _ksize and _anchor specify the kernel size and the anchor point. _anchor=(-1,-1) means
       that the anchor is at the center.
       to get interpolate pixel values outside the image _border_mode=IPL_BORDER_*** is used,
       _border_value specify the pixel value in case of IPL_BORDER_CONSTANT border mode.
       before initialization clear() is called if necessary.
    */
    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=cvPoint(-1,-1),
                       int _border_mode=IPL_BORDER_REPLICATE,
                       Scalar _border_value=cvScalarAll(0) );
    /* releases all the internal buffers.
       for the further use of the object, init() needs to be called. */
    virtual void clear();

    virtual int process( const Mat* _src, Mat* _dst,
                         Rect _src_roi=cvRect(0,0,-1,-1),
                         Point _dst_origin=cvPoint(0,0), int _flags=0 );
    /* retrieve various parameters of the filtering object */
    int get_src_type() const { return src_type; }
    int get_dst_type() const { return dst_type; }
    Size get_kernel_size() const { return ksize; }
    int get_width() const { return prev_x_range.end_index - prev_x_range.start_index; }
    RowFilterFunc get_x_filter_func() const { return x_func; }
    ColumnFilterFunc get_y_filter_func() const { return y_func; }

protected:
    /* initializes work_type, buf_size and max_rows */
    virtual void get_work_params();
    /* it is called (not always) from process when _phase=VOS_START or VOS_WHOLE.
       the method initializes ring buffer (buf_end, buf_head, buf_tail, buf_count, rows),
       prev_width, prev_x_range, const_row, border_tab, border_tab_sz* */
    virtual void start_process( Slice x_range, int width );
    /* forms pointers to "virtual rows" above or below the processed roi using the specified
       border mode */
    virtual void make_y_border( int row_count, int top_rows, int bottom_rows );

    virtual int fill_cyclic_buffer( const uchar* src, int src_step,
                                    int y, int y1, int y2 );

    enum { ALIGN=32 };
    
    int max_width;
    /* currently, work_type must be the same as src_type in case of non-separable filters */
    int min_depth, src_type, dst_type, work_type;

    /* pointers to convolution functions, initialized by init method.
       for non-separable filters only y_conv should be set */
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


/* Derived class, for linear separable filtering. */
class  SepFilter : public BaseImageFilter
{
public:
    SepFilter();

    virtual ~SepFilter();

    virtual void init( int _max_width, int _src_type, int _dst_type,
                       const Mat* _kx, const Mat* _ky,
                       Point _anchor=cvPoint(-1,-1),
                       int _border_mode=IPL_BORDER_REPLICATE,
                       Scalar _border_value=cvScalarAll(0) );
    virtual void init_deriv( int _max_width, int _src_type, int _dst_type,
                             int dx, int dy, int aperture_size, int flags=0 );
    virtual void init_gaussian( int _max_width, int _src_type, int _dst_type,
                                int gaussian_size, double sigma );

    /* dummy method to avoid compiler warnings */
    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=cvPoint(-1,-1),
                       int _border_mode=IPL_BORDER_REPLICATE,
                       Scalar _border_value=cvScalarAll(0) );

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




/* basic morphological operations: erosion & dilation */
class  Morphology : public BaseImageFilter
{
public:
    Morphology();
    Morphology( int _operation, int _max_width, int _src_dst_type,
                  int _element_shape, Mat* _element,
                  Size _ksize=cvSize(0,0), Point _anchor=cvPoint(-1,-1),
                  int _border_mode=IPL_BORDER_REPLICATE,
                  Scalar _border_value=cvScalarAll(0) );
    virtual ~Morphology();
    virtual void init( int _operation, int _max_width, int _src_dst_type,
                       int _element_shape, Mat* _element,
                       Size _ksize=cvSize(0,0), Point _anchor=cvPoint(-1,-1),
                       int _border_mode=IPL_BORDER_REPLICATE,
                       Scalar _border_value=cvScalarAll(0) );

    /* dummy method to avoid compiler warnings */
    virtual void init( int _max_width, int _src_type, int _dst_type,
                       bool _is_separable, Size _ksize,
                       Point _anchor=cvPoint(-1,-1),
                       int _border_mode=IPL_BORDER_REPLICATE,
                       Scalar _border_value=cvScalarAll(0) );

    virtual void clear();
    const Mat* get_element() const { return element; }
    int get_element_shape() const { return el_shape; }
    int get_operation() const { return operation; }
    uchar* get_element_sparse_buf() { return el_sparse; }
    int get_element_sparse_count() const { return el_sparse_count; }

    enum { RECT=0, CROSS=1, ELLIPSE=2, CUSTOM=100, BINARY = 0, GRAYSCALE=256 };
    enum { ERODE=0, DILATE=1 };

    static void init_binary_element( Mat* _element, int _element_shape,
                                     Point _anchor=cvPoint(-1,-1) );
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

#endif /* _VOS_HPP_ */

/* End of file. */


#include "cv.h"
#include "misc.h"
#include <limits.h>
#include <stdio.h>

static void icvErodeRectRow_8u(const uchar *src, uchar *dst, void *params);
static void icvDilateRectRow_8u(const uchar *src, uchar *dst, void *params);

static void icvErodeRectCol_8u(const uchar **src, uchar *dst, int dst_step,
                               int count, void *params);
static void icvDilateRectCol_8u(const uchar **src, uchar *dst, int dst_step,
                                int count, void *params);

static void icvErodeAny_8u(const uchar **src, uchar *dst, int dst_step,
                           int count, void *params);
static void icvDilateAny_8u(const uchar **src, uchar *dst, int dst_step,
                            int count, void *params);

Morphology::Morphology()
{
    element = 0;
    el_sparse = 0;
}

Morphology::Morphology(int _operation, int _max_width, int _src_dst_type,
                       int _element_shape, Mat *_element,
                       Size _ksize, Point _anchor,
                       int _border_mode, Scalar _border_value)
{
    element = 0;
    el_sparse = 0;
    init(_operation, _max_width, _src_dst_type,
         _element_shape, _element, _ksize, _anchor,
         _border_mode, _border_value);
}

void Morphology::clear()
{
    ReleaseMat(&element);
    SYS_FREE(&el_sparse);
    BaseImageFilter::clear();
}

Morphology::~Morphology()
{
    clear();
}

void Morphology::init(int _operation, int _max_width, int _src_dst_type,
                      int _element_shape, Mat *_element,
                      Size _ksize, Point _anchor,
                      int _border_mode, Scalar _border_value)
{
    VOS_FUNCNAME("Morphology::init");

    __BEGIN__;

    int depth = VOS_MAT_DEPTH(_src_dst_type);

    if (_operation != ERODE && _operation != DILATE)
        VOS_ERROR(VOS_StsBadArg, "Unknown/unsupported morphological operation");



    operation = _operation;
    el_shape = _element_shape;

    VOS_CALL(BaseImageFilter::init(_max_width, _src_dst_type, _src_dst_type,
                                   _element_shape == RECT, _ksize, _anchor, _border_mode, _border_value));

    if (el_shape == RECT)
    {
        if (operation == ERODE)
        {
            if (depth == VOS_8U)
            {
                x_func = (RowFilterFunc)icvErodeRectRow_8u;
                y_func = (ColumnFilterFunc)icvErodeRectCol_8u;
            }
            else
            {
                VOS_ERROR(VOS_StsBadArg,
                          "herelsp remove");
            }
        }
        else
        {
            assert(operation == DILATE);
            if (depth == VOS_8U)
            {
                x_func = (RowFilterFunc)icvDilateRectRow_8u;
                y_func = (ColumnFilterFunc)icvDilateRectCol_8u;
            }
            else
            {
                VOS_ERROR(VOS_StsBadArg,
                          "herelsp remove");
            }
        }
    }
    else
    {
        int i, j, k = 0;
        int cn = VOS_MAT_CN(src_type);
        Point *nz_loc;

        if (!(element && el_sparse &&
              _ksize.width == element->cols && _ksize.height == element->rows))
        {
            ReleaseMat(&element);
            SYS_FREE(&el_sparse);
            VOS_CALL(element = CreateMat(_ksize.height, _ksize.width, VOS_8UC1));
            VOS_CALL(el_sparse = (uchar *)SysAlloc(
                        ksize.width * ksize.height * (2 * sizeof(int) + sizeof(uchar *))));
        }

        if (el_shape == CUSTOM)
        {
            VOS_CALL(Convert(_element, element));
        }
        else
        {
            VOS_CALL(init_binary_element(element, el_shape, anchor));
        }

        if (operation == ERODE)
        {
            if (depth == VOS_8U)
                y_func = (ColumnFilterFunc)icvErodeAny_8u;
            else
            {
                VOS_ERROR(VOS_StsBadArg,
                          "herelsp remove");
            }
        }
        else
        {
            assert(operation == DILATE);
            if (VOS_8U == depth )
                y_func = (ColumnFilterFunc)icvDilateAny_8u;
            else
            {
                VOS_ERROR(VOS_StsBadArg, "");
            }
        }

        nz_loc = (Point *)el_sparse;

        for (i = 0; i < ksize.height; i++)
            for (j = 0; j < ksize.width; j++)
            {
                if (element->data.ptr[i * element->step + j])
                    nz_loc[k++] = InitPoint(j * cn, i);
            }
        if (k == 0)
            nz_loc[k++] = InitPoint(anchor.x * cn, anchor.y);
        el_sparse_count = k;
    }
    __END__;
}

void Morphology::init(int _max_width, int _src_type, int _dst_type,
                      bool _is_separable, Size _ksize,
                      Point _anchor, int _border_mode,
                      Scalar _border_value)
{
    BaseImageFilter::init(_max_width, _src_type, _dst_type, _is_separable,
                          _ksize, _anchor, _border_mode, _border_value);
}

void Morphology::start_process(Slice x_range, int width)
{
    BaseImageFilter::start_process(x_range, width);
    if (el_shape == RECT)
    {
        int t = buf_max_count - max_ky * 2;
        if (t > 1 && t % 2 != 0)
        {
            buf_max_count--;
            buf_end -= buf_step;
        }
    }
}

int Morphology::fill_cyclic_buffer(const uchar *src, int src_step,
                                   int y0, int y1, int y2)
{
    return BaseImageFilter::fill_cyclic_buffer(src, src_step, y0, y1, y2);
}

void Morphology::init_binary_element(Mat *element, int element_shape, Point anchor)
{
    VOS_FUNCNAME("Morphology::init_binary_element");

    __BEGIN__;

    int type;
    int i, j, cols, rows;
    int r = 0, c = 0;
    double inv_r2 = 0;

    if (!VOS_IS_MAT(element))
        VOS_ERROR(VOS_StsBadArg, "element must be valid matrix");

    type = VOS_MAT_TYPE(element->type);
    if (type != VOS_8UC1 && type != VOS_32SC1)
        VOS_ERROR(VOS_StsUnsupportedFormat, "element must have 8uC1 or 32sC1 type");

    if (anchor.x == -1)
        anchor.x = element->cols / 2;

    if (anchor.y == -1)
        anchor.y = element->rows / 2;

    if ((unsigned)anchor.x >= (unsigned)element->cols ||
            (unsigned)anchor.y >= (unsigned)element->rows)
        VOS_ERROR(VOS_StsOutOfRange, "anchor is outside of element");

    if (element_shape != RECT && element_shape != CROSS && element_shape != ELLIPSE)
        VOS_ERROR(VOS_StsBadArg, "Unknown/unsupported element shape");

    rows = element->rows;
    cols = element->cols;

    if (rows == 1 || cols == 1)
        element_shape = RECT;

    if (element_shape == ELLIPSE)
    {
        r = rows / 2;
        c = cols / 2;
        inv_r2 = r ? 1. / ((double)r * r) : 0;
    }

    for (i = 0; i < rows; i++)
    {
        uchar *ptr = element->data.ptr + i * element->step;
        int j1 = 0, j2 = 0, jx, t = 0;

        if (element_shape == RECT || (element_shape == CROSS && i == anchor.y))
            j2 = cols;
        else if (element_shape == CROSS)
            j1 = anchor.x, j2 = j1 + 1;
        else
        {
            int dy = i - r;
            if (abs(dy) <= r)
            {
                int dx = SysRound(c * sqrt(((double)r * r - dy * dy) * inv_r2));
                j1 = VOS_MAX(c - dx, 0);
                j2 = VOS_MIN(c + dx + 1, cols);
            }
        }

        for (j = 0, jx = j1; j < cols;)
        {
            for (; j < jx; j++)
            {
                if (type == VOS_8UC1)
                    ptr[j] = (uchar)t;
                else
                    ((int *)ptr)[j] = t;
            }
            if (jx == j2)
                jx = cols, t = 0;
            else
                jx = j2, t = 1;
        }
    }

    __END__;
}

#define IVOS_MORPH_RECT_ROW(name, flavor, arrtype,                 \
    worktype, update_extr_macro)           \
    \
    static void \
    icv##name##RectRow_##flavor(const arrtype *src,                   \
    arrtype *dst, void *params)           \
    \
{                                                            \
    const Morphology *state = (const Morphology *)params; \
    int ksize = state->get_kernel_size().width;               \
    int width = state->get_width();                           \
    int cn = VOS_MAT_CN(state->get_src_type());                \
    int i, j, k;                                              \
    \
    width *= cn;                                              \
    ksize *= cn;                                              \
    \
    if (ksize == cn)                                          \
{                                                         \
    for (i = 0; i < width; i++)                           \
    dst[i] = src[i];                                  \
    return;                                               \
    }                                                         \
    \
    for (k = 0; k < cn; k++, src++, dst++)                    \
{                                                         \
    for (i = 0; i <= width - cn * 2; i += cn * 2)         \
{                                                     \
    const arrtype *s = src + i;                       \
    worktype m = s[cn], t;                            \
    for (j = cn * 2; j < ksize; j += cn)              \
{                                                 \
    t = s[j];                                     \
    update_extr_macro(m, t);                      \
    }                                                 \
    t = s[0];                                         \
    update_extr_macro(t, m);                          \
    dst[i] = (arrtype)t;                              \
    t = s[j];                                         \
    update_extr_macro(t, m);                          \
    dst[i + cn] = (arrtype)t;                         \
    }                                                     \
    \
    for (; i < width; i += cn)                            \
{                                                     \
    const arrtype *s = src + i;                       \
    worktype m = s[0], t;                             \
    for (j = cn; j < ksize; j += cn)                  \
{                                                 \
    t = s[j];                                     \
    update_extr_macro(m, t);                      \
    }                                                 \
    dst[i] = (arrtype)m;                              \
    }                                                     \
    }                                                         \
    \
    }

IVOS_MORPH_RECT_ROW(Erode, 8u, uchar, int, VOS_CALC_MIN_8U)
IVOS_MORPH_RECT_ROW(Dilate, 8u, uchar, int, VOS_CALC_MAX_8U)


#define IVOS_MORPH_RECT_COL(name, flavor, arrtype,                                \
    worktype, update_extr_macro, toggle_macro)            \
    \
    static void \
    icv##name##RectCol_##flavor(const arrtype **src,                                 \
    arrtype *dst, int dst_step, int count, void *params) \
    \
{                                                                           \
    const Morphology *state = (const Morphology *)params;                \
    int ksize = state->get_kernel_size().height;                             \
    int width = state->get_width();                                          \
    int cn = VOS_MAT_CN(state->get_src_type());                               \
    int i, k;                                                                \
    \
    width *= cn;                                                             \
    dst_step /= sizeof(dst[0]);                                              \
    \
    for (; ksize > 1 && count > 1; count -= 2,                               \
    dst += dst_step * 2, src += 2)            \
{                                                                        \
    for (i = 0; i <= width - 4; i += 4)                                  \
{                                                                    \
    const arrtype *sptr = src[1] + i;                                \
    worktype s0 = sptr[0], s1 = sptr[1],                             \
    s2 = sptr[2], s3 = sptr[3], t0, t1;                     \
    \
    for (k = 2; k < ksize; k++)                                      \
{                                                                \
    sptr = src[k] + i;                                           \
    t0 = sptr[0];                                                \
    t1 = sptr[1];                                                \
    update_extr_macro(s0, t0);                                   \
    update_extr_macro(s1, t1);                                   \
    t0 = sptr[2];                                                \
    t1 = sptr[3];                                                \
    update_extr_macro(s2, t0);                                   \
    update_extr_macro(s3, t1);                                   \
    }                                                                \
    \
    sptr = src[0] + i;                                               \
    t0 = sptr[0];                                                    \
    t1 = sptr[1];                                                    \
    update_extr_macro(t0, s0);                                       \
    update_extr_macro(t1, s1);                                       \
    dst[i] = (arrtype)toggle_macro(t0);                              \
    dst[i + 1] = (arrtype)toggle_macro(t1);                          \
    t0 = sptr[2];                                                    \
    t1 = sptr[3];                                                    \
    update_extr_macro(t0, s2);                                       \
    update_extr_macro(t1, s3);                                       \
    dst[i + 2] = (arrtype)toggle_macro(t0);                          \
    dst[i + 3] = (arrtype)toggle_macro(t1);                          \
    \
    sptr = src[k] + i;                                               \
    t0 = sptr[0];                                                    \
    t1 = sptr[1];                                                    \
    update_extr_macro(t0, s0);                                       \
    update_extr_macro(t1, s1);                                       \
    dst[i + dst_step] = (arrtype)toggle_macro(t0);                   \
    dst[i + dst_step + 1] = (arrtype)toggle_macro(t1);               \
    t0 = sptr[2];                                                    \
    t1 = sptr[3];                                                    \
    update_extr_macro(t0, s2);                                       \
    update_extr_macro(t1, s3);                                       \
    dst[i + dst_step + 2] = (arrtype)toggle_macro(t0);               \
    dst[i + dst_step + 3] = (arrtype)toggle_macro(t1);               \
    }                                                                    \
    \
    for (; i < width; i++)                                               \
{                                                                    \
    const arrtype *sptr = src[1] + i;                                \
    worktype s0 = sptr[0], t0;                                       \
    \
    for (k = 2; k < ksize; k++)                                      \
{                                                                \
    sptr = src[k] + i;                                           \
    t0 = sptr[0];                                                \
    update_extr_macro(s0, t0);                                   \
    }                                                                \
    \
    sptr = src[0] + i;                                               \
    t0 = sptr[0];                                                    \
    update_extr_macro(t0, s0);                                       \
    dst[i] = (arrtype)toggle_macro(t0);                              \
    \
    sptr = src[k] + i;                                               \
    t0 = sptr[0];                                                    \
    update_extr_macro(t0, s0);                                       \
    dst[i + dst_step] = (arrtype)toggle_macro(t0);                   \
    }                                                                    \
    }                                                                        \
    \
    for (; count > 0; count--, dst += dst_step, src++)                       \
{                                                                        \
    for (i = 0; i <= width - 4; i += 4)                                  \
{                                                                    \
    const arrtype *sptr = src[0] + i;                                \
    worktype s0 = sptr[0], s1 = sptr[1],                             \
    s2 = sptr[2], s3 = sptr[3], t0, t1;                     \
    \
    for (k = 1; k < ksize; k++)                                      \
{                                                                \
    sptr = src[k] + i;                                           \
    t0 = sptr[0];                                                \
    t1 = sptr[1];                                                \
    update_extr_macro(s0, t0);                                   \
    update_extr_macro(s1, t1);                                   \
    t0 = sptr[2];                                                \
    t1 = sptr[3];                                                \
    update_extr_macro(s2, t0);                                   \
    update_extr_macro(s3, t1);                                   \
    }                                                                \
    dst[i] = (arrtype)toggle_macro(s0);                              \
    dst[i + 1] = (arrtype)toggle_macro(s1);                          \
    dst[i + 2] = (arrtype)toggle_macro(s2);                          \
    dst[i + 3] = (arrtype)toggle_macro(s3);                          \
    }                                                                    \
    \
    for (; i < width; i++)                                               \
{                                                                    \
    const arrtype *sptr = src[0] + i;                                \
    worktype s0 = sptr[0], t0;                                       \
    \
    for (k = 1; k < ksize; k++)                                      \
{                                                                \
    sptr = src[k] + i;                                           \
    t0 = sptr[0];                                                \
    update_extr_macro(s0, t0);                                   \
    }                                                                \
    dst[i] = (arrtype)toggle_macro(s0);                              \
    }                                                                    \
    }                                                                        \
    \
    }

IVOS_MORPH_RECT_COL(Erode, 8u, uchar, int, VOS_CALC_MIN_8U, VOS_NOP)
IVOS_MORPH_RECT_COL(Dilate, 8u, uchar, int, VOS_CALC_MAX_8U, VOS_NOP)


#define IVOS_MORPH_ANY(name, flavor, arrtype, worktype,                     \
    update_extr_macro, toggle_macro)                     \
    \
    static void \
    icv##name##Any_##flavor(const arrtype **src, arrtype *dst,                 \
    int dst_step, int count, void *params)             \
    \
{                                                                     \
    Morphology *state = (Morphology *)params;                      \
    int width = state->get_width();                                    \
    int cn = VOS_MAT_CN(state->get_src_type());                         \
    int i, k;                                                          \
    Point *el_sparse = (Point *)state->get_element_sparse_buf();   \
    int el_count = state->get_element_sparse_count();                  \
    const arrtype **el_ptr = (const arrtype **)(el_sparse + el_count); \
    const arrtype **el_end = el_ptr + el_count;                        \
    \
    width *= cn;                                                       \
    dst_step /= sizeof(dst[0]);                                        \
    \
    for (; count > 0; count--, dst += dst_step, src++)                 \
{                                                                  \
    for (k = 0; k < el_count; k++)                                 \
    el_ptr[k] = src[el_sparse[k].y] + el_sparse[k].x;          \
    \
    for (i = 0; i <= width - 4; i += 4)                            \
{                                                              \
    const arrtype **psptr = el_ptr;                            \
    const arrtype *sptr = *psptr++;                            \
    worktype s0 = sptr[i], s1 = sptr[i + 1],                   \
    s2 = sptr[i + 2], s3 = sptr[i + 3], t;            \
    \
    while (psptr != el_end)                                    \
{                                                          \
    sptr = *psptr++;                                       \
    t = sptr[i];                                           \
    update_extr_macro(s0, t);                              \
    t = sptr[i + 1];                                       \
    update_extr_macro(s1, t);                              \
    t = sptr[i + 2];                                       \
    update_extr_macro(s2, t);                              \
    t = sptr[i + 3];                                       \
    update_extr_macro(s3, t);                              \
    }                                                          \
    \
    dst[i] = (arrtype)toggle_macro(s0);                        \
    dst[i + 1] = (arrtype)toggle_macro(s1);                    \
    dst[i + 2] = (arrtype)toggle_macro(s2);                    \
    dst[i + 3] = (arrtype)toggle_macro(s3);                    \
    }                                                              \
    \
    for (; i < width; i++)                                         \
{                                                              \
    const arrtype *sptr = el_ptr[0] + i;                       \
    worktype s0 = sptr[0], t0;                                 \
    \
    for (k = 1; k < el_count; k++)                             \
{                                                          \
    sptr = el_ptr[k] + i;                                  \
    t0 = sptr[0];                                          \
    update_extr_macro(s0, t0);                             \
    }                                                          \
    \
    dst[i] = (arrtype)toggle_macro(s0);                        \
    }                                                              \
    }                                                                  \
    \
    }

IVOS_MORPH_ANY(Erode, 8u, uchar, int, VOS_CALC_MIN, VOS_NOP)
IVOS_MORPH_ANY(Dilate, 8u, uchar, int, VOS_CALC_MAX, VOS_NOP)

/////////////////////////////////// External Interface /////////////////////////////////////

IplConvKernel *
CreateStructuringElementEx(int cols, int rows,
                           int anchorX, int anchorY,
                           int shape, int *values)
{
    IplConvKernel *element = 0;
    int i, size = rows * cols;
    int element_size = sizeof(*element) + size * sizeof(element->values[0]);

    VOS_FUNCNAME("CreateStructuringElementEx");

    __BEGIN__;

    if (!values && shape == VOS_SHAPE_CUSTOM)
        VOS_ERROR_FROM_STATUS(VOS_NULLPTR_ERR);

    if (cols <= 0 || rows <= 0 ||
            (unsigned)anchorX >= (unsigned)cols ||
            (unsigned)anchorY >= (unsigned)rows)
        VOS_ERROR_FROM_STATUS(VOS_BADSIZE_ERR);

    VOS_CALL(element = (IplConvKernel *)SysAlloc(element_size + 32));
    if (!element)
        VOS_ERROR_FROM_STATUS(VOS_OUTOFMEM_ERR);

    element->nCols = cols;
    element->nRows = rows;
    element->anchorX = anchorX;
    element->anchorY = anchorY;
    element->nShiftR = shape < VOS_SHAPE_ELLIPSE ? shape : VOS_SHAPE_CUSTOM;
    element->values = (int *)(element + 1);

    if (shape == VOS_SHAPE_CUSTOM)
    {
        if (!values)
            VOS_ERROR(VOS_StsNullPtr, "Null pointer to the custom element mask");
        for (i = 0; i < size; i++)
            element->values[i] = values[i];
    }
    else
    {
        Mat el_hdr = InitMat(rows, cols, VOS_32SC1, element->values);
        VOS_CALL(Morphology::init_binary_element(&el_hdr,
                                                 shape, InitPoint(anchorX, anchorY)));
    }

    __END__;

    if (GetErrStatus() < 0)
        ReleaseStructuringElement(&element);

    return element;
}

void
ReleaseStructuringElement(IplConvKernel **element)
{
    VOS_FUNCNAME("ReleaseStructuringElement");

    __BEGIN__;

    if (!element)
        VOS_ERROR(VOS_StsNullPtr, "");
    SYS_FREE(element);

    __END__;
}

static void
iMorphOp(const void *srcarr, void *dstarr, IplConvKernel *element,
         int iterations, int mop)
{
    Morphology morphology;
    void *buffer = 0;
    int local_alloc = 0;
    Mat *temp = 0;

    VOS_FUNCNAME("iMorphOp");

    __BEGIN__;

    int i, coi1 = 0, coi2 = 0;
    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    Mat el_hdr, *el = 0;
    Size size, el_size;
    Point el_anchor;
    int el_shape;

    if (!VOS_IS_MAT(src))
        VOS_CALL(src = GetMat(src, &srcstub, &coi1));

    if (src != &srcstub)
    {
        srcstub = *src;
        src = &srcstub;
    }

    if (dstarr == srcarr)
        dst = src;
    else
    {
        VOS_CALL(dst = GetMat(dst, &dststub, &coi2));

        if (!VOS_ARE_TYPES_EQ(src, dst))
            VOS_ERROR(VOS_StsUnmatchedFormats, "");

        if (!VOS_ARE_SIZES_EQ(src, dst))
            VOS_ERROR(VOS_StsUnmatchedSizes, "");
    }

    if (dst != &dststub)
    {
        dststub = *dst;
        dst = &dststub;
    }

    if (coi1 != 0 || coi2 != 0)
        VOS_ERROR(VOS_BadCOI, "");

    size = GetMatSize(src);

    if (iterations == 0 || (element && element->nCols == 1 && element->nRows == 1))
    {
        if (src->data.ptr != dst->data.ptr)
            Copy(src, dst);
        EXIT;
    }

    if (element)
    {
        el_size = GetSize(element->nCols, element->nRows);
        el_anchor = InitPoint(element->anchorX, element->anchorY);
        el_shape = (int)(element->nShiftR);
        el_shape = el_shape < VOS_SHAPE_CUSTOM ? el_shape : VOS_SHAPE_CUSTOM;
    }
    else
    {
        el_size = GetSize(3, 3);
        el_anchor = InitPoint(1, 1);
        el_shape = VOS_SHAPE_RECT;
    }

    if (el_shape == VOS_SHAPE_RECT && iterations > 1)
    {
        el_size.width += (el_size.width - 1) * iterations;
        el_size.height += (el_size.height - 1) * iterations;
        el_anchor.x *= iterations;
        el_anchor.y *= iterations;
        iterations = 1;
    }

    if (el_shape != VOS_SHAPE_RECT)
    {
        el_hdr = InitMat(element->nRows, element->nCols, VOS_32SC1, element->values);
        el = &el_hdr;
        el_shape = VOS_SHAPE_CUSTOM;
    }

    VOS_CALL(morphology.init(mop, src->cols, src->type,
                             el_shape, el, el_size, el_anchor));

    for (i = 0; i < iterations; i++)
    {
        VOS_CALL(morphology.process(src, dst));
        src = dst;
    }

    __END__;

    if (!local_alloc)
        SYS_FREE(&buffer);

    ReleaseMat(&temp);
}

void
Erode(const void *src, void *dst, IplConvKernel *element, int iterations)
{
    iMorphOp(src, dst, element, iterations, 0);
}

void
Dilate(const void *src, void *dst, IplConvKernel *element, int iterations)
{
    iMorphOp(src, dst, element, iterations, 1);
}

/* End of file. */

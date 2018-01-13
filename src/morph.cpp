
#include "process.h"
#include "misc.h"
#include <limits.h>
#include <stdio.h>

static void iErodeRectRow_8u(const uchar *src, uchar *dst, void *params);
static void iDilateRectRow_8u(const uchar *src, uchar *dst, void *params);

static void iErodeRectCol_8u(const uchar **src, uchar *dst, int dst_step,
                             int count, void *params);
static void iDilateRectCol_8u(const uchar **src, uchar *dst, int dst_step,
                              int count, void *params);

Morphology::Morphology()
{
}

Morphology::Morphology(int _operation, int _max_width, int _src_dst_type,
                       Size _ksize, Point _anchor )
{
    init(_operation, _max_width, _src_dst_type,
          _ksize, _anchor );
}

void Morphology::clear()
{
    BaseImageFilter::clear();
}

Morphology::~Morphology()
{
    clear();
}

void Morphology::init(int _operation, int _max_width, int _src_dst_type,
                      Size _ksize, Point _anchor )
{
    VOS_FUNCNAME("Morphology::init");

    __BEGIN__;

    int depth = VOS_MAT_DEPTH(_src_dst_type);

    if (_operation != ERODE && _operation != DILATE)
        VOS_ERROR(VOS_StsBadArg, "Unknown/unsupported morphological operation");

    operation = _operation;

    VOS_CALL(BaseImageFilter::init(_max_width, _src_dst_type, _src_dst_type,
                                   true, _ksize, _anchor ));
    if (VOS_8U != depth)
    {
        VOS_ERROR(VOS_BadDepth, "");
    }

        if ( ERODE==operation )
        {
            x_func = (RowFilterFunc)iErodeRectRow_8u;
            y_func = (ColumnFilterFunc)iErodeRectCol_8u;
        }
        else if ( DILATE==operation )
        {
            x_func = (RowFilterFunc)iDilateRectRow_8u;
            y_func = (ColumnFilterFunc)iDilateRectCol_8u;
        }
        else
        {
            VOS_ERROR(VOS_StsNotImplemented, "");
        }

    __END__;
}

void Morphology::init(int _max_width, int _src_type, int _dst_type,
                      bool _is_separable, Size _ksize,
                      Point _anchor )
{
    BaseImageFilter::init(_max_width, _src_type, _dst_type, _is_separable,
                          _ksize, _anchor );
}

void Morphology::start_process(Slice x_range, int width)
{
    BaseImageFilter::start_process(x_range, width);

        int t = buf_max_count - max_ky * 2;
        if (t > 1 && t % 2 != 0)
        {
            buf_max_count--;
            buf_end -= buf_step;
        }
}

int Morphology::fill_cyclic_buffer(const uchar *src, int src_step,
                                   int y0, int y1, int y2)
{
    return BaseImageFilter::fill_cyclic_buffer(src, src_step, y0, y1, y2);
}

#define IVOS_MORPH_RECT_ROW(name, flavor, arrtype,            \
                            worktype, update_extr_macro)      \
                                                              \
    static void                                               \
        i##name##RectRow_##flavor(const arrtype *src,         \
                                  arrtype *dst, void *params) \
    \
{                                                        \
        const Morphology *state = (const Morphology *)params; \
        int ksize = state->get_kernel_size().width;           \
        int width = state->get_width();                       \
        int cn = VOS_MAT_CN(state->get_src_type());           \
        int i, j, k;                                          \
                                                              \
        width *= cn;                                          \
        ksize *= cn;                                          \
                                                              \
        if (ksize == cn)                                      \
        \
{                                                    \
            for (i = 0; i < width; i++)                       \
                dst[i] = src[i];                              \
            return;                                           \
        }                                                     \
                                                              \
        for (k = 0; k < cn; k++, src++, dst++)                \
        \
{                                                    \
            for (i = 0; i <= width - cn * 2; i += cn * 2)     \
            \
{                                                \
                const arrtype *s = src + i;                   \
                worktype m = s[cn], t;                        \
                for (j = cn * 2; j < ksize; j += cn)          \
                \
{                                            \
                    t = s[j];                                 \
                    update_extr_macro(m, t);                  \
                }                                             \
                t = s[0];                                     \
                update_extr_macro(t, m);                      \
                dst[i] = (arrtype)t;                          \
                t = s[j];                                     \
                update_extr_macro(t, m);                      \
                dst[i + cn] = (arrtype)t;                     \
            }                                                 \
                                                              \
            for (; i < width; i += cn)                        \
            \
{                                                \
                const arrtype *s = src + i;                   \
                worktype m = s[0], t;                         \
                for (j = cn; j < ksize; j += cn)              \
                \
{                                            \
                    t = s[j];                                 \
                    update_extr_macro(m, t);                  \
                }                                             \
                dst[i] = (arrtype)m;                          \
            }                                                 \
        }                                                     \
    }

IVOS_MORPH_RECT_ROW(Erode, 8u, uchar, int, VOS_CALC_MIN_8U);
IVOS_MORPH_RECT_ROW(Dilate, 8u, uchar, int, VOS_CALC_MAX_8U);

#define IVOS_MORPH_RECT_COL(name, flavor, arrtype,                                     \
                            worktype, update_extr_macro, toggle_macro)                 \
                                                                                       \
    static void                                                                        \
        i##name##RectCol_##flavor(const arrtype **src,                                 \
                                  arrtype *dst, int dst_step, int count, void *params) \
    \
{                                                                                 \
        const Morphology *state = (const Morphology *)params;                          \
        int ksize = state->get_kernel_size().height;                                   \
        int width = state->get_width();                                                \
        int cn = VOS_MAT_CN(state->get_src_type());                                    \
        int i, k;                                                                      \
                                                                                       \
        width *= cn;                                                                   \
        dst_step /= sizeof(dst[0]);                                                    \
                                                                                       \
        for (; ksize > 1 && count > 1; count -= 2,                                     \
                                       dst += dst_step * 2, src += 2)                  \
        \
{                                                                             \
            for (i = 0; i <= width - 4; i += 4)                                        \
            \
{                                                                         \
                const arrtype *sptr = src[1] + i;                                      \
                worktype s0 = sptr[0], s1 = sptr[1],                                   \
                         s2 = sptr[2], s3 = sptr[3], t0, t1;                           \
                                                                                       \
                for (k = 2; k < ksize; k++)                                            \
                \
{                                                                     \
                    sptr = src[k] + i;                                                 \
                    t0 = sptr[0];                                                      \
                    t1 = sptr[1];                                                      \
                    update_extr_macro(s0, t0);                                         \
                    update_extr_macro(s1, t1);                                         \
                    t0 = sptr[2];                                                      \
                    t1 = sptr[3];                                                      \
                    update_extr_macro(s2, t0);                                         \
                    update_extr_macro(s3, t1);                                         \
                }                                                                      \
                                                                                       \
                sptr = src[0] + i;                                                     \
                t0 = sptr[0];                                                          \
                t1 = sptr[1];                                                          \
                update_extr_macro(t0, s0);                                             \
                update_extr_macro(t1, s1);                                             \
                dst[i] = (arrtype)toggle_macro(t0);                                    \
                dst[i + 1] = (arrtype)toggle_macro(t1);                                \
                t0 = sptr[2];                                                          \
                t1 = sptr[3];                                                          \
                update_extr_macro(t0, s2);                                             \
                update_extr_macro(t1, s3);                                             \
                dst[i + 2] = (arrtype)toggle_macro(t0);                                \
                dst[i + 3] = (arrtype)toggle_macro(t1);                                \
                                                                                       \
                sptr = src[k] + i;                                                     \
                t0 = sptr[0];                                                          \
                t1 = sptr[1];                                                          \
                update_extr_macro(t0, s0);                                             \
                update_extr_macro(t1, s1);                                             \
                dst[i + dst_step] = (arrtype)toggle_macro(t0);                         \
                dst[i + dst_step + 1] = (arrtype)toggle_macro(t1);                     \
                t0 = sptr[2];                                                          \
                t1 = sptr[3];                                                          \
                update_extr_macro(t0, s2);                                             \
                update_extr_macro(t1, s3);                                             \
                dst[i + dst_step + 2] = (arrtype)toggle_macro(t0);                     \
                dst[i + dst_step + 3] = (arrtype)toggle_macro(t1);                     \
            }                                                                          \
                                                                                       \
            for (; i < width; i++)                                                     \
            \
{                                                                         \
                const arrtype *sptr = src[1] + i;                                      \
                worktype s0 = sptr[0], t0;                                             \
                                                                                       \
                for (k = 2; k < ksize; k++)                                            \
                \
{                                                                     \
                    sptr = src[k] + i;                                                 \
                    t0 = sptr[0];                                                      \
                    update_extr_macro(s0, t0);                                         \
                }                                                                      \
                                                                                       \
                sptr = src[0] + i;                                                     \
                t0 = sptr[0];                                                          \
                update_extr_macro(t0, s0);                                             \
                dst[i] = (arrtype)toggle_macro(t0);                                    \
                                                                                       \
                sptr = src[k] + i;                                                     \
                t0 = sptr[0];                                                          \
                update_extr_macro(t0, s0);                                             \
                dst[i + dst_step] = (arrtype)toggle_macro(t0);                         \
            }                                                                          \
        }                                                                              \
                                                                                       \
        for (; count > 0; count--, dst += dst_step, src++)                             \
        \
{                                                                             \
            for (i = 0; i <= width - 4; i += 4)                                        \
            \
{                                                                         \
                const arrtype *sptr = src[0] + i;                                      \
                worktype s0 = sptr[0], s1 = sptr[1],                                   \
                         s2 = sptr[2], s3 = sptr[3], t0, t1;                           \
                                                                                       \
                for (k = 1; k < ksize; k++)                                            \
                \
{                                                                     \
                    sptr = src[k] + i;                                                 \
                    t0 = sptr[0];                                                      \
                    t1 = sptr[1];                                                      \
                    update_extr_macro(s0, t0);                                         \
                    update_extr_macro(s1, t1);                                         \
                    t0 = sptr[2];                                                      \
                    t1 = sptr[3];                                                      \
                    update_extr_macro(s2, t0);                                         \
                    update_extr_macro(s3, t1);                                         \
                }                                                                      \
                dst[i] = (arrtype)toggle_macro(s0);                                    \
                dst[i + 1] = (arrtype)toggle_macro(s1);                                \
                dst[i + 2] = (arrtype)toggle_macro(s2);                                \
                dst[i + 3] = (arrtype)toggle_macro(s3);                                \
            }                                                                          \
                                                                                       \
            for (; i < width; i++)                                                     \
            \
{                                                                         \
                const arrtype *sptr = src[0] + i;                                      \
                worktype s0 = sptr[0], t0;                                             \
                                                                                       \
                for (k = 1; k < ksize; k++)                                            \
                \
{                                                                     \
                    sptr = src[k] + i;                                                 \
                    t0 = sptr[0];                                                      \
                    update_extr_macro(s0, t0);                                         \
                }                                                                      \
                dst[i] = (arrtype)toggle_macro(s0);                                    \
            }                                                                          \
        }                                                                              \
    }

IVOS_MORPH_RECT_COL(Erode, 8u, uchar, int, VOS_CALC_MIN_8U, VOS_NOP);
IVOS_MORPH_RECT_COL(Dilate, 8u, uchar, int, VOS_CALC_MAX_8U, VOS_NOP);

static void iMorphOp(const void *srcarr, void *dstarr, int iterations, int mop)
{
    Morphology morphology;
    void *buffer = NULL;
    int local_alloc = 0;
    Mat *temp = NULL;

    VOS_FUNCNAME("iMorphOp");

    __BEGIN__;

    int i, coi1 = 0, coi2 = 0;
    Mat srcstub, *src = (Mat *)srcarr;
    Mat dststub, *dst = (Mat *)dstarr;
    Size size, el_size;
    Point el_anchor;

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

    size = GetMatSize(src);

    if (0 == iterations)
    {
        if (src->data.ptr != dst->data.ptr)
            Copy(src, dst);
        EXIT;
    }

    el_size = GetSize(3, 3);
    el_anchor = InitPoint(1, 1);

    if (iterations > 1)
    {
        el_size.width += (el_size.width - 1) * iterations;
        el_size.height += (el_size.height - 1) * iterations;
        el_anchor.x *= iterations;
        el_anchor.y *= iterations;
        iterations = 1;
    }

    VOS_CALL(morphology.init(mop, src->cols, src->type,
                              el_size, el_anchor));

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

void Erode(const void *src, void *dst,  int iterations)
{
    iMorphOp(src, dst,iterations, ERODE);
}

void Dilate(const void *src, void *dst,  int iterations)
{
    iMorphOp(src, dst, iterations, DILATE);
}

/* End of file. */

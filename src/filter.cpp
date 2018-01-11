
#include "process.h"
#include "misc.h"

static void default_x_filter_func(const uchar *, uchar *, void *)
{
}

static void default_y_filter_func(uchar **, uchar *, int, int, void *)
{
}

BaseImageFilter::BaseImageFilter()
{
    min_depth = VOS_8U;
    buffer = NULL;
    rows = NULL;
    max_width = 0;
    x_func = default_x_filter_func;
    y_func = default_y_filter_func;
}

void BaseImageFilter::clear()
{
    SYS_FREE(&buffer);
    rows = NULL;
}

BaseImageFilter::~BaseImageFilter()
{
    clear();
}

void BaseImageFilter::get_work_params()
{
    int min_rows = max_ky * 2 + 3, rows = VOS_MAX(min_rows, 10), row_sz;
    int width = max_width, trow_sz = 0;

    if (is_separable)
    {
        int max_depth = VOS_MAX(VOS_MAT_DEPTH(src_type), VOS_MAT_DEPTH(dst_type));
        int max_cn = VOS_MAX(VOS_MAT_CN(src_type), VOS_MAT_CN(dst_type));
        max_depth = VOS_MAX(max_depth, min_depth);
        work_type = VOS_MAKETYPE(max_depth, max_cn);
        trow_sz = Align((max_width + ksize.width - 1) * VOS_ELEM_SIZE(src_type), ALIGN);
    }
    else
    {
        work_type = src_type;
        width += ksize.width - 1;
    }
    row_sz = Align(width * VOS_ELEM_SIZE(work_type), ALIGN);
    buf_size = rows * row_sz;
    buf_size = VOS_MIN(buf_size, 1 << 16);
    buf_size = VOS_MAX(buf_size, min_rows * row_sz);
    max_rows = (buf_size / row_sz) * 3 + max_ky * 2 + 8;
    buf_size += trow_sz;
}

void BaseImageFilter::init(int _max_width, int _src_type, int _dst_type,
                           bool _is_separable, Size _ksize, Point _anchor,
                           int _border_mode, Scalar _border_value)
{
    VOS_FUNCNAME("BaseImageFilter::init");

    __BEGIN__;

    int total_buf_sz, src_pix_sz, row_tab_sz, bsz;
    uchar *ptr;

    if (!(buffer && _max_width <= max_width && _src_type == src_type &&
          _dst_type == dst_type && _is_separable == is_separable &&
          _ksize.width == ksize.width && _ksize.height == ksize.height &&
          _anchor.x == anchor.x && _anchor.y == anchor.y))
        clear();

    is_separable = 0 != _is_separable;
    max_width = _max_width; //MAX(_max_width,_ksize.width);
    src_type = VOS_MAT_TYPE(_src_type);
    dst_type = VOS_MAT_TYPE(_dst_type);
    ksize = _ksize;
    anchor = _anchor;

    if (anchor.x == -1)
        anchor.x = ksize.width / 2;
    if (anchor.y == -1)
        anchor.y = ksize.height / 2;

    max_ky = VOS_MAX(anchor.y, ksize.height - anchor.y - 1);
    border_mode = _border_mode;
    border_value = _border_value;

    if (ksize.width <= 0 || ksize.height <= 0 ||
        (unsigned)anchor.x >= (unsigned)ksize.width ||
        (unsigned)anchor.y >= (unsigned)ksize.height)
        VOS_ERROR(VOS_StsOutOfRange, "invalid kernel size and/or anchor position");

    if (SYS_BORDER_REPLICATE != border_mode)
        VOS_ERROR(VOS_StsBadArg, "Invalid/unsupported border mode");

    get_work_params();

    prev_width = 0;
    prev_x_range = GetSlice(0, 0);

    buf_size = Align(buf_size, ALIGN);

    src_pix_sz = VOS_ELEM_SIZE(src_type);
    border_tab_sz1 = anchor.x * src_pix_sz;
    border_tab_sz = (ksize.width - 1) * src_pix_sz;
    bsz = Align(border_tab_sz * sizeof(int), ALIGN);

    assert(max_rows > max_ky * 2);
    row_tab_sz = Align(max_rows * sizeof(uchar *), ALIGN);
    total_buf_sz = buf_size + row_tab_sz + bsz;

    VOS_CALL(ptr = buffer = (uchar *)SysAlloc(total_buf_sz));

    rows = (uchar **)ptr;
    ptr += row_tab_sz;
    border_tab = (int *)ptr;
    ptr += bsz;

    buf_start = ptr;
    const_row = NULL;

    __END__;
}

void BaseImageFilter::start_process(Slice x_range, int width)
{
    int pix_sz = VOS_ELEM_SIZE(src_type), work_pix_sz = VOS_ELEM_SIZE(work_type);
    int bsz = buf_size, bw = x_range.end_index - x_range.start_index, bw1 = bw + ksize.width - 1;
    int tr_step = Align(bw1 * pix_sz, ALIGN);
    int i, j, k, ofs;

    if (x_range.start_index == prev_x_range.start_index &&
        x_range.end_index == prev_x_range.end_index &&
        width == prev_width)
        return;

    prev_x_range = x_range;
    prev_width = width;

    if (!is_separable)
        bw = bw1;
    else
        bsz -= tr_step;

    buf_step = Align(bw * work_pix_sz, ALIGN);

    buf_max_count = bsz / buf_step;
    buf_max_count = VOS_MIN(buf_max_count, max_rows - max_ky * 2);
    buf_end = buf_start + buf_max_count * buf_step;

    width = (width - 1) * pix_sz;
    ofs = (anchor.x - x_range.start_index) * pix_sz;

    for (k = 0; k < 2; k++)
    {
        int idx, delta;
        int i1, i2, di;

        if (0 == k)
        {
            idx = (x_range.start_index - 1) * pix_sz;
            delta = di = -pix_sz;
            i1 = border_tab_sz1 - pix_sz;
            i2 = -pix_sz;
        }
        else
        {
            idx = x_range.end_index * pix_sz;
            delta = di = pix_sz;
            i1 = border_tab_sz1;
            i2 = border_tab_sz;
        }

        if ((unsigned)idx > (unsigned)width)
        {
            idx = 0 == k ? 0 : width;
            delta = -delta;
        }

        for (i = i1; i != i2; i += di)
        {
            for (j = 0; j < pix_sz; j++)
                border_tab[i + j] = idx + ofs + j;
        }
    }
}

void BaseImageFilter::make_y_border(int row_count, int top_rows, int bottom_rows)
{
    int i;
    uchar *row1 = rows[max_ky];
    for (i = 0; i < top_rows && 0 == rows[i]; i++)
        rows[i] = row1;

    row1 = rows[row_count - 1];
    for (i = 0; i < bottom_rows; i++)
        rows[i + row_count] = row1;
}

int BaseImageFilter::fill_cyclic_buffer(const uchar *src, int src_step,
                                        int y0, int y1, int y2)
{
    int i, y = y0, bsz1 = border_tab_sz1, bsz = border_tab_sz;
    int pix_size = VOS_ELEM_SIZE(src_type);
    int width = prev_x_range.end_index - prev_x_range.start_index, width_n = width * pix_size;
    bool can_use_src_as_trow = is_separable && width >= ksize.width;

    // fill the cyclic buffer
    for (; buf_count < buf_max_count && y < y2; buf_count++, y++, src += src_step)
    {
        uchar *trow = is_separable ? buf_end : buf_tail;
        uchar *bptr = can_use_src_as_trow && y1 < y && y + 1 < y2 ? (uchar *)(src - bsz1) : trow;

        if (bptr != trow)
        {
            for (i = 0; i < bsz1; i++)
                trow[i] = bptr[i];
            for (; i < bsz; i++)
                trow[i] = bptr[i + width_n];
        }
        else if (!(((size_t)(bptr + bsz1) | (size_t)src | width_n) & (sizeof(int) - 1)))
            for (i = 0; i < width_n; i += sizeof(int))
                *(int *)(bptr + i + bsz1) = *(int *)(src + i);
        else
        	{
            for (i = 0; i < width_n; i++)
                bptr[i + bsz1] = src[i];
        	}


            for (i = 0; i < bsz1; i++)
            {
                int j = border_tab[i];
                bptr[i] = bptr[j];
            }
            for (; i < bsz; i++)
            {
                int j = border_tab[i];
                bptr[i + width_n] = bptr[j];
            }
       
  
        if (is_separable)
        {
            x_func(bptr, buf_tail, this);
            if (bptr != trow)
            {
                for (i = 0; i < bsz1; i++)
                    bptr[i] = trow[i];
                for (; i < bsz; i++)
                    bptr[i + width_n] = trow[i];
            }
        }

        buf_tail += buf_step;
        if (buf_tail >= buf_end)
            buf_tail = buf_start;
    }

    return y - y0;
}

int BaseImageFilter::process(const Mat *src, Mat *dst,
                             Rect src_roi, Point dst_origin, int flags)
{
    int rows_processed = 0;

    VOS_FUNCNAME("BaseImageFilter::process");

    __BEGIN__;

    int i, width, _src_y1, _src_y2;
    int src_x, src_y, src_y1, src_y2, dst_y;
    int pix_size = VOS_ELEM_SIZE(src_type);
    uchar *sptr = NULL, *dptr;
    int phase = flags & (VOS_START | VOS_END | VOS_MIDDLE);
    bool isolated_roi = (flags & VOS_ISOLATED_ROI) != 0;

    if (!VOS_IS_MAT(src))
        VOS_ERROR(VOS_StsBadArg, "");

    if (VOS_MAT_TYPE(src->type) != src_type)
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    width = src->cols;

    if (-1 == src_roi.width && 0 == src_roi.x)
        src_roi.width = width;

    if (-1 == src_roi.height && 0 == src_roi.y)
    {
        src_roi.y = 0;
        src_roi.height = src->rows;
    }

    if (src_roi.width > max_width ||
        src_roi.x < 0 || src_roi.width < 0 ||
        src_roi.y < 0 || src_roi.height < 0 ||
        src_roi.x + src_roi.width > width ||
        src_roi.y + src_roi.height > src->rows)
        VOS_ERROR(VOS_StsOutOfRange, "Too large source image or its ROI");

    src_x = src_roi.x;
    _src_y1 = 0;
    _src_y2 = src->rows;

    if (isolated_roi)
    {
        src_roi.x = 0;
        width = src_roi.width;
        _src_y1 = src_roi.y;
        _src_y2 = src_roi.y + src_roi.height;
    }

    if (!VOS_IS_MAT(dst))
        VOS_ERROR(VOS_StsBadArg, "");

    if (VOS_MAT_TYPE(dst->type) != dst_type)
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    if (dst_origin.x < 0 || dst_origin.y < 0)
        VOS_ERROR(VOS_StsOutOfRange, "Incorrect destination ROI origin");

    if ( VOS_WHOLE==phase )
        phase = VOS_START | VOS_END;
    phase &= VOS_START | VOS_END | VOS_MIDDLE;

    if (phase & VOS_START)
        start_process(GetSlice(src_roi.x, src_roi.x + src_roi.width), width);
    else if (prev_width != width || prev_x_range.start_index != src_roi.x ||
             prev_x_range.end_index != src_roi.x + src_roi.width)
        VOS_ERROR(VOS_StsBadArg,
                  "In a middle or at the end the horizontal placement of the stripe can not be changed");

    dst_y = dst_origin.y;
    src_y1 = src_roi.y;
    src_y2 = src_roi.y + src_roi.height;

    if (phase & VOS_START)
    {
        for (i = 0; i <= max_ky * 2; i++)
            rows[i] = 0;

        src_y1 -= max_ky;
        top_rows = bottom_rows = 0;

        if (src_y1 < _src_y1)
        {
            top_rows = _src_y1 - src_y1;
            src_y1 = _src_y1;
        }

        buf_head = buf_tail = buf_start;
        buf_count = 0;
    }

    if (phase & VOS_END)
    {
        src_y2 += max_ky;

        if (src_y2 > _src_y2)
        {
            bottom_rows = src_y2 - _src_y2;
            src_y2 = _src_y2;
        }
    }

    dptr = dst->data.ptr + dst_origin.y * dst->step + dst_origin.x * VOS_ELEM_SIZE(dst_type);
    sptr = src->data.ptr + src_y1 * src->step + src_x * pix_size;

    for (src_y = src_y1; src_y < src_y2;)
    {
        uchar *bptr;
        int row_count, delta;

        delta = fill_cyclic_buffer(sptr, src->step, src_y, src_y1, src_y2);

        src_y += delta;
        sptr += src->step * delta;

        bptr = buf_head;
        for (i = 0; i < buf_count; i++)
        {
            rows[i + top_rows] = bptr;
            bptr += buf_step;
            if (bptr >= buf_end)
                bptr = buf_start;
        }

        row_count = top_rows + buf_count;

        if (!rows[0] || ((phase & VOS_END) && src_y == src_y2))
        {
            int br = (phase & VOS_END) && src_y == src_y2 ? bottom_rows : 0;
            make_y_border(row_count, top_rows, br);
            row_count += br;
        }

        if (rows[0] && row_count > max_ky * 2)
        {
            int count = row_count - max_ky * 2;
            if (dst_y + count > dst->rows)
                VOS_ERROR(VOS_StsOutOfRange, "The destination image can not fit the result");

            assert(count >= 0);
            y_func(rows + max_ky - anchor.y, dptr, dst->step, count, this);
            row_count -= count;
            dst_y += count;
            dptr += dst->step * count;
            for (bptr = row_count > 0 ? rows[count] : 0; buf_head != bptr && buf_count > 0; buf_count--)
            {
                buf_head += buf_step;
                if (buf_head >= buf_end)
                    buf_head = buf_start;
            }
            rows_processed += count;
            top_rows = VOS_MAX(top_rows - count, 0);
        }
    }

    __END__;

    return rows_processed;
}

static void iFilterRowSymm_8u32s(const uchar *src, int *dst, void *params);
static void iFilterColSymm_32s8u(const int **src, uchar *dst, int dst_step,
                                 int count, void *params);
static void iFilterColSymm_32s16s(const int **src, short *dst, int dst_step,
                                  int count, void *params);

SepFilter::SepFilter()
{
    min_depth = VOS_32F;
    kx = ky = NULL;
    kx_flags = ky_flags = 0;
}

void SepFilter::clear()
{
    ReleaseMat(&kx);
    ReleaseMat(&ky);
    BaseImageFilter::clear();
}

SepFilter::~SepFilter()
{
    clear();
}

#undef FILTER_BITS
#define FILTER_BITS 8

void SepFilter::init(int _max_width, int _src_type, int _dst_type,
                     const Mat *_kx, const Mat *_ky,
                     Point _anchor, int _border_mode,
                     Scalar _border_value)
{
    VOS_FUNCNAME("SepFilter::init");

    __BEGIN__;

    Size _ksize;
    int filter_type;
    int i, xsz, ysz;
    int convert_filters = 0;
    double xsum = 0, ysum = 0;
    const float eps = FLT_EPSILON * 100.f;

    if (VOS_MAT_CN(_src_type) != VOS_MAT_CN(_dst_type))
        VOS_ERROR(VOS_StsUnmatchedFormats, "Input and output must have the same number of channels");

    filter_type = VOS_MAX(VOS_32F, VOS_MAT_DEPTH(_kx->type));

    _ksize.width = _kx->rows + _kx->cols - 1;
    _ksize.height = _ky->rows + _ky->cols - 1;

    VOS_CALL(BaseImageFilter::init(_max_width, _src_type, _dst_type, 1, _ksize,
                                   _anchor, _border_mode, _border_value));

    if (!(kx && VOS_ARE_SIZES_EQ(kx, _kx)))
    {
        ReleaseMat(&kx);
        VOS_CALL(kx = CreateMat(_kx->rows, _kx->cols, filter_type));
    }

    if (!(ky && VOS_ARE_SIZES_EQ(ky, _ky)))
    {
        ReleaseMat(&ky);
        VOS_CALL(ky = CreateMat(_ky->rows, _ky->cols, filter_type));
    }

    VOS_CALL(Convert(_kx, kx));
    VOS_CALL(Convert(_ky, ky));

    xsz = kx->rows + kx->cols - 1;
    ysz = ky->rows + ky->cols - 1;
    kx_flags = ky_flags = ASYMMETRICAL + SYMMETRICAL + POSITIVE + SUM_TO_1 + INTEGER;

    if (!(xsz & 1))
        kx_flags &= ~(ASYMMETRICAL + SYMMETRICAL);
    if (!(ysz & 1))
        ky_flags &= ~(ASYMMETRICAL + SYMMETRICAL);

    for (i = 0; i < xsz; i++)
    {
        float v = kx->data.fl[i];
        xsum += v;
        if (v < 0)
            kx_flags &= ~POSITIVE;
        if (fabs(v - SysRound(v)) > eps)
            kx_flags &= ~INTEGER;
        if (fabs(v - kx->data.fl[xsz - i - 1]) > eps)
            kx_flags &= ~SYMMETRICAL;
        if (fabs(v + kx->data.fl[xsz - i - 1]) > eps)
            kx_flags &= ~ASYMMETRICAL;
    }

    if (fabs(xsum - 1.) > eps)
        kx_flags &= ~SUM_TO_1;

    for (i = 0; i < ysz; i++)
    {
        float v = ky->data.fl[i];
        ysum += v;
        if (v < 0)
            ky_flags &= ~POSITIVE;
        if (fabs(v - SysRound(v)) > eps)
            ky_flags &= ~INTEGER;
        if (fabs(v - ky->data.fl[ysz - i - 1]) > eps)
            ky_flags &= ~SYMMETRICAL;
        if (fabs(v + ky->data.fl[ysz - i - 1]) > eps)
            ky_flags &= ~ASYMMETRICAL;
    }

    if (fabs(ysum - 1.) > eps)
        ky_flags &= ~SUM_TO_1;

    x_func = NULL;
    y_func = NULL;

    if (VOS_MAT_DEPTH(src_type) == VOS_8U)
    {
        if (VOS_MAT_DEPTH(dst_type) == VOS_8U &&
            ((kx_flags & ky_flags) & (SYMMETRICAL + POSITIVE + SUM_TO_1)) == SYMMETRICAL + POSITIVE + SUM_TO_1)
        {
            x_func = (RowFilterFunc)iFilterRowSymm_8u32s;
            y_func = (ColumnFilterFunc)iFilterColSymm_32s8u;
            kx_flags &= ~INTEGER;
            ky_flags &= ~INTEGER;
            convert_filters = 1;
        }
        else if (VOS_MAT_DEPTH(dst_type) == VOS_16S &&
                 (kx_flags & (SYMMETRICAL + ASYMMETRICAL)) && (kx_flags & INTEGER) &&
                 (ky_flags & (SYMMETRICAL + ASYMMETRICAL)) && (ky_flags & INTEGER))
        {
            x_func = (RowFilterFunc)iFilterRowSymm_8u32s;
            y_func = (ColumnFilterFunc)iFilterColSymm_32s16s;
            convert_filters = 1;
        }
        else
        {
            VOS_ERROR(VOS_StsUnsupportedFormat, "Unknown or unsupported input data type");
        }
    }
    else
    {
        VOS_ERROR(VOS_StsUnsupportedFormat, "Unknown or unsupported input data type");
    }

    if (convert_filters)
    {
        int scale = kx_flags & ky_flags & INTEGER ? 1 : (1 << FILTER_BITS);
        int sum;

        for (i = sum = 0; i < xsz; i++)
        {
            int t = SysRound(kx->data.fl[i] * scale);
            kx->data.i[i] = t;
            sum += t;
        }
        if (scale > 1)
            kx->data.i[xsz / 2] += scale - sum;

        for (i = sum = 0; i < ysz; i++)
        {
            int t = SysRound(ky->data.fl[i] * scale);
            ky->data.i[i] = t;
            sum += t;
        }
        if (scale > 1)
            ky->data.i[ysz / 2] += scale - sum;
        kx->type = (kx->type & ~VOS_MAT_DEPTH_MASK) | VOS_32S;
        ky->type = (ky->type & ~VOS_MAT_DEPTH_MASK) | VOS_32S;
    }

    __END__;
}

void SepFilter::init(int _max_width, int _src_type, int _dst_type,
                     bool _is_separable, Size _ksize,
                     Point _anchor, int _border_mode,
                     Scalar _border_value)
{
    BaseImageFilter::init(_max_width, _src_type, _dst_type, _is_separable,
                          _ksize, _anchor, _border_mode, _border_value);
}

static void
iFilterRowSymm_8u32s(const uchar *src, int *dst, void *params)
{
    const SepFilter *state = (const SepFilter *)params;
    const Mat *_kx = state->get_x_kernel();
    const int *kx = _kx->data.i;
    int ksize = _kx->cols + _kx->rows - 1;
    int i = 0, j, k, width = state->get_width();
    int cn = VOS_MAT_CN(state->get_src_type());
    int ksize2 = ksize / 2, ksize2n = ksize2 * cn;
    int is_symm = state->get_x_kernel_flags() & SepFilter::SYMMETRICAL;
    const uchar *s = src + ksize2n;

    kx += ksize2;
    width *= cn;

    if (is_symm)
    {
        if (1 == ksize && 1 == kx[0])
        {
            for (i = 0; i <= width - 2; i += 2)
            {
                int s0 = s[i], s1 = s[i + 1];
                dst[i] = s0;
                dst[i + 1] = s1;
            }
            s += i;
        }
        else if (3 == ksize)
        {
            if (2 == kx[0] && 1 == kx[1])
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = s[-cn] + s[0] * 2 + s[cn], s1 = s[1 - cn] + s[1] * 2 + s[1 + cn];
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
            else if (10 == kx[0] && 3 == kx[1])
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = s[0] * 10 + (s[-cn] + s[cn]) * 3, s1 = s[1] * 10 + (s[1 - cn] + s[1 + cn]) * 3;
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
            else if (2 * 64 == kx[0] && 1 * 64 == kx[1])
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = (s[0] * 2 + s[-cn] + s[cn]) << 6;
                    int s1 = (s[1] * 2 + s[1 - cn] + s[1 + cn]) << 6;
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
            else
            {
                int k0 = kx[0], k1 = kx[1];
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = s[0] * k0 + (s[-cn] + s[cn]) * k1, s1 = s[1] * k0 + (s[1 - cn] + s[1 + cn]) * k1;
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
            }
        }
        else if (ksize == 5)
        {
            int k0 = kx[0], k1 = kx[1], k2 = kx[2];
            if (6 * 16 == k0 && 4 * 16 == k1 && 1 * 16 == k2)
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = (s[0] * 6 + (s[-cn] + s[cn]) * 4 + (s[-cn * 2] + s[cn * 2]) * 1) << 4;
                    int s1 = (s[1] * 6 + (s[1 - cn] + s[1 + cn]) * 4 + (s[1 - cn * 2] + s[1 + cn * 2]) * 1) << 4;
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
            else
                for (; i <= width - 2; i += 2, s += 2)
                {
                    int s0 = s[0] * k0 + (s[-cn] + s[cn]) * k1 + (s[-cn * 2] + s[cn * 2]) * k2;
                    int s1 = s[1] * k0 + (s[1 - cn] + s[1 + cn]) * k1 + (s[1 - cn * 2] + s[1 + cn * 2]) * k2;
                    dst[i] = s0;
                    dst[i + 1] = s1;
                }
        }
        else
            for (; i <= width - 4; i += 4, s += 4)
            {
                int f = kx[0];
                int s0 = f * s[0], s1 = f * s[1], s2 = f * s[2], s3 = f * s[3];
                for (k = 1, j = cn; k <= ksize2; k++, j += cn)
                {
                    f = kx[k];
                    s0 += f * (s[j] + s[-j]);
                    s1 += f * (s[j + 1] + s[-j + 1]);
                    s2 += f * (s[j + 2] + s[-j + 2]);
                    s3 += f * (s[j + 3] + s[-j + 3]);
                }

                dst[i] = s0;
                dst[i + 1] = s1;
                dst[i + 2] = s2;
                dst[i + 3] = s3;
            }

        for (; i < width; i++, s++)
        {
            int s0 = kx[0] * s[0];
            for (k = 1, j = cn; k <= ksize2; k++, j += cn)
                s0 += kx[k] * (s[j] + s[-j]);
            dst[i] = s0;
        }
    }
    else
    {
        if (3 == ksize && 0 == kx[0] && 1 == kx[1])
            for (; i <= width - 2; i += 2, s += 2)
            {
                int s0 = s[cn] - s[-cn], s1 = s[1 + cn] - s[1 - cn];
                dst[i] = s0;
                dst[i + 1] = s1;
            }
        else
            for (; i <= width - 4; i += 4, s += 4)
            {
                int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                for (k = 1, j = cn; k <= ksize2; k++, j += cn)
                {
                    int f = kx[k];
                    s0 += f * (s[j] - s[-j]);
                    s1 += f * (s[j + 1] - s[-j + 1]);
                    s2 += f * (s[j + 2] - s[-j + 2]);
                    s3 += f * (s[j + 3] - s[-j + 3]);
                }

                dst[i] = s0;
                dst[i + 1] = s1;
                dst[i + 2] = s2;
                dst[i + 3] = s3;
            }

        for (; i < width; i++, s++)
        {
            int s0 = kx[0] * s[0];
            for (k = 1, j = cn; k <= ksize2; k++, j += cn)
                s0 += kx[k] * (s[j] - s[-j]);
            dst[i] = s0;
        }
    }
}

static void
iFilterColSymm_32s8u(const int **src, uchar *dst, int dst_step, int count, void *params)
{
    const SepFilter *state = (const SepFilter *)params;
    const Mat *_ky = state->get_y_kernel();
    const int *ky = _ky->data.i;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize / 2;
    int i, k, width = state->get_width();
    int cn = VOS_MAT_CN(state->get_src_type());

    width *= cn;
    src += ksize2;
    ky += ksize2;

    for (; count--; dst += dst_step, src++)
    {
        if (ksize == 3)
        {
            const int *sptr0 = src[-1], *sptr1 = src[0], *sptr2 = src[1];
            int k0 = ky[0], k1 = ky[1];
            for (i = 0; i <= width - 2; i += 2)
            {
                int s0 = sptr1[i] * k0 + (sptr0[i] + sptr2[i]) * k1;
                int s1 = sptr1[i + 1] * k0 + (sptr0[i + 1] + sptr2[i + 1]) * k1;
                s0 = VOS_DESCALE(s0, FILTER_BITS * 2);
                s1 = VOS_DESCALE(s1, FILTER_BITS * 2);
                dst[i] = (uchar)s0;
                dst[i + 1] = (uchar)s1;
            }
        }
        else if (ksize == 5)
        {
            const int *sptr0 = src[-2], *sptr1 = src[-1],
                      *sptr2 = src[0], *sptr3 = src[1], *sptr4 = src[2];
            int k0 = ky[0], k1 = ky[1], k2 = ky[2];
            for (i = 0; i <= width - 2; i += 2)
            {
                int s0 = sptr2[i] * k0 + (sptr1[i] + sptr3[i]) * k1 + (sptr0[i] + sptr4[i]) * k2;
                int s1 = sptr2[i + 1] * k0 + (sptr1[i + 1] + sptr3[i + 1]) * k1 + (sptr0[i + 1] + sptr4[i + 1]) * k2;
                s0 = VOS_DESCALE(s0, FILTER_BITS * 2);
                s1 = VOS_DESCALE(s1, FILTER_BITS * 2);
                dst[i] = (uchar)s0;
                dst[i + 1] = (uchar)s1;
            }
        }
        else
            for (i = 0; i <= width - 4; i += 4)
            {
                int f = ky[0];
                const int *sptr = src[0] + i, *sptr2;
                int s0 = f * sptr[0], s1 = f * sptr[1], s2 = f * sptr[2], s3 = f * sptr[3];
                for (k = 1; k <= ksize2; k++)
                {
                    sptr = src[k] + i;
                    sptr2 = src[-k] + i;
                    f = ky[k];
                    s0 += f * (sptr[0] + sptr2[0]);
                    s1 += f * (sptr[1] + sptr2[1]);
                    s2 += f * (sptr[2] + sptr2[2]);
                    s3 += f * (sptr[3] + sptr2[3]);
                }

                s0 = VOS_DESCALE(s0, FILTER_BITS * 2);
                s1 = VOS_DESCALE(s1, FILTER_BITS * 2);
                dst[i] = (uchar)s0;
                dst[i + 1] = (uchar)s1;
                s2 = VOS_DESCALE(s2, FILTER_BITS * 2);
                s3 = VOS_DESCALE(s3, FILTER_BITS * 2);
                dst[i + 2] = (uchar)s2;
                dst[i + 3] = (uchar)s3;
            }

        for (; i < width; i++)
        {
            int s0 = ky[0] * src[0][i];
            for (k = 1; k <= ksize2; k++)
                s0 += ky[k] * (src[k][i] + src[-k][i]);

            s0 = VOS_DESCALE(s0, FILTER_BITS * 2);
            dst[i] = (uchar)s0;
        }
    }
}

static void
iFilterColSymm_32s16s(const int **src, short *dst,
                      int dst_step, int count, void *params)
{
    const SepFilter *state = (const SepFilter *)params;
    const Mat *_ky = state->get_y_kernel();
    const int *ky = (const int *)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize / 2;
    int i = 0, k, width = state->get_width();
    int cn = VOS_MAT_CN(state->get_src_type());
    int is_symm = state->get_y_kernel_flags() & SepFilter::SYMMETRICAL;
    int is_1_2_1 = is_symm && ksize == 3 && ky[1] == 2 && ky[2] == 1;
    int is_3_10_3 = is_symm && ksize == 3 && ky[1] == 10 && ky[2] == 3;
    int is_m1_0_1 = !is_symm && ksize == 3 && ky[1] == 0 &&
                            ky[2] * ky[2] == 1
                        ? (ky[2] > 0 ? 1 : -1)
                        : 0;

    width *= cn;
    src += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if (is_symm)
    {
        for (; count--; dst += dst_step, src++)
        {
            if (is_1_2_1)
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for (i = 0; i <= width - 2; i += 2)
                {
                    int s0 = src0[i] + src1[i] * 2 + src2[i],
                        s1 = src0[i + 1] + src1[i + 1] * 2 + src2[i + 1];

                    dst[i] = (short)s0;
                    dst[i + 1] = (short)s1;
                }
            }
            else if (is_3_10_3)
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for (i = 0; i <= width - 2; i += 2)
                {
                    int s0 = src1[i] * 10 + (src0[i] + src2[i]) * 3,
                        s1 = src1[i + 1] * 10 + (src0[i + 1] + src2[i + 1]) * 3;

                    dst[i] = (short)s0;
                    dst[i + 1] = (short)s1;
                }
            }
            else
                for (i = 0; i <= width - 4; i += 4)
                {
                    int f = ky[0];
                    const int *sptr = src[0] + i, *sptr2;
                    int s0 = f * sptr[0], s1 = f * sptr[1],
                        s2 = f * sptr[2], s3 = f * sptr[3];
                    for (k = 1; k <= ksize2; k++)
                    {
                        sptr = src[k] + i;
                        sptr2 = src[-k] + i;
                        f = ky[k];
                        s0 += f * (sptr[0] + sptr2[0]);
                        s1 += f * (sptr[1] + sptr2[1]);
                        s2 += f * (sptr[2] + sptr2[2]);
                        s3 += f * (sptr[3] + sptr2[3]);
                    }

                    dst[i] = VOS_CAST_16S(s0);
                    dst[i + 1] = VOS_CAST_16S(s1);
                    dst[i + 2] = VOS_CAST_16S(s2);
                    dst[i + 3] = VOS_CAST_16S(s3);
                }

            for (; i < width; i++)
            {
                int s0 = ky[0] * src[0][i];
                for (k = 1; k <= ksize2; k++)
                    s0 += ky[k] * (src[k][i] + src[-k][i]);
                dst[i] = VOS_CAST_16S(s0);
            }
        }
    }
    else
    {
        for (; count--; dst += dst_step, src++)
        {
            if (is_m1_0_1)
            {
                const int *src0 = src[-is_m1_0_1], *src2 = src[is_m1_0_1];

                for (i = 0; i <= width - 2; i += 2)
                {
                    int s0 = src2[i] - src0[i], s1 = src2[i + 1] - src0[i + 1];
                    dst[i] = (short)s0;
                    dst[i + 1] = (short)s1;
                }
            }
            else
                for (i = 0; i <= width - 4; i += 4)
                {
                    int f = ky[0];
                    const int *sptr = src[0] + i, *sptr2;
                    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                    for (k = 1; k <= ksize2; k++)
                    {
                        sptr = src[k] + i;
                        sptr2 = src[-k] + i;
                        f = ky[k];
                        s0 += f * (sptr[0] - sptr2[0]);
                        s1 += f * (sptr[1] - sptr2[1]);
                        s2 += f * (sptr[2] - sptr2[2]);
                        s3 += f * (sptr[3] - sptr2[3]);
                    }

                    dst[i] = VOS_CAST_16S(s0);
                    dst[i + 1] = VOS_CAST_16S(s1);
                    dst[i + 2] = VOS_CAST_16S(s2);
                    dst[i + 3] = VOS_CAST_16S(s3);
                }

            for (; i < width; i++)
            {
                int s0 = ky[0] * src[0][i];
                for (k = 1; k <= ksize2; k++)
                    s0 += ky[k] * (src[k][i] - src[-k][i]);
                dst[i] = VOS_CAST_16S(s0);
            }
        }
    }
}

#define VOS_SMALL_GAUSSIAN_SIZE 7

void SepFilter::init_gaussian_kernel(Mat *kernel, double sigma)
{
    static const float small_gaussian_tab[][VOS_SMALL_GAUSSIAN_SIZE / 2 + 1] =
        {
            {1.0f},
            {0.5f, 0.25f},
            {0.375f, 0.25f, 0.0625f},
            {0.28125f, 0.21875f, 0.109375f, 0.03125f}};

    VOS_FUNCNAME("SepFilter::init_gaussian_kernel");

    __BEGIN__;

    int type, i, n, step;
    const float *fixed_kernel = NULL;
    double sigmaX, scale2X, sum;
    float *cf;

    if (!VOS_IS_MAT(kernel))
        VOS_ERROR(VOS_StsBadArg, "kernel is not a valid matrix");

    type = VOS_MAT_TYPE(kernel->type);

    n = kernel->cols + kernel->rows - 1;

    if (n <= VOS_SMALL_GAUSSIAN_SIZE && sigma <= 0)
        fixed_kernel = small_gaussian_tab[n >> 1];

    sigmaX = sigma > 0 ? sigma : (n / 2 - 1) * 0.3 + 0.8;
    scale2X = -0.5 / (sigmaX * sigmaX);
    step = 1 == kernel->rows ? 1 : kernel->step / VOS_ELEM_SIZE1(type);
    cf = kernel->data.fl;

    sum = fixed_kernel ? -fixed_kernel[0] : -1.;

    for (i = 0; i <= n / 2; i++)
    {
        double t = fixed_kernel ? (double)fixed_kernel[i] : exp(scale2X * i * i);

        cf[(n / 2 + i) * step] = (float)t;
        sum += cf[(n / 2 + i) * step] * 2;
    }

    sum = 1. / sum;
    for (i = 0; i <= n / 2; i++)
    {
        cf[(n / 2 + i) * step] = cf[(n / 2 - i) * step] = (float)(cf[(n / 2 + i) * step] * sum);
    }

    __END__;
}

void SepFilter::init_sobel_kernel(Mat *_kx, Mat *_ky, int dx, int dy, int flags)
{
    VOS_FUNCNAME("SepFilter::init_sobel_kernel");

    __BEGIN__;

    int i, j, k, msz;
    int *kerI;
    bool normalize = (flags & NORMALIZE_KERNEL) != 0;
    bool flip = (flags & FLIP_KERNEL) != 0;

    if (!VOS_IS_MAT(_kx) || !VOS_IS_MAT(_ky))
        VOS_ERROR(VOS_StsBadArg, "One of the kernel matrices is not valid");

    msz = VOS_MAX(_kx->cols + _kx->rows, _ky->cols + _ky->rows);
    if (msz > 32)
        VOS_ERROR(VOS_StsOutOfRange, "Too large kernel size");

    kerI = (int *)sysStackAlloc(msz * sizeof(kerI[0]));

    if (dx < 0 || dy < 0 || dx + dy <= 0)
        VOS_ERROR(VOS_StsOutOfRange,
                  "Both derivative orders (dx and dy) must be non-negative "
                  "and at least one of them must be positive.");

    for (k = 0; k < 2; k++)
    {
        Mat *kernel = 0 == k ? _kx : _ky;
        int order = 0 == k ? dx : dy;
        int n = kernel->cols + kernel->rows - 1, step;
        int type = VOS_MAT_TYPE(kernel->type);
        double scale = !normalize ? 1. : 1. / (1 << (n - order - 1));
        int iscale = 1;

        if (n <= order)
            VOS_ERROR(VOS_StsOutOfRange,
                      "Derivative order must be smaller than the corresponding kernel size");

        if (1 == n)
            kerI[0] = 1;
        else if (3 == n)
        {
            if (0 == order)
                kerI[0] = 1, kerI[1] = 2, kerI[2] = 1;
            else if (1 == order)
                kerI[0] = -1, kerI[1] = 0, kerI[2] = 1;
            else
                kerI[0] = 1, kerI[1] = -2, kerI[2] = 1;
        }
        else
        {
            int oldval, newval;
            kerI[0] = 1;
            for (i = 0; i < n; i++)
                kerI[i + 1] = 0;

            for (i = 0; i < n - order - 1; i++)
            {
                oldval = kerI[0];
                for (j = 1; j <= n; j++)
                {
                    newval = kerI[j] + kerI[j - 1];
                    kerI[j - 1] = oldval;
                    oldval = newval;
                }
            }

            for (i = 0; i < order; i++)
            {
                oldval = -kerI[0];
                for (j = 1; j <= n; j++)
                {
                    newval = kerI[j - 1] - kerI[j];
                    kerI[j - 1] = oldval;
                    oldval = newval;
                }
            }
        }

        step = kernel->rows == 1 ? 1 : kernel->step / VOS_ELEM_SIZE1(type);
        if (flip && (order & 1) && k)
            iscale = -iscale, scale = -scale;

        for (i = 0; i < n; i++)
        {
            kernel->data.fl[i * step] = (float)(kerI[i] * scale);
        }
    }

    __END__;
}

void SepFilter::init_deriv(int _max_width, int _src_type, int _dst_type,
                           int dx, int dy, int aperture_size, int flags)
{
    VOS_FUNCNAME("SepFilter::init_deriv");

    __BEGIN__;

    int kx_size = aperture_size == VOS_SCHARR ? 3 : aperture_size, ky_size = kx_size;
    float kx_data[VOS_MAX_SOBEL_KSIZE], ky_data[VOS_MAX_SOBEL_KSIZE];
    Mat _kx, _ky;

    if (kx_size <= 0 || ky_size > VOS_MAX_SOBEL_KSIZE)
        VOS_ERROR(VOS_StsOutOfRange, "Incorrect aperture_size");

    if (1 == kx_size && dx)
        kx_size = 3;
    if (1 == ky_size && dy)
        ky_size = 3;

    _kx = InitMat(1, kx_size, VOS_32FC1, kx_data);
    _ky = InitMat(1, ky_size, VOS_32FC1, ky_data);

    VOS_CALL(init_sobel_kernel(&_kx, &_ky, dx, dy, flags));

    VOS_CALL(init(_max_width, _src_type, _dst_type, &_kx, &_ky));

    __END__;
}

#define VOS_MAX_GAUSSIAN_SIZE 1024
#define VOS_MIN_GAUSSIN_SIZE 0

void SepFilter::init_gaussian(int _max_width, int _src_type, int _dst_type,
                              int gaussian_size, double sigma)
{
    float *kdata = NULL;

    VOS_FUNCNAME("SepFilter::init_gaussian");

    __BEGIN__;

    Mat _kernel;

    if (gaussian_size <= VOS_MIN_GAUSSIN_SIZE || gaussian_size > VOS_MAX_GAUSSIAN_SIZE)
        VOS_ERROR(VOS_StsBadSize, "Incorrect size of gaussian kernel");

    kdata = (float *)sysStackAlloc(gaussian_size * sizeof(kdata[0]));
    _kernel = InitMat(1, gaussian_size, VOS_32F, kdata);

    VOS_CALL(init_gaussian_kernel(&_kernel, sigma));
    VOS_CALL(init(_max_width, _src_type, _dst_type, &_kernel, &_kernel));

    __END__;
}

/* End of file. */



#include "core.precomp.hpp"

/****************************************************************************************\
*                           [scaled] Identity matrix initialization                      *
\****************************************************************************************/

namespace cv
{

static inline void setSize(Mat &m, int _dims, const int *_sz,
                           const size_t *_steps, bool autoSteps = false)
{
    CV_Assert(0 <= _dims && _dims <= CV_MAX_DIM);
    if (m.dims != _dims)
    {
        if (m.step.p != m.step.buf)
        {
            fastFree(m.step.p);
            m.step.p = m.step.buf;
            m.size.p = &m.rows;
        }
        if (_dims > 2)
        {
            m.step.p = (size_t *)fastMalloc(_dims * sizeof(m.step.p[0]) + (_dims + 1) * sizeof(m.size.p[0]));
            m.size.p = (int *)(m.step.p + _dims) + 1;
            m.size.p[-1] = _dims;
            m.rows = m.cols = -1;
        }
    }

    m.dims = _dims;
    if (!_sz)
        return;

    size_t esz = CV_ELEM_SIZE(m.flags), total = esz;
    int i;
    for (i = _dims - 1; i >= 0; i--)
    {
        int s = _sz[i];
        CV_Assert(s >= 0);
        m.size.p[i] = s;

        if (_steps)
            m.step.p[i] = i < _dims - 1 ? _steps[i] : esz;
        else if (autoSteps)
        {
            m.step.p[i] = total;
            int64 total1 = (int64)total * s;
            if ((uint64)total1 != (size_t)total1)
                CV_Error(CV_StsOutOfRange, "The total matrix size does not fit to \"size_t\" type");
            total = (size_t)total1;
        }
    }

    if (_dims == 1)
    {
        m.dims = 2;
        m.cols = 1;
        m.step[1] = esz;
    }
}

static void updateContinuityFlag(Mat &m)
{
    int i, j;
    for (i = 0; i < m.dims; i++)
    {
        if (m.size[i] > 1)
            break;
    }

    for (j = m.dims - 1; j > i; j--)
    {
        if (m.step[j] * m.size[j] < m.step[j - 1])
            break;
    }

    uint64 t = (uint64)m.step[0] * m.size[0];
    if (j <= i && t == (size_t)t)
        m.flags |= Mat::CONTINUOUS_FLAG;
    else
        m.flags &= ~Mat::CONTINUOUS_FLAG;
}

static void finalizeHdr(Mat &m)
{
    updateContinuityFlag(m);
    int d = m.dims;
    if (d > 2)
        m.rows = m.cols = -1;
    if (m.data)
    {
        m.datalimit = m.datastart + m.size[0] * m.step[0];
        if (m.size[0] > 0)
        {
            m.dataend = m.data + m.size[d - 1] * m.step[d - 1];
            for (int i = 0; i < d - 1; i++)
                m.dataend += (m.size[i] - 1) * m.step[i];
        }
        else
            m.dataend = m.datalimit;
    }
    else
        m.dataend = m.datalimit = 0;
}

void Mat::create(int d, const int *_sizes, int _type)
{
    int i;
    CV_Assert(0 <= d && d <= CV_MAX_DIM && _sizes);
    _type = CV_MAT_TYPE(_type);

    if (data && (d == dims || (d == 1 && dims <= 2)) && _type == type())
    {
        if (d == 2 && rows == _sizes[0] && cols == _sizes[1])
            return;
        for (i = 0; i < d; i++)
            if (size[i] != _sizes[i])
                break;
        if (i == d && (d > 1 || size[1] == 1))
            return;
    }

    release();
    if (d == 0)
        return;
    flags = (_type & CV_MAT_TYPE_MASK) | MAGIC_VAL;
    setSize(*this, d, _sizes, 0, true);

    if (total() > 0)
    {
        if (!allocator)
        {
            size_t totalsize = alignSize(step.p[0] * size.p[0], (int)sizeof(*refcount));
            data = datastart = (uchar *)fastMalloc(totalsize + (int)sizeof(*refcount));
            refcount = (int *)(data + totalsize);
            *refcount = 1;
        }
        else
        {
            allocator->allocate(dims, size, _type, refcount, datastart, data, step.p);
            CV_Assert(step[dims - 1] == (size_t)CV_ELEM_SIZE(flags));
        }
    }

    finalizeHdr(*this);
}

void Mat::copySize(const Mat &m)
{
    setSize(*this, m.dims, 0, 0);
    for (int i = 0; i < dims; i++)
    {
        size[i] = m.size[i];
        step[i] = m.step[i];
    }
}

void Mat::deallocate()
{
    if (allocator)
        allocator->deallocate(refcount, datastart, data);
    else
    {
        CV_DbgAssert(refcount != 0);
        fastFree(datastart);
    }
}

Mat::Mat(const Mat &m, const Range &_rowRange, const Range &_colRange) : size(&rows)
{
    initEmpty();
    CV_Assert(m.dims >= 2);
    if (m.dims > 2)
    {
        AutoBuffer<Range> rs(m.dims);
        rs[0] = _rowRange;
        rs[1] = _colRange;
        for (int i = 2; i < m.dims; i++)
            rs[i] = Range::all();
        *this = m(rs);
        return;
    }

    *this = m;
    try
    {
        if (_rowRange != Range::all() && _rowRange != Range(0, rows))
        {
            CV_Assert(0 <= _rowRange.start && _rowRange.start <= _rowRange.end && _rowRange.end <= m.rows);
            rows = _rowRange.size();
            data += step * _rowRange.start;
            flags |= SUBMATRIX_FLAG;
        }

        if (_colRange != Range::all() && _colRange != Range(0, cols))
        {
            CV_Assert(0 <= _colRange.start && _colRange.start <= _colRange.end && _colRange.end <= m.cols);
            cols = _colRange.size();
            data += _colRange.start * elemSize();
            flags &= cols < m.cols ? ~CONTINUOUS_FLAG : -1;
            flags |= SUBMATRIX_FLAG;
        }
    }
    catch (...)
    {
        release();
        throw;
    }

    if (rows == 1)
        flags |= CONTINUOUS_FLAG;

    if (rows <= 0 || cols <= 0)
    {
        release();
        rows = cols = 0;
    }
}

Mat::Mat(const Mat &m, const Rect &roi)
    : flags(m.flags), dims(2), rows(roi.height), cols(roi.width),
      data(m.data + roi.y * m.step[0]), refcount(m.refcount),
      datastart(m.datastart), dataend(m.dataend), datalimit(m.datalimit),
      allocator(m.allocator), size(&rows)
{
    CV_Assert(m.dims <= 2);
    flags &= roi.width < m.cols ? ~CONTINUOUS_FLAG : -1;
    flags |= roi.height == 1 ? CONTINUOUS_FLAG : 0;

    size_t esz = CV_ELEM_SIZE(flags);
    data += roi.x * esz;
    CV_Assert(0 <= roi.x && 0 <= roi.width && roi.x + roi.width <= m.cols &&
              0 <= roi.y && 0 <= roi.height && roi.y + roi.height <= m.rows);
    if (refcount)
        CV_XADD(refcount, 1);
    if (roi.width < m.cols || roi.height < m.rows)
        flags |= SUBMATRIX_FLAG;

    step[0] = m.step[0];
    step[1] = esz;

    if (rows <= 0 || cols <= 0)
    {
        release();
        rows = cols = 0;
    }
}

Mat::Mat(const Mat &m, const Range *ranges) : size(&rows)
{
    initEmpty();
    int i, d = m.dims;

    CV_Assert(ranges);
    for (i = 0; i < d; i++)
    {
        Range r = ranges[i];
        CV_Assert(r == Range::all() || (0 <= r.start && r.start < r.end && r.end <= m.size[i]));
    }
    *this = m;
    for (i = 0; i < d; i++)
    {
        Range r = ranges[i];
        if (r != Range::all() && r != Range(0, size.p[i]))
        {
            size.p[i] = r.end - r.start;
            data += r.start * step.p[i];
            flags |= SUBMATRIX_FLAG;
        }
    }
    updateContinuityFlag(*this);
}

Mat::Mat(const CvMat *m, bool copyData) : size(&rows)
{
    initEmpty();

    if (!m)
        return;

    if (!copyData)
    {
        flags = MAGIC_VAL + (m->type & (CV_MAT_TYPE_MASK | CV_MAT_CONT_FLAG));
        dims = 2;
        rows = m->rows;
        cols = m->cols;
        data = datastart = m->data.ptr;
        size_t esz = CV_ELEM_SIZE(m->type), minstep = cols * esz, _step = m->step;
        if (_step == 0)
            _step = minstep;
        datalimit = datastart + _step * rows;
        dataend = datalimit - _step + minstep;
        step[0] = _step;
        step[1] = esz;
    }
    else
    {
        data = datastart = dataend = 0;
        Mat(m->rows, m->cols, m->type, m->data.ptr, m->step).copyTo(*this);
    }
}

Mat::Mat(const IplImage *img, bool copyData) : size(&rows)
{
    initEmpty();

    if (!img)
        return;

    dims = 2;
    CV_DbgAssert(CV_IS_IMAGE(img) && img->imageData != 0);

    int imgdepth = IPL2CV_DEPTH(img->depth);
    size_t esz;
    step[0] = img->widthStep;

    CV_Assert(img->dataOrder == IPL_DATA_ORDER_PIXEL);
    flags = MAGIC_VAL + CV_MAKETYPE(imgdepth, img->nChannels);
    rows = img->height;
    cols = img->width;
    datastart = data = (uchar *)img->imageData;
    esz = CV_ELEM_SIZE(flags);

    datalimit = datastart + step.p[0] * rows;
    dataend = datastart + step.p[0] * (rows - 1) + esz * cols;
    flags |= (cols * esz == step.p[0] || rows == 1 ? CONTINUOUS_FLAG : 0);
    step[1] = esz;

    if (copyData)
    {
        Mat m = *this;
        release();
        if (img->dataOrder == IPL_DATA_ORDER_PLANE)
            m.copyTo(*this);
    }
}

void Mat::reserve(size_t nelems)
{
    const size_t MIN_SIZE = 64;

    CV_Assert((int)nelems >= 0);
    if (!isSubmatrix() && data + step.p[0] * nelems <= datalimit)
        return;

    int r = size.p[0];

    if ((size_t)r >= nelems)
        return;

    size.p[0] = std::max((int)nelems, 1);
    size_t newsize = total() * elemSize();

    if (newsize < MIN_SIZE)
        size.p[0] = (int)((MIN_SIZE + newsize - 1) * nelems / newsize);

    Mat m(dims, size.p, type());
    size.p[0] = r;
    if (r > 0)
    {
        Mat mpart = m.rowRange(0, r);
        copyTo(mpart);
    }

    *this = m;
    size.p[0] = r;
    dataend = data + step.p[0] * r;
}

void Mat::resize(size_t nelems)
{
    int saveRows = size.p[0];
    if (saveRows == (int)nelems)
        return;
    CV_Assert((int)nelems >= 0);

    if (isSubmatrix() || data + step.p[0] * nelems > datalimit)
        reserve(nelems);

    size.p[0] = (int)nelems;
    dataend += (size.p[0] - saveRows) * step.p[0];

    //updateContinuityFlag(*this);
}

void Mat::resize(size_t nelems, const Scalar &s)
{
    int saveRows = size.p[0];
    resize(nelems);

    if (size.p[0] > saveRows)
    {
        Mat part = rowRange(saveRows, size.p[0]);
        part = s;
    }
}

Mat cvarrToMat(const CvArr *arr, bool copyData,
               bool /*allowND*/, int coiMode)
{
    if (!arr)
        return Mat();
    if (CV_IS_MAT(arr))
        return Mat((const CvMat *)arr, copyData);
    if (CV_IS_IMAGE(arr))
    {
        const IplImage *iplimg = (const IplImage *)arr;
        return Mat(iplimg, copyData);
    }
    CV_Error(CV_StsBadArg, "Unknown array type");
    return Mat();
}

void Mat::locateROI(Size &wholeSize, Point &ofs) const
{
    CV_Assert(dims <= 2 && step[0] > 0);
    size_t esz = elemSize(), minstep;
    ptrdiff_t delta1 = data - datastart, delta2 = dataend - datastart;

    if (delta1 == 0)
        ofs.x = ofs.y = 0;
    else
    {
        ofs.y = (int)(delta1 / step[0]);
        ofs.x = (int)((delta1 - step[0] * ofs.y) / esz);
        CV_DbgAssert(data == datastart + ofs.y * step[0] + ofs.x * esz);
    }
    minstep = (ofs.x + cols) * esz;
    wholeSize.height = (int)((delta2 - minstep) / step[0] + 1);
    wholeSize.height = std::max(wholeSize.height, ofs.y + rows);
    wholeSize.width = (int)((delta2 - step * (wholeSize.height - 1)) / esz);
    wholeSize.width = std::max(wholeSize.width, ofs.x + cols);
}

Mat &Mat::adjustROI(int dtop, int dbottom, int dleft, int dright)
{
    CV_Assert(dims <= 2 && step[0] > 0);
    Size wholeSize;
    Point ofs;
    size_t esz = elemSize();
    locateROI(wholeSize, ofs);
    int row1 = std::max(ofs.y - dtop, 0), row2 = std::min(ofs.y + rows + dbottom, wholeSize.height);
    int col1 = std::max(ofs.x - dleft, 0), col2 = std::min(ofs.x + cols + dright, wholeSize.width);
    data += (row1 - ofs.y) * step + (col1 - ofs.x) * esz;
    rows = row2 - row1;
    cols = col2 - col1;
    size.p[0] = rows;
    size.p[1] = cols;
    if (esz * cols == step[0] || rows == 1)
        flags |= CONTINUOUS_FLAG;
    else
        flags &= ~CONTINUOUS_FLAG;
    return *this;
}
}

namespace cv
{

void scalarToRawData(const Scalar &s, void *_buf, int type, int unroll_to)
{
    int i, depth = CV_MAT_DEPTH(type), cn = CV_MAT_CN(type);
    CV_Assert(cn <= 4);
    switch (depth)
    {
    case CV_8U:
    {
        uchar *buf = (uchar *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<uchar>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_8S:
    {
        schar *buf = (schar *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<schar>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_16U:
    {
        ushort *buf = (ushort *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<ushort>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_16S:
    {
        short *buf = (short *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<short>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_32S:
    {
        int *buf = (int *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<int>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_32F:
    {
        float *buf = (float *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<float>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
    }
    break;
    case CV_64F:
    {
        double *buf = (double *)_buf;
        for (i = 0; i < cn; i++)
            buf[i] = saturate_cast<double>(s.val[i]);
        for (; i < unroll_to; i++)
            buf[i] = buf[i - cn];
        break;
    }
    default:
        CV_Error(CV_StsUnsupportedFormat, "");
    }
}

/*************************************************************************************************\
                                        Input/Output Array
\*************************************************************************************************/

_InputArray::_InputArray() : flags(0), obj(0) {}
_InputArray::_InputArray(const Mat &m) : flags(MAT), obj((void *)&m) {}
_InputArray::_InputArray(const vector<Mat> &vec) : flags(STD_VECTOR_MAT), obj((void *)&vec) {}
_InputArray::_InputArray(const double &val) : flags(FIXED_TYPE + FIXED_SIZE + MATX + CV_64F), obj((void *)&val), sz(Size(1, 1)) {}

Mat _InputArray::getMat(int i) const
{
    int k = kind();

    if (k == MAT)
    {
        const Mat *m = (const Mat *)obj;
        if (i < 0)
            return *m;
        return m->row(i);
    }
    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
}

int _InputArray::kind() const
{
    return flags & KIND_MASK;
}

Size _InputArray::size(int i) const
{
    int k = kind();

    if (k == MAT)
    {
        CV_Assert(i < 0);
        return ((const Mat *)obj)->size();
    }
    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
}

size_t _InputArray::total(int i) const
{
    int k = kind();

    if (k == MAT)
    {
        CV_Assert(i < 0);
        return ((const Mat *)obj)->total();
    }
    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }

    return size(i).area();
}

int _InputArray::type(int i) const
{
    int k = kind();

    if (k == MAT)
        return ((const Mat *)obj)->type();

    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
}

int _InputArray::depth(int i) const
{
    return CV_MAT_DEPTH(type(i));
}

int _InputArray::channels(int i) const
{
    return CV_MAT_CN(type(i));
}

bool _InputArray::empty() const
{
    int k = kind();

    if (k == MAT)
        return ((const Mat *)obj)->empty();

    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
    return false;
}

_OutputArray::_OutputArray() {}
#ifdef OPENCV_CAN_BREAK_BINARY_COMPATIBILITY
_OutputArray::~_OutputArray()
{
}
#endif
_OutputArray::_OutputArray(Mat &m) : _InputArray(m)
{
}
_OutputArray::_OutputArray(vector<Mat> &vec) : _InputArray(vec) {}
_OutputArray::_OutputArray(const Mat &m) : _InputArray(m) { flags |= FIXED_SIZE | FIXED_TYPE; }
_OutputArray::_OutputArray(const vector<Mat> &vec) : _InputArray(vec) { flags |= FIXED_SIZE; }

bool _OutputArray::fixedSize() const
{
    return (flags & FIXED_SIZE) == FIXED_SIZE;
}

bool _OutputArray::fixedType() const
{
    return (flags & FIXED_TYPE) == FIXED_TYPE;
}

void _OutputArray::create(Size _sz, int mtype, int i, bool allowTransposed, int fixedDepthMask) const
{
    int k = kind();
    if (k == MAT && i < 0 && !allowTransposed && fixedDepthMask == 0)
    {
        CV_Assert(!fixedSize() || ((Mat *)obj)->size.operator()() == _sz);
        CV_Assert(!fixedType() || ((Mat *)obj)->type() == mtype);
        ((Mat *)obj)->create(_sz, mtype);
        return;
    }
    if (k == GPU_MAT && i < 0 && !allowTransposed && fixedDepthMask == 0)
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for oclMat yet");
    }
    if (k == OPENGL_BUFFER && i < 0 && !allowTransposed && fixedDepthMask == 0)
    {
        return;
    }
    int sizes[] = {_sz.height, _sz.width};
    create(2, sizes, mtype, i, allowTransposed, fixedDepthMask);
}

void _OutputArray::create(int rows, int cols, int mtype, int i, bool allowTransposed, int fixedDepthMask) const
{
    int k = kind();
    if (k == MAT && i < 0 && !allowTransposed && fixedDepthMask == 0)
    {
        CV_Assert(!fixedSize() || ((Mat *)obj)->size.operator()() == Size(cols, rows));
        CV_Assert(!fixedType() || ((Mat *)obj)->type() == mtype);
        ((Mat *)obj)->create(rows, cols, mtype);
        return;
    }

    if (k == OPENGL_BUFFER && i < 0 && !allowTransposed && fixedDepthMask == 0)
    {
        //  CV_Assert(!fixedSize() || ((ogl::Buffer*)obj)->size() == Size(cols, rows));
        //  CV_Assert(!fixedType() || ((ogl::Buffer*)obj)->type() == mtype);
        //        ((ogl::Buffer*)obj)->create(rows, cols, mtype);
        return;
    }
    int sizes[] = {rows, cols};
    create(2, sizes, mtype, i, allowTransposed, fixedDepthMask);
}

void _OutputArray::create(int dims, const int *sizes, int mtype, int i, bool allowTransposed, int fixedDepthMask) const
{
    int k = kind();
    mtype = CV_MAT_TYPE(mtype);

    if (k == MAT)
    {
        CV_Assert(i < 0);
        Mat &m = *(Mat *)obj;
        if (allowTransposed)
        {
            if (!m.isContinuous())
            {
                CV_Assert(!fixedType() && !fixedSize());
                m.release();
            }

            if (dims == 2 && m.dims == 2 && m.data &&
                m.type() == mtype && m.rows == sizes[1] && m.cols == sizes[0])
                return;
        }

        if (fixedType())
        {
            if (CV_MAT_CN(mtype) == m.channels() && ((1 << CV_MAT_TYPE(flags)) & fixedDepthMask) != 0)
                mtype = m.type();
            else
                CV_Assert(CV_MAT_TYPE(mtype) == m.type());
        }
        if (fixedSize())
        {
            CV_Assert(m.dims == dims);
            for (int j = 0; j < dims; ++j)
                CV_Assert(m.size[j] == sizes[j]);
        }
        m.create(dims, sizes, mtype);
        return;
    }
    else
    {
        CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
}

void _OutputArray::release() const
{
    CV_Assert(!fixedSize());

    int k = kind();

    if (k == MAT)
    {
        ((Mat *)obj)->release();
        return;
    }
    else
    {
         CV_Error(CV_StsNotImplemented, "This method is not implemented for other yet");
    }
}

void _OutputArray::clear() const
{
    int k = kind();

    if (k == MAT)
    {
        CV_Assert(!fixedSize());
        ((Mat *)obj)->resize(0);
        return;
    }

    release();
}

bool _OutputArray::needed() const
{
    return kind() != NONE;
}

Mat &_OutputArray::getMatRef(int i) const
{
    int k = kind();
    if (i < 0)
    {
        CV_Assert(k == MAT);
        return *(Mat *)obj;
    }
    else
    {
        CV_Assert(k == STD_VECTOR_MAT);
        vector<Mat> &v = *(vector<Mat> *)obj;
        CV_Assert(i < (int)v.size());
        return v[i];
    }
}

static _OutputArray _none;
OutputArray noArray() { return _none; }
}

////////////////////////////////////// transpose /////////////////////////////////////////

namespace cv
{

template <typename T>
static void
transpose_(const uchar *src, size_t sstep, uchar *dst, size_t dstep, Size sz)
{
    int i = 0, j, m = sz.width, n = sz.height;

    for (; i < m; i++)
    {
        T *d0 = (T *)(dst + dstep * i);
        j = 0;
        for (; j < n; j++)
        {
            const T *s0 = (const T *)(src + i * sizeof(T) + j * sstep);
            d0[j] = s0[0];
        }
    }
}

template <typename T>
static void
transposeI_(uchar *data, size_t step, int n)
{
    int i, j;
    for (i = 0; i < n; i++)
    {
        T *row = (T *)(data + step * i);
        uchar *data1 = data + i * sizeof(T);
        for (j = i + 1; j < n; j++)
            std::swap(row[j], *(T *)(data1 + step * j));
    }
}

typedef void (*TransposeFunc)(const uchar *src, size_t sstep, uchar *dst, size_t dstep, Size sz);
typedef void (*TransposeInplaceFunc)(uchar *data, size_t step, int n);

#define DEF_TRANSPOSE_FUNC(suffix, type)          \
    \
static void transpose_##suffix(const uchar *src, size_t sstep, uchar *dst, size_t dstep, Size sz) \
{ transpose_<type>(src, sstep, dst, dstep, sz); } \
    \
\
static void transposeI_##suffix(uchar *data, size_t step, int n) \
{ transposeI_<type>(data, step, n); }

DEF_TRANSPOSE_FUNC(8u, uchar)
DEF_TRANSPOSE_FUNC(16u, ushort)
DEF_TRANSPOSE_FUNC(8uC3, Vec3b)
DEF_TRANSPOSE_FUNC(32s, int)
DEF_TRANSPOSE_FUNC(16uC3, Vec3s)
DEF_TRANSPOSE_FUNC(32sC2, Vec2i)
DEF_TRANSPOSE_FUNC(32sC3, Vec3i)
DEF_TRANSPOSE_FUNC(32sC4, Vec4i)
DEF_TRANSPOSE_FUNC(32sC6, Vec6i)
DEF_TRANSPOSE_FUNC(32sC8, Vec8i)

static TransposeFunc transposeTab[] =
    {
        0, transpose_8u, transpose_16u, transpose_8uC3, transpose_32s, 0, transpose_16uC3, 0,
        transpose_32sC2, 0, 0, 0, transpose_32sC3, 0, 0, 0, transpose_32sC4,
        0, 0, 0, 0, 0, 0, 0, transpose_32sC6, 0, 0, 0, 0, 0, 0, 0, transpose_32sC8};

static TransposeInplaceFunc transposeInplaceTab[] =
    {
        0, transposeI_8u, transposeI_16u, transposeI_8uC3, transposeI_32s, 0, transposeI_16uC3, 0,
        transposeI_32sC2, 0, 0, 0, transposeI_32sC3, 0, 0, 0, transposeI_32sC4,
        0, 0, 0, 0, 0, 0, 0, transposeI_32sC6, 0, 0, 0, 0, 0, 0, 0, transposeI_32sC8};
}

void cv::transpose(InputArray _src, OutputArray _dst)
{
    Mat src = _src.getMat();
    size_t esz = src.elemSize();
    CV_Assert(src.dims <= 2 && esz <= (size_t)32);

    _dst.create(src.cols, src.rows, src.type());
    Mat dst = _dst.getMat();

    // handle the case of single-column/single-row matrices, stored in STL vectors.
    if (src.rows != dst.cols || src.cols != dst.rows)
    {
        CV_Assert(src.size() == dst.size() && (src.cols == 1 || src.rows == 1));
        src.copyTo(dst);
        return;
    }

    if (dst.data == src.data)
    {
        TransposeInplaceFunc func = transposeInplaceTab[esz];
        CV_Assert(func != 0);
        func(dst.data, dst.step, dst.rows);
    }
    else
    {
        TransposeFunc func = transposeTab[esz];
        CV_Assert(func != 0);
        func(src.data, src.step, dst.data, dst.step, src.size());
    }
}

///////////////////////////// n-dimensional matrices ////////////////////////////

namespace cv
{

NAryMatIterator::NAryMatIterator()
    : arrays(0), planes(0), ptrs(0), narrays(0), nplanes(0), size(0), iterdepth(0), idx(0)
{
    int i = 0;
}

NAryMatIterator::NAryMatIterator(const Mat **_arrays, Mat *_planes, int _narrays)
    : arrays(0), planes(0), ptrs(0), narrays(0), nplanes(0), size(0), iterdepth(0), idx(0)
{
    init(_arrays, _planes, 0, _narrays);
}

NAryMatIterator::NAryMatIterator(const Mat **_arrays, uchar **_ptrs, int _narrays)
    : arrays(0), planes(0), ptrs(0), narrays(0), nplanes(0), size(0), iterdepth(0), idx(0)
{
    init(_arrays, 0, _ptrs, _narrays);
}

void NAryMatIterator::init(const Mat **_arrays, Mat *_planes, uchar **_ptrs, int _narrays)
{
    CV_Assert(_arrays && (_ptrs || _planes));
    int i, j, d1 = 0, i0 = -1, d = -1;

    arrays = _arrays;
    ptrs = _ptrs;
    planes = _planes;
    narrays = _narrays;
    nplanes = 0;
    size = 0;

    if (narrays < 0)
    {
        for (i = 0; _arrays[i] != 0; i++)
            ;
        narrays = i;
        CV_Assert(narrays <= 1000);
    }

    iterdepth = 0;

    for (i = 0; i < narrays; i++)
    {
        CV_Assert(arrays[i] != 0);
        const Mat &A = *arrays[i];
        if (ptrs)
            ptrs[i] = A.data;

        if (!A.data)
            continue;

        if (i0 < 0)
        {
            i0 = i;
            d = A.dims;

            // find the first dimensionality which is different from 1;
            // in any of the arrays the first "d1" step do not affect the continuity
            for (d1 = 0; d1 < d; d1++)
                if (A.size[d1] > 1)
                    break;
        }
        else
            CV_Assert(A.size == arrays[i0]->size);

        if (!A.isContinuous())
        {
            CV_Assert(A.step[d - 1] == A.elemSize());
            for (j = d - 1; j > d1; j--)
                if (A.step[j] * A.size[j] < A.step[j - 1])
                    break;
            iterdepth = std::max(iterdepth, j);
        }
    }

    if (i0 >= 0)
    {
        size = arrays[i0]->size[d - 1];
        for (j = d - 1; j > iterdepth; j--)
        {
            int64 total1 = (int64)size * arrays[i0]->size[j - 1];
            if (total1 != (int)total1)
                break;
            size = (int)total1;
        }

        iterdepth = j;
        if (iterdepth == d1)
            iterdepth = 0;

        nplanes = 1;
        for (j = iterdepth - 1; j >= 0; j--)
            nplanes *= arrays[i0]->size[j];
    }
    else
        iterdepth = 0;

    idx = 0;

    if (!planes)
        return;

    for (i = 0; i < narrays; i++)
    {
        CV_Assert(arrays[i] != 0);
        const Mat &A = *arrays[i];

        if (!A.data)
        {
            planes[i] = Mat();
            continue;
        }

        planes[i] = Mat(1, (int)size, A.type(), A.data);
    }
}

NAryMatIterator &NAryMatIterator::operator++()
{
    if (idx >= nplanes - 1)
        return *this;
    ++idx;

    if (iterdepth == 1)
    {
        if (ptrs)
        {
            for (int i = 0; i < narrays; i++)
            {
                if (!ptrs[i])
                    continue;
                ptrs[i] = arrays[i]->data + arrays[i]->step[0] * idx;
            }
        }
        if (planes)
        {
            for (int i = 0; i < narrays; i++)
            {
                if (!planes[i].data)
                    continue;
                planes[i].data = arrays[i]->data + arrays[i]->step[0] * idx;
            }
        }
    }
    else
    {
        for (int i = 0; i < narrays; i++)
        {
            const Mat &A = *arrays[i];
            if (!A.data)
                continue;
            int _idx = (int)idx;
            uchar *data = A.data;
            for (int j = iterdepth - 1; j >= 0 && _idx > 0; j--)
            {
                int szi = A.size[j], t = _idx / szi;
                data += (_idx - t * szi) * A.step[j];
                _idx = t;
            }
            if (ptrs)
                ptrs[i] = data;
            if (planes)
                planes[i].data = data;
        }
    }

    return *this;
}

NAryMatIterator NAryMatIterator::operator++(int)
{
    NAryMatIterator it = *this;
    ++*this;
    return it;
}
}

/* End of file. */

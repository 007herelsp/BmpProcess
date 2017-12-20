#ifndef __OPENCV_CORE_MATRIX_OPERATIONS_HPP__
#define __OPENCV_CORE_MATRIX_OPERATIONS_HPP__

#ifdef __cplusplus

namespace cv
{

//////////////////////////////// Mat ////////////////////////////////

inline void Mat::initEmpty()
{
    flags = MAGIC_VAL;
    dims = rows = cols = 0;
    data = datastart = dataend = datalimit = 0;
    refcount = 0;
    allocator = 0;
}

inline Mat::Mat() : size(&rows)
{
    initEmpty();
}

inline Mat::Mat(int _rows, int _cols, int _type) : size(&rows)
{
    initEmpty();
    create(_rows, _cols, _type);
}

inline Mat::Mat(Size _sz, int _type) : size(&rows)
{
    initEmpty();
    create(_sz.height, _sz.width, _type);
}

inline Mat::Mat(Size _sz, int _type, const Scalar &_s) : size(&rows)
{
    initEmpty();
    create(_sz.height, _sz.width, _type);
    *this = _s;
}

inline Mat::Mat(int _dims, const int *_sz, int _type) : size(&rows)
{
    initEmpty();
    create(_dims, _sz, _type);
}

inline Mat::Mat(int _dims, const int *_sz, int _type, const Scalar &_s) : size(&rows)
{
    initEmpty();
    create(_dims, _sz, _type);
    *this = _s;
}

inline Mat::Mat(const Mat &m)
    : flags(m.flags), dims(m.dims), rows(m.rows), cols(m.cols), data(m.data),
      refcount(m.refcount), datastart(m.datastart), dataend(m.dataend),
      datalimit(m.datalimit), allocator(m.allocator), size(&rows)
{
    if (refcount)
        CV_XADD(refcount, 1);
    if (m.dims <= 2)
    {
        step[0] = m.step[0];
        step[1] = m.step[1];
    }
    else
    {
        dims = 0;
        copySize(m);
    }
}

inline Mat::Mat(int _rows, int _cols, int _type, void *_data, size_t _step)
    : flags(MAGIC_VAL + (_type & TYPE_MASK)), dims(2), rows(_rows), cols(_cols),
      data((uchar *)_data), refcount(0), datastart((uchar *)_data), dataend(0),
      datalimit(0), allocator(0), size(&rows)
{
    size_t esz = CV_ELEM_SIZE(_type), minstep = cols * esz;
    if (_step == AUTO_STEP)
    {
        _step = minstep;
        flags |= CONTINUOUS_FLAG;
    }
    else
    {
        if (rows == 1)
            _step = minstep;
        CV_DbgAssert(_step >= minstep);
        flags |= _step == minstep ? CONTINUOUS_FLAG : 0;
    }
    step[0] = _step;
    step[1] = esz;
    datalimit = datastart + _step * rows;
    dataend = datalimit - _step + minstep;
}

inline Mat::Mat(Size _sz, int _type, void *_data, size_t _step)
    : flags(MAGIC_VAL + (_type & TYPE_MASK)), dims(2), rows(_sz.height), cols(_sz.width),
      data((uchar *)_data), refcount(0), datastart((uchar *)_data), dataend(0),
      datalimit(0), allocator(0), size(&rows)
{
    size_t esz = CV_ELEM_SIZE(_type), minstep = cols * esz;
    if (_step == AUTO_STEP)
    {
        _step = minstep;
        flags |= CONTINUOUS_FLAG;
    }
    else
    {
        if (rows == 1)
            _step = minstep;
        CV_DbgAssert(_step >= minstep);
        flags |= _step == minstep ? CONTINUOUS_FLAG : 0;
    }
    step[0] = _step;
    step[1] = esz;
    datalimit = datastart + _step * rows;
    dataend = datalimit - _step + minstep;
}

inline Mat::~Mat()
{
    release();
    if (step.p != step.buf)
        fastFree(step.p);
}

inline Mat &Mat::operator=(const Mat &m)
{
    if (this != &m)
    {
        if (m.refcount)
            CV_XADD(m.refcount, 1);
        release();
        flags = m.flags;
        if (dims <= 2 && m.dims <= 2)
        {
            dims = m.dims;
            rows = m.rows;
            cols = m.cols;
            step[0] = m.step[0];
            step[1] = m.step[1];
        }
        else
            copySize(m);
        data = m.data;
        datastart = m.datastart;
        dataend = m.dataend;
        datalimit = m.datalimit;
        refcount = m.refcount;
        allocator = m.allocator;
    }
    return *this;
}

inline Mat Mat::row(int y) const
{
    return Mat(*this, Range(y, y + 1), Range::all());
}
inline Mat Mat::col(int x) const
{
    return Mat(*this, Range::all(), Range(x, x + 1));
}
inline Mat Mat::rowRange(int startrow, int endrow) const
{
    return Mat(*this, Range(startrow, endrow), Range::all());
}
inline Mat Mat::rowRange(const Range &r) const
{
    return Mat(*this, r, Range::all());
}
inline Mat Mat::colRange(int startcol, int endcol) const
{
    return Mat(*this, Range::all(), Range(startcol, endcol));
}
inline Mat Mat::colRange(const Range &r) const
{
    return Mat(*this, Range::all(), r);
}



inline Mat Mat::clone() const
{
    Mat m;
    copyTo(m);
    return m;
}

inline void Mat::assignTo(Mat &m, int _type) const
{
    if (_type < 0)
        m = *this;
    else
        convertTo(m, _type);
}

inline void Mat::create(int _rows, int _cols, int _type)
{
    _type &= TYPE_MASK;
    if (dims <= 2 && rows == _rows && cols == _cols && type() == _type && data)
        return;
    int sz[] = {_rows, _cols};
    create(2, sz, _type);
}

inline void Mat::create(Size _sz, int _type)
{
    create(_sz.height, _sz.width, _type);
}

inline void Mat::addref()
{
    if (refcount)
        CV_XADD(refcount, 1);
}

inline void Mat::release()
{
    if (refcount && CV_XADD(refcount, -1) == 1)
        deallocate();
    data = datastart = dataend = datalimit = 0;
    for (int i = 0; i < dims; i++)
        size.p[i] = 0;
#ifdef _DEBUG
    flags = MAGIC_VAL;
    dims = rows = cols = 0;
    if (step.p != step.buf)
    {
        fastFree(step.p);
        step.p = step.buf;
        size.p = &rows;
    }
#endif
    refcount = 0;
}

inline Mat Mat::operator()(Range _rowRange, Range _colRange) const
{
    return Mat(*this, _rowRange, _colRange);
}

inline Mat Mat::operator()(const Rect &roi) const
{
    return Mat(*this, roi);
}

inline Mat Mat::operator()(const Range *ranges) const
{
    return Mat(*this, ranges);
}

inline Mat::operator CvMat() const
{
    CV_DbgAssert(dims <= 2);
    CvMat m = cvMat(rows, dims == 1 ? 1 : cols, type(), data);
    m.step = (int)step[0];
    m.type = (m.type & ~CONTINUOUS_FLAG) | (flags & CONTINUOUS_FLAG);
    return m;
}

inline bool Mat::isContinuous() const
{
    return (flags & CONTINUOUS_FLAG) != 0;
}
inline bool Mat::isSubmatrix() const
{
    return (flags & SUBMATRIX_FLAG) != 0;
}
inline size_t Mat::elemSize() const
{
    return dims > 0 ? step.p[dims - 1] : 0;
}
inline size_t Mat::elemSize1() const
{
    return CV_ELEM_SIZE1(flags);
}
inline int Mat::type() const
{
    return CV_MAT_TYPE(flags);
}
inline int Mat::depth() const
{
    return CV_MAT_DEPTH(flags);
}
inline int Mat::channels() const
{
    return CV_MAT_CN(flags);
}
inline size_t Mat::step1(int i) const
{
    return step.p[i] / elemSize1();
}
inline bool Mat::empty() const
{
    return data == 0 || total() == 0;
}
inline size_t Mat::total() const
{
    if (dims <= 2)
        return (size_t)rows * cols;
    size_t p = 1;
    for (int i = 0; i < dims; i++)
        p *= size[i];
    return p;
}

inline uchar *Mat::ptr(int y)
{
    CV_DbgAssert(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)size.p[0]));
    return data + step.p[0] * y;
}

inline const uchar *Mat::ptr(int y) const
{
    CV_DbgAssert(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)size.p[0]));
    return data + step.p[0] * y;
}

template <typename _Tp>
inline _Tp *Mat::ptr(int y)
{
    CV_DbgAssert(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)size.p[0]));
    return (_Tp *)(data + step.p[0] * y);
}

template <typename _Tp>
inline const _Tp *Mat::ptr(int y) const
{
    CV_DbgAssert(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)size.p[0]));
    return (const _Tp *)(data + step.p[0] * y);
}

inline uchar *Mat::ptr(const int *idx)
{
    int i, d = dims;
    uchar *p = data;
    CV_DbgAssert(d >= 1 && p);
    for (i = 0; i < d; i++)
    {
        CV_DbgAssert((unsigned)idx[i] < (unsigned)size.p[i]);
        p += idx[i] * step.p[i];
    }
    return p;
}

inline const uchar *Mat::ptr(const int *idx) const
{
    int i, d = dims;
    uchar *p = data;
    CV_DbgAssert(d >= 1 && p);
    for (i = 0; i < d; i++)
    {
        CV_DbgAssert((unsigned)idx[i] < (unsigned)size.p[i]);
        p += idx[i] * step.p[i];
    }
    return p;
}

template <typename _Tp>
inline _Tp &Mat::at(int i0, int i1)
{
    CV_DbgAssert(dims <= 2 && data && (unsigned)i0 < (unsigned)size.p[0] &&
                 (unsigned)(i1 * DataType<_Tp>::channels) < (unsigned)(size.p[1] * channels()) &&
                 CV_ELEM_SIZE1(DataType<_Tp>::depth) == elemSize1());
    return ((_Tp *)(data + step.p[0] * i0))[i1];
}

template <typename _Tp>
inline const _Tp &Mat::at(int i0, int i1) const
{
    CV_DbgAssert(dims <= 2 && data && (unsigned)i0 < (unsigned)size.p[0] &&
                 (unsigned)(i1 * DataType<_Tp>::channels) < (unsigned)(size.p[1] * channels()) &&
                 CV_ELEM_SIZE1(DataType<_Tp>::depth) == elemSize1());
    return ((const _Tp *)(data + step.p[0] * i0))[i1];
}

template <typename _Tp>
inline _Tp &Mat::at(Point pt)
{
    CV_DbgAssert(dims <= 2 && data && (unsigned)pt.y < (unsigned)size.p[0] &&
                 (unsigned)(pt.x * DataType<_Tp>::channels) < (unsigned)(size.p[1] * channels()) &&
                 CV_ELEM_SIZE1(DataType<_Tp>::depth) == elemSize1());
    return ((_Tp *)(data + step.p[0] * pt.y))[pt.x];
}

template <typename _Tp>
inline const _Tp &Mat::at(Point pt) const
{
    CV_DbgAssert(dims <= 2 && data && (unsigned)pt.y < (unsigned)size.p[0] &&
                 (unsigned)(pt.x * DataType<_Tp>::channels) < (unsigned)(size.p[1] * channels()) &&
                 CV_ELEM_SIZE1(DataType<_Tp>::depth) == elemSize1());
    return ((const _Tp *)(data + step.p[0] * pt.y))[pt.x];
}

template <typename _Tp>
inline _Tp &Mat::at(int i0)
{
    CV_DbgAssert(dims <= 2 && data &&
                 (unsigned)i0 < (unsigned)(size.p[0] * size.p[1]) &&
                 elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    if (isContinuous() || size.p[0] == 1)
        return ((_Tp *)data)[i0];
    if (size.p[1] == 1)
        return *(_Tp *)(data + step.p[0] * i0);
    int i = i0 / cols, j = i0 - i * cols;
    return ((_Tp *)(data + step.p[0] * i))[j];
}

template <typename _Tp>
inline const _Tp &Mat::at(int i0) const
{
    CV_DbgAssert(dims <= 2 && data &&
                 (unsigned)i0 < (unsigned)(size.p[0] * size.p[1]) &&
                 elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    if (isContinuous() || size.p[0] == 1)
        return ((const _Tp *)data)[i0];
    if (size.p[1] == 1)
        return *(const _Tp *)(data + step.p[0] * i0);
    int i = i0 / cols, j = i0 - i * cols;
    return ((const _Tp *)(data + step.p[0] * i))[j];
}

template <typename _Tp>
inline _Tp &Mat::at(int i0, int i1, int i2)
{
    CV_DbgAssert(elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    return *(_Tp *)ptr(i0, i1, i2);
}
template <typename _Tp>
inline const _Tp &Mat::at(int i0, int i1, int i2) const
{
    CV_DbgAssert(elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    return *(const _Tp *)ptr(i0, i1, i2);
}
template <typename _Tp>
inline _Tp &Mat::at(const int *idx)
{
    CV_DbgAssert(elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    return *(_Tp *)ptr(idx);
}
template <typename _Tp>
inline const _Tp &Mat::at(const int *idx) const
{
    CV_DbgAssert(elemSize() == CV_ELEM_SIZE(DataType<_Tp>::type));
    return *(const _Tp *)ptr(idx);
}


inline Mat::MSize::MSize(int *_p) : p(_p) {}
inline Size Mat::MSize::operator()() const
{
    CV_DbgAssert(p[-1] <= 2);
    return Size(p[1], p[0]);
}
inline const int &Mat::MSize::operator[](int i) const { return p[i]; }
inline int &Mat::MSize::operator[](int i) { return p[i]; }
inline Mat::MSize::operator const int *() const { return p; }

inline bool Mat::MSize::operator==(const MSize &sz) const
{
    int d = p[-1], dsz = sz.p[-1];
    if (d != dsz)
        return false;
    if (d == 2)
        return p[0] == sz.p[0] && p[1] == sz.p[1];

    for (int i = 0; i < d; i++)
        if (p[i] != sz.p[i])
            return false;
    return true;
}

inline bool Mat::MSize::operator!=(const MSize &sz) const
{
    return !(*this == sz);
}

inline Mat::MStep::MStep()
{
    p = buf;
    p[0] = p[1] = 0;
}
inline Mat::MStep::MStep(size_t s)
{
    p = buf;
    p[0] = s;
    p[1] = 0;
}
inline const size_t &Mat::MStep::operator[](int i) const { return p[i]; }
inline size_t &Mat::MStep::operator[](int i) { return p[i]; }
inline Mat::MStep::operator size_t() const
{
    CV_DbgAssert(p == buf);
    return buf[0];
}
inline Mat::MStep &Mat::MStep::operator=(size_t s)
{
    CV_DbgAssert(p == buf);
    buf[0] = s;
    return *this;
}


////////////////////////////// Augmenting algebraic operations //////////////////////////////////


static inline Mat &operator*=(const Mat &a, double s)
{
    a.convertTo((Mat &)a, -1, s);
    return (Mat &)a;
}
}

#endif
#endif

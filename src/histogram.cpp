
#include "imgproc.precomp.hpp"

namespace cv
{

template <>
void Ptr<CvHistogram>::delete_obj()
{
    cvReleaseHist(&obj);
}

////////////////// Helper functions //////////////////////

static const size_t OUT_OF_RANGE = (size_t)1 << (sizeof(size_t) * 8 - 2);

static void
calcHistLookupTables_8u(const Mat &hist, const SparseMat &shist,
                        int dims, const float **ranges, const double *uniranges,
                        bool uniform, bool issparse, vector<size_t> &_tab)
{
    const int low = 0, high = 256;
    int i, j;
    _tab.resize((high - low) * dims);
    size_t *tab = &_tab[0];

    if (uniform)
    {
        for (i = 0; i < dims; i++)
        {
            double a = uniranges[i * 2];
            double b = uniranges[i * 2 + 1];
            int sz = !issparse ? hist.size[i] : shist.size(i);
            size_t step = !issparse ? hist.step[i] : 1;

            for (j = low; j < high; j++)
            {
                int idx = cvFloor(j * a + b);
                size_t written_idx;
                if ((unsigned)idx < (unsigned)sz)
                    written_idx = idx * step;
                else
                    written_idx = OUT_OF_RANGE;

                tab[i * (high - low) + j - low] = written_idx;
            }
        }
    }
    else
    {
        for (i = 0; i < dims; i++)
        {
            int limit = std::min(cvCeil(ranges[i][0]), high);
            int idx = -1, sz = !issparse ? hist.size[i] : shist.size(i);
            size_t written_idx = OUT_OF_RANGE;
            size_t step = !issparse ? hist.step[i] : 1;

            for (j = low;;)
            {
                for (; j < limit; j++)
                    tab[i * (high - low) + j - low] = written_idx;

                if ((unsigned)(++idx) < (unsigned)sz)
                {
                    limit = std::min(cvCeil(ranges[i][idx + 1]), high);
                    written_idx = idx * step;
                }
                else
                {
                    for (; j < high; j++)
                        tab[i * (high - low) + j - low] = OUT_OF_RANGE;
                    break;
                }
            }
        }
    }
}

static void histPrepareImages(const Mat *images, int nimages, const int *channels,
                              const Mat &mask, int dims, const int *histSize,
                              const float **ranges, bool uniform,
                              vector<uchar *> &ptrs, vector<int> &deltas,
                              Size &imsize, vector<double> &uniranges)
{
    int i, j, c;
    CV_Assert(channels != 0 || nimages == dims);

    imsize = images[0].size();
    int depth = images[0].depth(), esz1 = (int)images[0].elemSize1();
    bool isContinuous = true;

    ptrs.resize(dims + 1);
    deltas.resize((dims + 1) * 2);

    for (i = 0; i < dims; i++)
    {
        if (!channels)
        {
            j = i;
            c = 0;
            CV_Assert(images[j].channels() == 1);
        }
        else
        {
            c = channels[i];
            CV_Assert(c >= 0);
            for (j = 0; j < nimages; c -= images[j].channels(), j++)
                if (c < images[j].channels())
                    break;
            CV_Assert(j < nimages);
        }

        CV_Assert(images[j].size() == imsize && images[j].depth() == depth);
        if (!images[j].isContinuous())
            isContinuous = false;
        ptrs[i] = images[j].data + c * esz1;
        deltas[i * 2] = images[j].channels();
        deltas[i * 2 + 1] = (int)(images[j].step / esz1 - imsize.width * deltas[i * 2]);
    }

    if (mask.data)
    {
        CV_Assert(mask.size() == imsize && mask.channels() == 1);
        isContinuous = isContinuous && mask.isContinuous();
        ptrs[dims] = mask.data;
        deltas[dims * 2] = 1;
        deltas[dims * 2 + 1] = (int)(mask.step / mask.elemSize1());
    }

#ifndef HAVE_TBB
    if (isContinuous)
    {
        imsize.width *= imsize.height;
        imsize.height = 1;
    }
#endif

    if (!ranges)
    {
        CV_Assert(depth == CV_8U);

        uniranges.resize(dims * 2);
        for (i = 0; i < dims; i++)
        {
            uniranges[i * 2] = histSize[i] / 256.;
            uniranges[i * 2 + 1] = 0;
        }
    }
    else if (uniform)
    {
        uniranges.resize(dims * 2);
        for (i = 0; i < dims; i++)
        {
            CV_Assert(ranges[i] && ranges[i][0] < ranges[i][1]);
            double low = ranges[i][0], high = ranges[i][1];
            double t = histSize[i] / (high - low);
            uniranges[i * 2] = t;
            uniranges[i * 2 + 1] = -t * low;
        }
    }
    else
    {
        for (i = 0; i < dims; i++)
        {
            size_t n = histSize[i];
            for (size_t k = 0; k < n; k++)
                CV_Assert(ranges[i][k] < ranges[i][k + 1]);
        }
    }
}

////////////////////////////////// C A L C U L A T E    H I S T O G R A M ////////////////////////////////////
#ifdef HAVE_TBB
enum
{
    one = 1,
    two,
    three
}; // array elements number

template <typename T>
class calcHist1D_Invoker
{
  public:
    calcHist1D_Invoker(const vector<uchar *> &_ptrs, const vector<int> &_deltas,
                       Mat &hist, const double *_uniranges, int sz, int dims,
                       Size &imageSize)
        : mask_(_ptrs[dims]),
          mstep_(_deltas[dims * 2 + 1]),
          imageWidth_(imageSize.width),
          histogramSize_(hist.size()), histogramType_(hist.type()),
          globalHistogram_((tbb::atomic<int> *)hist.data)
    {
        p_[0] = ((T **)&_ptrs[0])[0];
        step_[0] = (&_deltas[0])[1];
        d_[0] = (&_deltas[0])[0];
        a_[0] = (&_uniranges[0])[0];
        b_[0] = (&_uniranges[0])[1];
        size_[0] = sz;
    }

    void operator()(const BlockedRange &range) const
    {
        T *p0 = p_[0] + range.begin() * (step_[0] + imageWidth_ * d_[0]);
        uchar *mask = mask_ + range.begin() * mstep_;

        for (int row = range.begin(); row < range.end(); row++, p0 += step_[0])
        {
            if (!mask_)
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0])
                {
                    int idx = cvFloor(*p0 * a_[0] + b_[0]);
                    if ((unsigned)idx < (unsigned)size_[0])
                    {
                        globalHistogram_[idx].fetch_and_add(1);
                    }
                }
            }
            else
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0])
                {
                    if (mask[x])
                    {
                        int idx = cvFloor(*p0 * a_[0] + b_[0]);
                        if ((unsigned)idx < (unsigned)size_[0])
                        {
                            globalHistogram_[idx].fetch_and_add(1);
                        }
                    }
                }
                mask += mstep_;
            }
        }
    }

  private:
    calcHist1D_Invoker operator=(const calcHist1D_Invoker &);

    T *p_[one];
    uchar *mask_;
    int step_[one];
    int d_[one];
    int mstep_;
    double a_[one];
    double b_[one];
    int size_[one];
    int imageWidth_;
    Size histogramSize_;
    int histogramType_;
    tbb::atomic<int> *globalHistogram_;
};

template <typename T>
class calcHist2D_Invoker
{
  public:
    calcHist2D_Invoker(const vector<uchar *> &_ptrs, const vector<int> &_deltas,
                       Mat &hist, const double *_uniranges, const int *size,
                       int dims, Size &imageSize, size_t *hstep)
        : mask_(_ptrs[dims]),
          mstep_(_deltas[dims * 2 + 1]),
          imageWidth_(imageSize.width),
          histogramSize_(hist.size()), histogramType_(hist.type()),
          globalHistogram_(hist.data)
    {
        p_[0] = ((T **)&_ptrs[0])[0];
        p_[1] = ((T **)&_ptrs[0])[1];
        step_[0] = (&_deltas[0])[1];
        step_[1] = (&_deltas[0])[3];
        d_[0] = (&_deltas[0])[0];
        d_[1] = (&_deltas[0])[2];
        a_[0] = (&_uniranges[0])[0];
        a_[1] = (&_uniranges[0])[2];
        b_[0] = (&_uniranges[0])[1];
        b_[1] = (&_uniranges[0])[3];
        size_[0] = size[0];
        size_[1] = size[1];
        hstep_[0] = hstep[0];
    }

    void operator()(const BlockedRange &range) const
    {
        T *p0 = p_[0] + range.begin() * (step_[0] + imageWidth_ * d_[0]);
        T *p1 = p_[1] + range.begin() * (step_[1] + imageWidth_ * d_[1]);
        uchar *mask = mask_ + range.begin() * mstep_;

        for (int row = range.begin(); row < range.end(); row++, p0 += step_[0], p1 += step_[1])
        {
            if (!mask_)
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1])
                {
                    int idx0 = cvFloor(*p0 * a_[0] + b_[0]);
                    int idx1 = cvFloor(*p1 * a_[1] + b_[1]);
                    if ((unsigned)idx0 < (unsigned)size_[0] && (unsigned)idx1 < (unsigned)size_[1])
                        ((tbb::atomic<int> *)(globalHistogram_ + hstep_[0] * idx0))[idx1].fetch_and_add(1);
                }
            }
            else
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1])
                {
                    if (mask[x])
                    {
                        int idx0 = cvFloor(*p0 * a_[0] + b_[0]);
                        int idx1 = cvFloor(*p1 * a_[1] + b_[1]);
                        if ((unsigned)idx0 < (unsigned)size_[0] && (unsigned)idx1 < (unsigned)size_[1])
                            ((tbb::atomic<int> *)(globalHistogram_ + hstep_[0] * idx0))[idx1].fetch_and_add(1);
                    }
                }
                mask += mstep_;
            }
        }
    }

  private:
    calcHist2D_Invoker operator=(const calcHist2D_Invoker &);

    T *p_[two];
    uchar *mask_;
    int step_[two];
    int d_[two];
    int mstep_;
    double a_[two];
    double b_[two];
    int size_[two];
    const int imageWidth_;
    size_t hstep_[one];
    Size histogramSize_;
    int histogramType_;
    uchar *globalHistogram_;
};

template <typename T>
class calcHist3D_Invoker
{
  public:
    calcHist3D_Invoker(const vector<uchar *> &_ptrs, const vector<int> &_deltas,
                       Size imsize, Mat &hist, const double *uniranges, int _dims,
                       size_t *hstep, int *size)
        : mask_(_ptrs[_dims]),
          mstep_(_deltas[_dims * 2 + 1]),
          imageWidth_(imsize.width),
          globalHistogram_(hist.data)
    {
        p_[0] = ((T **)&_ptrs[0])[0];
        p_[1] = ((T **)&_ptrs[0])[1];
        p_[2] = ((T **)&_ptrs[0])[2];
        step_[0] = (&_deltas[0])[1];
        step_[1] = (&_deltas[0])[3];
        step_[2] = (&_deltas[0])[5];
        d_[0] = (&_deltas[0])[0];
        d_[1] = (&_deltas[0])[2];
        d_[2] = (&_deltas[0])[4];
        a_[0] = uniranges[0];
        a_[1] = uniranges[2];
        a_[2] = uniranges[4];
        b_[0] = uniranges[1];
        b_[1] = uniranges[3];
        b_[2] = uniranges[5];
        size_[0] = size[0];
        size_[1] = size[1];
        size_[2] = size[2];
        hstep_[0] = hstep[0];
        hstep_[1] = hstep[1];
    }

    void operator()(const BlockedRange &range) const
    {
        T *p0 = p_[0] + range.begin() * (imageWidth_ * d_[0] + step_[0]);
        T *p1 = p_[1] + range.begin() * (imageWidth_ * d_[1] + step_[1]);
        T *p2 = p_[2] + range.begin() * (imageWidth_ * d_[2] + step_[2]);
        uchar *mask = mask_ + range.begin() * mstep_;

        for (int i = range.begin(); i < range.end(); i++, p0 += step_[0], p1 += step_[1], p2 += step_[2])
        {
            if (!mask_)
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1], p2 += d_[2])
                {
                    int idx0 = cvFloor(*p0 * a_[0] + b_[0]);
                    int idx1 = cvFloor(*p1 * a_[1] + b_[1]);
                    int idx2 = cvFloor(*p2 * a_[2] + b_[2]);
                    if ((unsigned)idx0 < (unsigned)size_[0] &&
                        (unsigned)idx1 < (unsigned)size_[1] &&
                        (unsigned)idx2 < (unsigned)size_[2])
                    {
                        ((tbb::atomic<int> *)(globalHistogram_ + hstep_[0] * idx0 + hstep_[1] * idx1))[idx2].fetch_and_add(1);
                    }
                }
            }
            else
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1], p2 += d_[2])
                {
                    if (mask[x])
                    {
                        int idx0 = cvFloor(*p0 * a_[0] + b_[0]);
                        int idx1 = cvFloor(*p1 * a_[1] + b_[1]);
                        int idx2 = cvFloor(*p2 * a_[2] + b_[2]);
                        if ((unsigned)idx0 < (unsigned)size_[0] &&
                            (unsigned)idx1 < (unsigned)size_[1] &&
                            (unsigned)idx2 < (unsigned)size_[2])
                        {
                            ((tbb::atomic<int> *)(globalHistogram_ + hstep_[0] * idx0 + hstep_[1] * idx1))[idx2].fetch_and_add(1);
                        }
                    }
                }
                mask += mstep_;
            }
        }
    }

    static bool isFit(const Mat &histogram, const Size imageSize)
    {
        return (imageSize.width * imageSize.height >= 320 * 240 && histogram.total() >= 8 * 8 * 8);
    }

  private:
    calcHist3D_Invoker operator=(const calcHist3D_Invoker &);

    T *p_[three];
    uchar *mask_;
    int step_[three];
    int d_[three];
    const int mstep_;
    double a_[three];
    double b_[three];
    int size_[three];
    int imageWidth_;
    size_t hstep_[two];
    uchar *globalHistogram_;
};

class CalcHist1D_8uInvoker
{
  public:
    CalcHist1D_8uInvoker(const vector<uchar *> &ptrs, const vector<int> &deltas,
                         Size imsize, Mat &hist, int dims, const vector<size_t> &tab,
                         tbb::mutex *lock)
        : mask_(ptrs[dims]),
          mstep_(deltas[dims * 2 + 1]),
          imageWidth_(imsize.width),
          imageSize_(imsize),
          histSize_(hist.size()), histType_(hist.type()),
          tab_((size_t *)&tab[0]),
          histogramWriteLock_(lock),
          globalHistogram_(hist.data)
    {
        p_[0] = (&ptrs[0])[0];
        step_[0] = (&deltas[0])[1];
        d_[0] = (&deltas[0])[0];
    }

    void operator()(const BlockedRange &range) const
    {
        int localHistogram[256] = {
            0,
        };
        uchar *mask = mask_;
        uchar *p0 = p_[0];
        int x;
        tbb::mutex::scoped_lock lock;

        if (!mask_)
        {
            int n = (imageWidth_ - 4) / 4 + 1;
            int tail = imageWidth_ - n * 4;

            int xN = 4 * n;
            p0 += (xN * d_[0] + tail * d_[0] + step_[0]) * range.begin();
        }
        else
        {
            p0 += (imageWidth_ * d_[0] + step_[0]) * range.begin();
            mask += mstep_ * range.begin();
        }

        for (int i = range.begin(); i < range.end(); i++, p0 += step_[0])
        {
            if (!mask_)
            {
                if (d_[0] == 1)
                {
                    for (x = 0; x <= imageWidth_ - 4; x += 4)
                    {
                        int t0 = p0[x], t1 = p0[x + 1];
                        localHistogram[t0]++;
                        localHistogram[t1]++;
                        t0 = p0[x + 2];
                        t1 = p0[x + 3];
                        localHistogram[t0]++;
                        localHistogram[t1]++;
                    }
                    p0 += x;
                }
                else
                {
                    for (x = 0; x <= imageWidth_ - 4; x += 4)
                    {
                        int t0 = p0[0], t1 = p0[d_[0]];
                        localHistogram[t0]++;
                        localHistogram[t1]++;
                        p0 += d_[0] * 2;
                        t0 = p0[0];
                        t1 = p0[d_[0]];
                        localHistogram[t0]++;
                        localHistogram[t1]++;
                        p0 += d_[0] * 2;
                    }
                }

                for (; x < imageWidth_; x++, p0 += d_[0])
                {
                    localHistogram[*p0]++;
                }
            }
            else
            {
                for (x = 0; x < imageWidth_; x++, p0 += d_[0])
                {
                    if (mask[x])
                    {
                        localHistogram[*p0]++;
                    }
                }
                mask += mstep_;
            }
        }

        lock.acquire(*histogramWriteLock_);
        for (int i = 0; i < 256; i++)
        {
            size_t hidx = tab_[i];
            if (hidx < OUT_OF_RANGE)
            {
                *(int *)((globalHistogram_ + hidx)) += localHistogram[i];
            }
        }
        lock.release();
    }

    static bool isFit(const Mat &histogram, const Size imageSize)
    {
        return (histogram.total() >= 8 && imageSize.width * imageSize.height >= 160 * 120);
    }

  private:
    uchar *p_[one];
    uchar *mask_;
    int mstep_;
    int step_[one];
    int d_[one];
    int imageWidth_;
    Size imageSize_;
    Size histSize_;
    int histType_;
    size_t *tab_;
    tbb::mutex *histogramWriteLock_;
    uchar *globalHistogram_;
};

class CalcHist2D_8uInvoker
{
  public:
    CalcHist2D_8uInvoker(const vector<uchar *> &_ptrs, const vector<int> &_deltas,
                         Size imsize, Mat &hist, int dims, const vector<size_t> &_tab,
                         tbb::mutex *lock)
        : mask_(_ptrs[dims]),
          mstep_(_deltas[dims * 2 + 1]),
          imageWidth_(imsize.width),
          histSize_(hist.size()), histType_(hist.type()),
          tab_((size_t *)&_tab[0]),
          histogramWriteLock_(lock),
          globalHistogram_(hist.data)
    {
        p_[0] = (uchar *)(&_ptrs[0])[0];
        p_[1] = (uchar *)(&_ptrs[0])[1];
        step_[0] = (&_deltas[0])[1];
        step_[1] = (&_deltas[0])[3];
        d_[0] = (&_deltas[0])[0];
        d_[1] = (&_deltas[0])[2];
    }

    void operator()(const BlockedRange &range) const
    {
        uchar *p0 = p_[0] + range.begin() * (step_[0] + imageWidth_ * d_[0]);
        uchar *p1 = p_[1] + range.begin() * (step_[1] + imageWidth_ * d_[1]);
        uchar *mask = mask_ + range.begin() * mstep_;

        Mat localHist = Mat::zeros(histSize_, histType_);
        uchar *localHistData = localHist.data;
        tbb::mutex::scoped_lock lock;

        for (int i = range.begin(); i < range.end(); i++, p0 += step_[0], p1 += step_[1])
        {
            if (!mask_)
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1])
                {
                    size_t idx = tab_[*p0] + tab_[*p1 + 256];
                    if (idx < OUT_OF_RANGE)
                    {
                        ++*(int *)(localHistData + idx);
                    }
                }
            }
            else
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1])
                {
                    size_t idx;
                    if (mask[x] && (idx = tab_[*p0] + tab_[*p1 + 256]) < OUT_OF_RANGE)
                    {
                        ++*(int *)(localHistData + idx);
                    }
                }
                mask += mstep_;
            }
        }

        lock.acquire(*histogramWriteLock_);
        for (int i = 0; i < histSize_.width * histSize_.height; i++)
        {
            ((int *)globalHistogram_)[i] += ((int *)localHistData)[i];
        }
        lock.release();
    }

    static bool isFit(const Mat &histogram, const Size imageSize)
    {
        return ((histogram.total() > 4 * 4 && histogram.total() <= 116 * 116 && imageSize.width * imageSize.height >= 320 * 240) || (histogram.total() > 116 * 116 && imageSize.width * imageSize.height >= 1280 * 720));
    }

  private:
    uchar *p_[two];
    uchar *mask_;
    int step_[two];
    int d_[two];
    int mstep_;
    int imageWidth_;
    Size histSize_;
    int histType_;
    size_t *tab_;
    tbb::mutex *histogramWriteLock_;
    uchar *globalHistogram_;
};

class CalcHist3D_8uInvoker
{
  public:
    CalcHist3D_8uInvoker(const vector<uchar *> &_ptrs, const vector<int> &_deltas,
                         Size imsize, Mat &hist, int dims, const vector<size_t> &tab)
        : mask_(_ptrs[dims]),
          mstep_(_deltas[dims * 2 + 1]),
          histogramSize_(hist.size.p), histogramType_(hist.type()),
          imageWidth_(imsize.width),
          tab_((size_t *)&tab[0]),
          globalHistogram_(hist.data)
    {
        p_[0] = (uchar *)(&_ptrs[0])[0];
        p_[1] = (uchar *)(&_ptrs[0])[1];
        p_[2] = (uchar *)(&_ptrs[0])[2];
        step_[0] = (&_deltas[0])[1];
        step_[1] = (&_deltas[0])[3];
        step_[2] = (&_deltas[0])[5];
        d_[0] = (&_deltas[0])[0];
        d_[1] = (&_deltas[0])[2];
        d_[2] = (&_deltas[0])[4];
    }

    void operator()(const BlockedRange &range) const
    {
        uchar *p0 = p_[0] + range.begin() * (step_[0] + imageWidth_ * d_[0]);
        uchar *p1 = p_[1] + range.begin() * (step_[1] + imageWidth_ * d_[1]);
        uchar *p2 = p_[2] + range.begin() * (step_[2] + imageWidth_ * d_[2]);
        uchar *mask = mask_ + range.begin() * mstep_;

        for (int i = range.begin(); i < range.end(); i++, p0 += step_[0], p1 += step_[1], p2 += step_[2])
        {
            if (!mask_)
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1], p2 += d_[2])
                {
                    size_t idx = tab_[*p0] + tab_[*p1 + 256] + tab_[*p2 + 512];
                    if (idx < OUT_OF_RANGE)
                    {
                        (*(tbb::atomic<int> *)(globalHistogram_ + idx)).fetch_and_add(1);
                    }
                }
            }
            else
            {
                for (int x = 0; x < imageWidth_; x++, p0 += d_[0], p1 += d_[1], p2 += d_[2])
                {
                    size_t idx;
                    if (mask[x] && (idx = tab_[*p0] + tab_[*p1 + 256] + tab_[*p2 + 512]) < OUT_OF_RANGE)
                    {
                        (*(tbb::atomic<int> *)(globalHistogram_ + idx)).fetch_and_add(1);
                    }
                }
                mask += mstep_;
            }
        }
    }

    static bool isFit(const Mat &histogram, const Size imageSize)
    {
        return (histogram.total() >= 128 * 128 * 128 && imageSize.width * imageSize.width >= 320 * 240);
    }

  private:
    uchar *p_[three];
    uchar *mask_;
    int mstep_;
    int step_[three];
    int d_[three];
    int *histogramSize_;
    int histogramType_;
    int imageWidth_;
    size_t *tab_;
    uchar *globalHistogram_;
};

static void
callCalcHist2D_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                  Size imsize, Mat &hist, int dims, vector<size_t> &_tab)
{
    int grainSize = imsize.height / tbb::task_scheduler_init::default_num_threads();
    tbb::mutex histogramWriteLock;

    CalcHist2D_8uInvoker body(_ptrs, _deltas, imsize, hist, dims, _tab, &histogramWriteLock);
    parallel_for(BlockedRange(0, imsize.height, grainSize), body);
}

static void
callCalcHist3D_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                  Size imsize, Mat &hist, int dims, vector<size_t> &_tab)
{
    CalcHist3D_8uInvoker body(_ptrs, _deltas, imsize, hist, dims, _tab);
    parallel_for(BlockedRange(0, imsize.height), body);
}
#endif

template <typename T>
static void
calcHist_(vector<uchar *> &_ptrs, const vector<int> &_deltas,
          Size imsize, Mat &hist, int dims, const float **_ranges,
          const double *_uniranges, bool uniform)
{
    T **ptrs = (T **)&_ptrs[0];
    const int *deltas = &_deltas[0];
    uchar *H = hist.data;
    int i, x;
    const uchar *mask = _ptrs[dims];
    int mstep = _deltas[dims * 2 + 1];
    int size[CV_MAX_DIM];
    size_t hstep[CV_MAX_DIM];

    for (i = 0; i < dims; i++)
    {
        size[i] = hist.size[i];
        hstep[i] = hist.step[i];
    }

    if (uniform)
    {
        const double *uniranges = &_uniranges[0];

        if (dims == 1)
        {
#ifdef HAVE_TBB
            calcHist1D_Invoker<T> body(_ptrs, _deltas, hist, _uniranges, size[0], dims, imsize);
            parallel_for(BlockedRange(0, imsize.height), body);
#else
            double a = uniranges[0], b = uniranges[1];
            int sz = size[0], d0 = deltas[0], step0 = deltas[1];
            const T *p0 = (const T *)ptrs[0];

            for (; imsize.height--; p0 += step0, mask += mstep)
            {
                if (!mask)
                    for (x = 0; x < imsize.width; x++, p0 += d0)
                    {
                        int idx = cvFloor(*p0 * a + b);
                        if ((unsigned)idx < (unsigned)sz)
                            ((int *)H)[idx]++;
                    }
                else
                    for (x = 0; x < imsize.width; x++, p0 += d0)
                        if (mask[x])
                        {
                            int idx = cvFloor(*p0 * a + b);
                            if ((unsigned)idx < (unsigned)sz)
                                ((int *)H)[idx]++;
                        }
            }
#endif //HAVE_TBB
            return;
        }
        else if (dims == 2)
        {
#ifdef HAVE_TBB
            calcHist2D_Invoker<T> body(_ptrs, _deltas, hist, _uniranges, size, dims, imsize, hstep);
            parallel_for(BlockedRange(0, imsize.height), body);
#else
            double a0 = uniranges[0], b0 = uniranges[1], a1 = uniranges[2], b1 = uniranges[3];
            int sz0 = size[0], sz1 = size[1];
            int d0 = deltas[0], step0 = deltas[1],
                d1 = deltas[2], step1 = deltas[3];
            size_t hstep0 = hstep[0];
            const T *p0 = (const T *)ptrs[0];
            const T *p1 = (const T *)ptrs[1];

            for (; imsize.height--; p0 += step0, p1 += step1, mask += mstep)
            {
                if (!mask)
                    for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
                    {
                        int idx0 = cvFloor(*p0 * a0 + b0);
                        int idx1 = cvFloor(*p1 * a1 + b1);
                        if ((unsigned)idx0 < (unsigned)sz0 && (unsigned)idx1 < (unsigned)sz1)
                            ((int *)(H + hstep0 * idx0))[idx1]++;
                    }
                else
                    for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
                        if (mask[x])
                        {
                            int idx0 = cvFloor(*p0 * a0 + b0);
                            int idx1 = cvFloor(*p1 * a1 + b1);
                            if ((unsigned)idx0 < (unsigned)sz0 && (unsigned)idx1 < (unsigned)sz1)
                                ((int *)(H + hstep0 * idx0))[idx1]++;
                        }
            }
#endif //HAVE_TBB
            return;
        }
        else if (dims == 3)
        {
#ifdef HAVE_TBB
            if (calcHist3D_Invoker<T>::isFit(hist, imsize))
            {
                calcHist3D_Invoker<T> body(_ptrs, _deltas, imsize, hist, uniranges, dims, hstep, size);
                parallel_for(BlockedRange(0, imsize.height), body);
                return;
            }
#endif
            double a0 = uniranges[0], b0 = uniranges[1],
                   a1 = uniranges[2], b1 = uniranges[3],
                   a2 = uniranges[4], b2 = uniranges[5];
            int sz0 = size[0], sz1 = size[1], sz2 = size[2];
            int d0 = deltas[0], step0 = deltas[1],
                d1 = deltas[2], step1 = deltas[3],
                d2 = deltas[4], step2 = deltas[5];
            size_t hstep0 = hstep[0], hstep1 = hstep[1];
            const T *p0 = (const T *)ptrs[0];
            const T *p1 = (const T *)ptrs[1];
            const T *p2 = (const T *)ptrs[2];

            for (; imsize.height--; p0 += step0, p1 += step1, p2 += step2, mask += mstep)
            {
                if (!mask)
                    for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
                    {
                        int idx0 = cvFloor(*p0 * a0 + b0);
                        int idx1 = cvFloor(*p1 * a1 + b1);
                        int idx2 = cvFloor(*p2 * a2 + b2);
                        if ((unsigned)idx0 < (unsigned)sz0 &&
                            (unsigned)idx1 < (unsigned)sz1 &&
                            (unsigned)idx2 < (unsigned)sz2)
                            ((int *)(H + hstep0 * idx0 + hstep1 * idx1))[idx2]++;
                    }
                else
                    for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
                        if (mask[x])
                        {
                            int idx0 = cvFloor(*p0 * a0 + b0);
                            int idx1 = cvFloor(*p1 * a1 + b1);
                            int idx2 = cvFloor(*p2 * a2 + b2);
                            if ((unsigned)idx0 < (unsigned)sz0 &&
                                (unsigned)idx1 < (unsigned)sz1 &&
                                (unsigned)idx2 < (unsigned)sz2)
                                ((int *)(H + hstep0 * idx0 + hstep1 * idx1))[idx2]++;
                        }
            }
        }
        else
        {
            for (; imsize.height--; mask += mstep)
            {
                if (!mask)
                    for (x = 0; x < imsize.width; x++)
                    {
                        uchar *Hptr = H;
                        for (i = 0; i < dims; i++)
                        {
                            int idx = cvFloor(*ptrs[i] * uniranges[i * 2] + uniranges[i * 2 + 1]);
                            if ((unsigned)idx >= (unsigned)size[i])
                                break;
                            ptrs[i] += deltas[i * 2];
                            Hptr += idx * hstep[i];
                        }

                        if (i == dims)
                            ++*((int *)Hptr);
                        else
                            for (; i < dims; i++)
                                ptrs[i] += deltas[i * 2];
                    }
                else
                    for (x = 0; x < imsize.width; x++)
                    {
                        uchar *Hptr = H;
                        i = 0;
                        if (mask[x])
                            for (; i < dims; i++)
                            {
                                int idx = cvFloor(*ptrs[i] * uniranges[i * 2] + uniranges[i * 2 + 1]);
                                if ((unsigned)idx >= (unsigned)size[i])
                                    break;
                                ptrs[i] += deltas[i * 2];
                                Hptr += idx * hstep[i];
                            }

                        if (i == dims)
                            ++*((int *)Hptr);
                        else
                            for (; i < dims; i++)
                                ptrs[i] += deltas[i * 2];
                    }
                for (i = 0; i < dims; i++)
                    ptrs[i] += deltas[i * 2 + 1];
            }
        }
    }
    else
    {
        // non-uniform histogram
        const float *ranges[CV_MAX_DIM];
        for (i = 0; i < dims; i++)
            ranges[i] = &_ranges[i][0];

        for (; imsize.height--; mask += mstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                uchar *Hptr = H;
                i = 0;

                if (!mask || mask[x])
                    for (; i < dims; i++)
                    {
                        float v = (float)*ptrs[i];
                        const float *R = ranges[i];
                        int idx = -1, sz = size[i];

                        while (v >= R[idx + 1] && ++idx < sz)
                            ; // nop

                        if ((unsigned)idx >= (unsigned)sz)
                            break;

                        ptrs[i] += deltas[i * 2];
                        Hptr += idx * hstep[i];
                    }

                if (i == dims)
                    ++*((int *)Hptr);
                else
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
            }

            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}

static void
calcHist_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
            Size imsize, Mat &hist, int dims, const float **_ranges,
            const double *_uniranges, bool uniform)
{
    uchar **ptrs = &_ptrs[0];
    const int *deltas = &_deltas[0];
    uchar *H = hist.data;
    int x;
    const uchar *mask = _ptrs[dims];
    int mstep = _deltas[dims * 2 + 1];
    vector<size_t> _tab;

    calcHistLookupTables_8u(hist, SparseMat(), dims, _ranges, _uniranges, uniform, false, _tab);
    const size_t *tab = &_tab[0];

    if (dims == 1)
    {
#ifdef HAVE_TBB
        if (CalcHist1D_8uInvoker::isFit(hist, imsize))
        {
            int treadsNumber = tbb::task_scheduler_init::default_num_threads();
            int grainSize = imsize.height / treadsNumber;
            tbb::mutex histogramWriteLock;

            CalcHist1D_8uInvoker body(_ptrs, _deltas, imsize, hist, dims, _tab, &histogramWriteLock);
            parallel_for(BlockedRange(0, imsize.height, grainSize), body);
            return;
        }
#endif
        int d0 = deltas[0], step0 = deltas[1];
        int matH[256] = {
            0,
        };
        const uchar *p0 = (const uchar *)ptrs[0];

        for (; imsize.height--; p0 += step0, mask += mstep)
        {
            if (!mask)
            {
                if (d0 == 1)
                {
                    for (x = 0; x <= imsize.width - 4; x += 4)
                    {
                        int t0 = p0[x], t1 = p0[x + 1];
                        matH[t0]++;
                        matH[t1]++;
                        t0 = p0[x + 2];
                        t1 = p0[x + 3];
                        matH[t0]++;
                        matH[t1]++;
                    }
                    p0 += x;
                }
                else
                    for (x = 0; x <= imsize.width - 4; x += 4)
                    {
                        int t0 = p0[0], t1 = p0[d0];
                        matH[t0]++;
                        matH[t1]++;
                        p0 += d0 * 2;
                        t0 = p0[0];
                        t1 = p0[d0];
                        matH[t0]++;
                        matH[t1]++;
                        p0 += d0 * 2;
                    }

                for (; x < imsize.width; x++, p0 += d0)
                    matH[*p0]++;
            }
            else
                for (x = 0; x < imsize.width; x++, p0 += d0)
                    if (mask[x])
                        matH[*p0]++;
        }

        for (int i = 0; i < 256; i++)
        {
            size_t hidx = tab[i];
            if (hidx < OUT_OF_RANGE)
                *(int *)(H + hidx) += matH[i];
        }
    }
    else if (dims == 2)
    {
#ifdef HAVE_TBB
        if (CalcHist2D_8uInvoker::isFit(hist, imsize))
        {
            callCalcHist2D_8u(_ptrs, _deltas, imsize, hist, dims, _tab);
            return;
        }
#endif
        int d0 = deltas[0], step0 = deltas[1],
            d1 = deltas[2], step1 = deltas[3];
        const uchar *p0 = (const uchar *)ptrs[0];
        const uchar *p1 = (const uchar *)ptrs[1];

        for (; imsize.height--; p0 += step0, p1 += step1, mask += mstep)
        {
            if (!mask)
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
                {
                    size_t idx = tab[*p0] + tab[*p1 + 256];
                    if (idx < OUT_OF_RANGE)
                        ++*(int *)(H + idx);
                }
            else
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
                {
                    size_t idx;
                    if (mask[x] && (idx = tab[*p0] + tab[*p1 + 256]) < OUT_OF_RANGE)
                        ++*(int *)(H + idx);
                }
        }
    }
    else if (dims == 3)
    {
#ifdef HAVE_TBB
        if (CalcHist3D_8uInvoker::isFit(hist, imsize))
        {
            callCalcHist3D_8u(_ptrs, _deltas, imsize, hist, dims, _tab);
            return;
        }
#endif
        int d0 = deltas[0], step0 = deltas[1],
            d1 = deltas[2], step1 = deltas[3],
            d2 = deltas[4], step2 = deltas[5];

        const uchar *p0 = (const uchar *)ptrs[0];
        const uchar *p1 = (const uchar *)ptrs[1];
        const uchar *p2 = (const uchar *)ptrs[2];

        for (; imsize.height--; p0 += step0, p1 += step1, p2 += step2, mask += mstep)
        {
            if (!mask)
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
                {
                    size_t idx = tab[*p0] + tab[*p1 + 256] + tab[*p2 + 512];
                    if (idx < OUT_OF_RANGE)
                        ++*(int *)(H + idx);
                }
            else
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
                {
                    size_t idx;
                    if (mask[x] && (idx = tab[*p0] + tab[*p1 + 256] + tab[*p2 + 512]) < OUT_OF_RANGE)
                        ++*(int *)(H + idx);
                }
        }
    }
    else
    {
        for (; imsize.height--; mask += mstep)
        {
            if (!mask)
                for (x = 0; x < imsize.width; x++)
                {
                    uchar *Hptr = H;
                    int i = 0;
                    for (; i < dims; i++)
                    {
                        size_t idx = tab[*ptrs[i] + i * 256];
                        if (idx >= OUT_OF_RANGE)
                            break;
                        Hptr += idx;
                        ptrs[i] += deltas[i * 2];
                    }

                    if (i == dims)
                        ++*((int *)Hptr);
                    else
                        for (; i < dims; i++)
                            ptrs[i] += deltas[i * 2];
                }
            else
                for (x = 0; x < imsize.width; x++)
                {
                    uchar *Hptr = H;
                    int i = 0;
                    if (mask[x])
                        for (; i < dims; i++)
                        {
                            size_t idx = tab[*ptrs[i] + i * 256];
                            if (idx >= OUT_OF_RANGE)
                                break;
                            Hptr += idx;
                            ptrs[i] += deltas[i * 2];
                        }

                    if (i == dims)
                        ++*((int *)Hptr);
                    else
                        for (; i < dims; i++)
                            ptrs[i] += deltas[i * 2];
                }
            for (int i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}
}

void cv::calcHist(const Mat *images, int nimages, const int *channels,
                  InputArray _mask, OutputArray _hist, int dims, const int *histSize,
                  const float **ranges, bool uniform, bool accumulate)
{
    Mat mask = _mask.getMat();

    CV_Assert(dims > 0 && histSize);

    uchar *histdata = _hist.getMat().data;
    _hist.create(dims, histSize, CV_32F);
    Mat hist = _hist.getMat(), ihist = hist;
    ihist.flags = (ihist.flags & ~CV_MAT_TYPE_MASK) | CV_32S;

    if (!accumulate || histdata != hist.data)
        hist = Scalar(0.);
    else
        hist.convertTo(ihist, CV_32S);

    vector<uchar *> ptrs;
    vector<int> deltas;
    vector<double> uniranges;
    Size imsize;

    CV_Assert(!mask.data || mask.type() == CV_8UC1);
    histPrepareImages(images, nimages, channels, mask, dims, hist.size, ranges,
                      uniform, ptrs, deltas, imsize, uniranges);
    const double *_uniranges = uniform ? &uniranges[0] : 0;

    int depth = images[0].depth();

    if (depth == CV_8U)
        calcHist_8u(ptrs, deltas, imsize, ihist, dims, ranges, _uniranges, uniform);
    else if (depth == CV_16U)
        calcHist_<ushort>(ptrs, deltas, imsize, ihist, dims, ranges, _uniranges, uniform);
    else if (depth == CV_32F)
        calcHist_<float>(ptrs, deltas, imsize, ihist, dims, ranges, _uniranges, uniform);
    else
        CV_Error(CV_StsUnsupportedFormat, "");

    ihist.convertTo(hist, CV_32F);
}

namespace cv
{

template <typename T>
static void
calcSparseHist_(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                Size imsize, SparseMat &hist, int dims, const float **_ranges,
                const double *_uniranges, bool uniform)
{
    T **ptrs = (T **)&_ptrs[0];
    const int *deltas = &_deltas[0];
    int i, x;
    const uchar *mask = _ptrs[dims];
    int mstep = _deltas[dims * 2 + 1];
    const int *size = hist.hdr->size;
    int idx[CV_MAX_DIM];

    if (uniform)
    {
        const double *uniranges = &_uniranges[0];

        for (; imsize.height--; mask += mstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                i = 0;
                if (!mask || mask[x])
                    for (; i < dims; i++)
                    {
                        idx[i] = cvFloor(*ptrs[i] * uniranges[i * 2] + uniranges[i * 2 + 1]);
                        if ((unsigned)idx[i] >= (unsigned)size[i])
                            break;
                        ptrs[i] += deltas[i * 2];
                    }

                if (i == dims)
                    ++*(int *)hist.ptr(idx, true);
                else
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
            }
            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
    else
    {
        // non-uniform histogram
        const float *ranges[CV_MAX_DIM];
        for (i = 0; i < dims; i++)
            ranges[i] = &_ranges[i][0];

        for (; imsize.height--; mask += mstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                i = 0;

                if (!mask || mask[x])
                    for (; i < dims; i++)
                    {
                        float v = (float)*ptrs[i];
                        const float *R = ranges[i];
                        int j = -1, sz = size[i];

                        while (v >= R[j + 1] && ++j < sz)
                            ; // nop

                        if ((unsigned)j >= (unsigned)sz)
                            break;
                        ptrs[i] += deltas[i * 2];
                        idx[i] = j;
                    }

                if (i == dims)
                    ++*(int *)hist.ptr(idx, true);
                else
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
            }

            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}

static void
calcSparseHist_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                  Size imsize, SparseMat &hist, int dims, const float **_ranges,
                  const double *_uniranges, bool uniform)
{
    uchar **ptrs = (uchar **)&_ptrs[0];
    const int *deltas = &_deltas[0];
    int x;
    const uchar *mask = _ptrs[dims];
    int mstep = _deltas[dims * 2 + 1];
    int idx[CV_MAX_DIM];
    vector<size_t> _tab;

    calcHistLookupTables_8u(Mat(), hist, dims, _ranges, _uniranges, uniform, true, _tab);
    const size_t *tab = &_tab[0];

    for (; imsize.height--; mask += mstep)
    {
        for (x = 0; x < imsize.width; x++)
        {
            int i = 0;
            if (!mask || mask[x])
                for (; i < dims; i++)
                {
                    size_t hidx = tab[*ptrs[i] + i * 256];
                    if (hidx >= OUT_OF_RANGE)
                        break;
                    ptrs[i] += deltas[i * 2];
                    idx[i] = (int)hidx;
                }

            if (i == dims)
                ++*(int *)hist.ptr(idx, true);
            else
                for (; i < dims; i++)
                    ptrs[i] += deltas[i * 2];
        }
        for (int i = 0; i < dims; i++)
            ptrs[i] += deltas[i * 2 + 1];
    }
}

static void calcHist(const Mat *images, int nimages, const int *channels,
                     const Mat &mask, SparseMat &hist, int dims, const int *histSize,
                     const float **ranges, bool uniform, bool accumulate, bool keepInt)
{
    size_t i, N;

    if (!accumulate)
        hist.create(dims, histSize, CV_32F);
    else
    {
        SparseMatIterator it = hist.begin();
        for (i = 0, N = hist.nzcount(); i < N; i++, ++it)
        {
            Cv32suf *val = (Cv32suf *)it.ptr;
            val->i = cvRound(val->f);
        }
    }

    vector<uchar *> ptrs;
    vector<int> deltas;
    vector<double> uniranges;
    Size imsize;

    CV_Assert(!mask.data || mask.type() == CV_8UC1);
    histPrepareImages(images, nimages, channels, mask, dims, hist.hdr->size, ranges,
                      uniform, ptrs, deltas, imsize, uniranges);
    const double *_uniranges = uniform ? &uniranges[0] : 0;

    int depth = images[0].depth();
    if (depth == CV_8U)
        calcSparseHist_8u(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, uniform);
    else if (depth == CV_16U)
        calcSparseHist_<ushort>(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, uniform);
    else if (depth == CV_32F)
        calcSparseHist_<float>(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, uniform);
    else
        CV_Error(CV_StsUnsupportedFormat, "");

    if (!keepInt)
    {
        SparseMatIterator it = hist.begin();
        for (i = 0, N = hist.nzcount(); i < N; i++, ++it)
        {
            Cv32suf *val = (Cv32suf *)it.ptr;
            val->f = (float)val->i;
        }
    }
}
}

void cv::calcHist(const Mat *images, int nimages, const int *channels,
                  InputArray _mask, SparseMat &hist, int dims, const int *histSize,
                  const float **ranges, bool uniform, bool accumulate)
{
    Mat mask = _mask.getMat();
    calcHist(images, nimages, channels, mask, hist, dims, histSize,
             ranges, uniform, accumulate, false);
}

void cv::calcHist(InputArrayOfArrays images, const vector<int> &channels,
                  InputArray mask, OutputArray hist,
                  const vector<int> &histSize,
                  const vector<float> &ranges,
                  bool accumulate)
{
    int i, dims = (int)histSize.size(), rsz = (int)ranges.size(), csz = (int)channels.size();
    int nimages = (int)images.total();

    CV_Assert(nimages > 0 && dims > 0);
    CV_Assert(rsz == dims * 2 || (rsz == 0 && images.depth(0) == CV_8U));
    CV_Assert(csz == 0 || csz == dims);
    float *_ranges[CV_MAX_DIM];
    if (rsz > 0)
    {
        for (i = 0; i < rsz / 2; i++)
            _ranges[i] = (float *)&ranges[i * 2];
    }

    AutoBuffer<Mat> buf(nimages);
    for (i = 0; i < nimages; i++)
        buf[i] = images.getMat(i);

    calcHist(&buf[0], nimages, csz ? &channels[0] : 0,
             mask, hist, dims, &histSize[0], rsz ? (const float **)_ranges : 0,
             true, accumulate);
}

/////////////////////////////////////// B A C K   P R O J E C T ////////////////////////////////////

namespace cv
{

template <typename T, typename BT>
static void
calcBackProj_(vector<uchar *> &_ptrs, const vector<int> &_deltas,
              Size imsize, const Mat &hist, int dims, const float **_ranges,
              const double *_uniranges, float scale, bool uniform)
{
    T **ptrs = (T **)&_ptrs[0];
    const int *deltas = &_deltas[0];
    uchar *H = hist.data;
    int i, x;
    BT *bproj = (BT *)_ptrs[dims];
    int bpstep = _deltas[dims * 2 + 1];
    int size[CV_MAX_DIM];
    size_t hstep[CV_MAX_DIM];

    for (i = 0; i < dims; i++)
    {
        size[i] = hist.size[i];
        hstep[i] = hist.step[i];
    }

    if (uniform)
    {
        const double *uniranges = &_uniranges[0];

        if (dims == 1)
        {
            double a = uniranges[0], b = uniranges[1];
            int sz = size[0], d0 = deltas[0], step0 = deltas[1];
            const T *p0 = (const T *)ptrs[0];

            for (; imsize.height--; p0 += step0, bproj += bpstep)
            {
                for (x = 0; x < imsize.width; x++, p0 += d0)
                {
                    int idx = cvFloor(*p0 * a + b);
                    bproj[x] = (unsigned)idx < (unsigned)sz ? saturate_cast<BT>(((float *)H)[idx] * scale) : 0;
                }
            }
        }
        else if (dims == 2)
        {
            double a0 = uniranges[0], b0 = uniranges[1],
                   a1 = uniranges[2], b1 = uniranges[3];
            int sz0 = size[0], sz1 = size[1];
            int d0 = deltas[0], step0 = deltas[1],
                d1 = deltas[2], step1 = deltas[3];
            size_t hstep0 = hstep[0];
            const T *p0 = (const T *)ptrs[0];
            const T *p1 = (const T *)ptrs[1];

            for (; imsize.height--; p0 += step0, p1 += step1, bproj += bpstep)
            {
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
                {
                    int idx0 = cvFloor(*p0 * a0 + b0);
                    int idx1 = cvFloor(*p1 * a1 + b1);
                    bproj[x] = (unsigned)idx0 < (unsigned)sz0 &&
                                       (unsigned)idx1 < (unsigned)sz1
                                   ? saturate_cast<BT>(((float *)(H + hstep0 * idx0))[idx1] * scale)
                                   : 0;
                }
            }
        }
        else if (dims == 3)
        {
            double a0 = uniranges[0], b0 = uniranges[1],
                   a1 = uniranges[2], b1 = uniranges[3],
                   a2 = uniranges[4], b2 = uniranges[5];
            int sz0 = size[0], sz1 = size[1], sz2 = size[2];
            int d0 = deltas[0], step0 = deltas[1],
                d1 = deltas[2], step1 = deltas[3],
                d2 = deltas[4], step2 = deltas[5];
            size_t hstep0 = hstep[0], hstep1 = hstep[1];
            const T *p0 = (const T *)ptrs[0];
            const T *p1 = (const T *)ptrs[1];
            const T *p2 = (const T *)ptrs[2];

            for (; imsize.height--; p0 += step0, p1 += step1, p2 += step2, bproj += bpstep)
            {
                for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
                {
                    int idx0 = cvFloor(*p0 * a0 + b0);
                    int idx1 = cvFloor(*p1 * a1 + b1);
                    int idx2 = cvFloor(*p2 * a2 + b2);
                    bproj[x] = (unsigned)idx0 < (unsigned)sz0 &&
                                       (unsigned)idx1 < (unsigned)sz1 &&
                                       (unsigned)idx2 < (unsigned)sz2
                                   ? saturate_cast<BT>(((float *)(H + hstep0 * idx0 + hstep1 * idx1))[idx2] * scale)
                                   : 0;
                }
            }
        }
        else
        {
            for (; imsize.height--; bproj += bpstep)
            {
                for (x = 0; x < imsize.width; x++)
                {
                    uchar *Hptr = H;
                    for (i = 0; i < dims; i++)
                    {
                        int idx = cvFloor(*ptrs[i] * uniranges[i * 2] + uniranges[i * 2 + 1]);
                        if ((unsigned)idx >= (unsigned)size[i] || (_ranges && *ptrs[i] >= _ranges[i][1]))
                            break;
                        ptrs[i] += deltas[i * 2];
                        Hptr += idx * hstep[i];
                    }

                    if (i == dims)
                        bproj[x] = saturate_cast<BT>(*(float *)Hptr * scale);
                    else
                    {
                        bproj[x] = 0;
                        for (; i < dims; i++)
                            ptrs[i] += deltas[i * 2];
                    }
                }
                for (i = 0; i < dims; i++)
                    ptrs[i] += deltas[i * 2 + 1];
            }
        }
    }
    else
    {
        // non-uniform histogram
        const float *ranges[CV_MAX_DIM];
        for (i = 0; i < dims; i++)
            ranges[i] = &_ranges[i][0];

        for (; imsize.height--; bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                uchar *Hptr = H;
                for (i = 0; i < dims; i++)
                {
                    float v = (float)*ptrs[i];
                    const float *R = ranges[i];
                    int idx = -1, sz = size[i];

                    while (v >= R[idx + 1] && ++idx < sz)
                        ; // nop

                    if ((unsigned)idx >= (unsigned)sz)
                        break;

                    ptrs[i] += deltas[i * 2];
                    Hptr += idx * hstep[i];
                }

                if (i == dims)
                    bproj[x] = saturate_cast<BT>(*(float *)Hptr * scale);
                else
                {
                    bproj[x] = 0;
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
                }
            }

            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}

static void
calcBackProj_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                Size imsize, const Mat &hist, int dims, const float **_ranges,
                const double *_uniranges, float scale, bool uniform)
{
    uchar **ptrs = &_ptrs[0];
    const int *deltas = &_deltas[0];
    uchar *H = hist.data;
    int i, x;
    uchar *bproj = _ptrs[dims];
    int bpstep = _deltas[dims * 2 + 1];
    vector<size_t> _tab;

    calcHistLookupTables_8u(hist, SparseMat(), dims, _ranges, _uniranges, uniform, false, _tab);
    const size_t *tab = &_tab[0];

    if (dims == 1)
    {
        int d0 = deltas[0], step0 = deltas[1];
        uchar matH[256] = {0};
        const uchar *p0 = (const uchar *)ptrs[0];

        for (i = 0; i < 256; i++)
        {
            size_t hidx = tab[i];
            if (hidx < OUT_OF_RANGE)
                matH[i] = saturate_cast<uchar>(*(float *)(H + hidx) * scale);
        }

        for (; imsize.height--; p0 += step0, bproj += bpstep)
        {
            if (d0 == 1)
            {
                for (x = 0; x <= imsize.width - 4; x += 4)
                {
                    uchar t0 = matH[p0[x]], t1 = matH[p0[x + 1]];
                    bproj[x] = t0;
                    bproj[x + 1] = t1;
                    t0 = matH[p0[x + 2]];
                    t1 = matH[p0[x + 3]];
                    bproj[x + 2] = t0;
                    bproj[x + 3] = t1;
                }
                p0 += x;
            }
            else
                for (x = 0; x <= imsize.width - 4; x += 4)
                {
                    uchar t0 = matH[p0[0]], t1 = matH[p0[d0]];
                    bproj[x] = t0;
                    bproj[x + 1] = t1;
                    p0 += d0 * 2;
                    t0 = matH[p0[0]];
                    t1 = matH[p0[d0]];
                    bproj[x + 2] = t0;
                    bproj[x + 3] = t1;
                    p0 += d0 * 2;
                }

            for (; x < imsize.width; x++, p0 += d0)
                bproj[x] = matH[*p0];
        }
    }
    else if (dims == 2)
    {
        int d0 = deltas[0], step0 = deltas[1],
            d1 = deltas[2], step1 = deltas[3];
        const uchar *p0 = (const uchar *)ptrs[0];
        const uchar *p1 = (const uchar *)ptrs[1];

        for (; imsize.height--; p0 += step0, p1 += step1, bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1)
            {
                size_t idx = tab[*p0] + tab[*p1 + 256];
                bproj[x] = idx < OUT_OF_RANGE ? saturate_cast<uchar>(*(float *)(H + idx) * scale) : 0;
            }
        }
    }
    else if (dims == 3)
    {
        int d0 = deltas[0], step0 = deltas[1],
            d1 = deltas[2], step1 = deltas[3],
            d2 = deltas[4], step2 = deltas[5];
        const uchar *p0 = (const uchar *)ptrs[0];
        const uchar *p1 = (const uchar *)ptrs[1];
        const uchar *p2 = (const uchar *)ptrs[2];

        for (; imsize.height--; p0 += step0, p1 += step1, p2 += step2, bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++, p0 += d0, p1 += d1, p2 += d2)
            {
                size_t idx = tab[*p0] + tab[*p1 + 256] + tab[*p2 + 512];
                bproj[x] = idx < OUT_OF_RANGE ? saturate_cast<uchar>(*(float *)(H + idx) * scale) : 0;
            }
        }
    }
    else
    {
        for (; imsize.height--; bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                uchar *Hptr = H;
                for (i = 0; i < dims; i++)
                {
                    size_t idx = tab[*ptrs[i] + i * 256];
                    if (idx >= OUT_OF_RANGE)
                        break;
                    ptrs[i] += deltas[i * 2];
                    Hptr += idx;
                }

                if (i == dims)
                    bproj[x] = saturate_cast<uchar>(*(float *)Hptr * scale);
                else
                {
                    bproj[x] = 0;
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
                }
            }
            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}
}

void cv::calcBackProject(const Mat *images, int nimages, const int *channels,
                         InputArray _hist, OutputArray _backProject,
                         const float **ranges, double scale, bool uniform)
{
    Mat hist = _hist.getMat();
    vector<uchar *> ptrs;
    vector<int> deltas;
    vector<double> uniranges;
    Size imsize;
    int dims = hist.dims == 2 && hist.size[1] == 1 ? 1 : hist.dims;

    CV_Assert(dims > 0 && hist.data);
    _backProject.create(images[0].size(), images[0].depth());
    Mat backProject = _backProject.getMat();
    histPrepareImages(images, nimages, channels, backProject, dims, hist.size, ranges,
                      uniform, ptrs, deltas, imsize, uniranges);
    const double *_uniranges = uniform ? &uniranges[0] : 0;

    int depth = images[0].depth();
    if (depth == CV_8U)
        calcBackProj_8u(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, (float)scale, uniform);
    else if (depth == CV_16U)
        calcBackProj_<ushort, ushort>(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, (float)scale, uniform);
    else if (depth == CV_32F)
        calcBackProj_<float, float>(ptrs, deltas, imsize, hist, dims, ranges, _uniranges, (float)scale, uniform);
    else
        CV_Error(CV_StsUnsupportedFormat, "");
}

namespace cv
{

template <typename T, typename BT>
static void
calcSparseBackProj_(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                    Size imsize, const SparseMat &hist, int dims, const float **_ranges,
                    const double *_uniranges, float scale, bool uniform)
{
    T **ptrs = (T **)&_ptrs[0];
    const int *deltas = &_deltas[0];
    int i, x;
    BT *bproj = (BT *)_ptrs[dims];
    int bpstep = _deltas[dims * 2 + 1];
    const int *size = hist.hdr->size;
    int idx[CV_MAX_DIM];
    const SparseMat_<float> &hist_ = (const SparseMat_<float> &)hist;

    if (uniform)
    {
        const double *uniranges = &_uniranges[0];
        for (; imsize.height--; bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                for (i = 0; i < dims; i++)
                {
                    idx[i] = cvFloor(*ptrs[i] * uniranges[i * 2] + uniranges[i * 2 + 1]);
                    if ((unsigned)idx[i] >= (unsigned)size[i])
                        break;
                    ptrs[i] += deltas[i * 2];
                }

                if (i == dims)
                    bproj[x] = saturate_cast<BT>(hist_(idx) * scale);
                else
                {
                    bproj[x] = 0;
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
                }
            }
            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
    else
    {
        // non-uniform histogram
        const float *ranges[CV_MAX_DIM];
        for (i = 0; i < dims; i++)
            ranges[i] = &_ranges[i][0];

        for (; imsize.height--; bproj += bpstep)
        {
            for (x = 0; x < imsize.width; x++)
            {
                for (i = 0; i < dims; i++)
                {
                    float v = (float)*ptrs[i];
                    const float *R = ranges[i];
                    int j = -1, sz = size[i];

                    while (v >= R[j + 1] && ++j < sz)
                        ; // nop

                    if ((unsigned)j >= (unsigned)sz)
                        break;
                    idx[i] = j;
                    ptrs[i] += deltas[i * 2];
                }

                if (i == dims)
                    bproj[x] = saturate_cast<BT>(hist_(idx) * scale);
                else
                {
                    bproj[x] = 0;
                    for (; i < dims; i++)
                        ptrs[i] += deltas[i * 2];
                }
            }

            for (i = 0; i < dims; i++)
                ptrs[i] += deltas[i * 2 + 1];
        }
    }
}

static void
calcSparseBackProj_8u(vector<uchar *> &_ptrs, const vector<int> &_deltas,
                      Size imsize, const SparseMat &hist, int dims, const float **_ranges,
                      const double *_uniranges, float scale, bool uniform)
{
    uchar **ptrs = &_ptrs[0];
    const int *deltas = &_deltas[0];
    int i, x;
    uchar *bproj = _ptrs[dims];
    int bpstep = _deltas[dims * 2 + 1];
    vector<size_t> _tab;
    int idx[CV_MAX_DIM];

    calcHistLookupTables_8u(Mat(), hist, dims, _ranges, _uniranges, uniform, true, _tab);
    const size_t *tab = &_tab[0];

    for (; imsize.height--; bproj += bpstep)
    {
        for (x = 0; x < imsize.width; x++)
        {
            for (i = 0; i < dims; i++)
            {
                size_t hidx = tab[*ptrs[i] + i * 256];
                if (hidx >= OUT_OF_RANGE)
                    break;
                idx[i] = (int)hidx;
                ptrs[i] += deltas[i * 2];
            }

            if (i == dims)
                bproj[x] = saturate_cast<uchar>(hist.value<float>(idx) * scale);
            else
            {
                bproj[x] = 0;
                for (; i < dims; i++)
                    ptrs[i] += deltas[i * 2];
            }
        }
        for (i = 0; i < dims; i++)
            ptrs[i] += deltas[i * 2 + 1];
    }
}
}

void cv::calcBackProject(const Mat *images, int nimages, const int *channels,
                         const SparseMat &hist, OutputArray _backProject,
                         const float **ranges, double scale, bool uniform)
{
    vector<uchar *> ptrs;
    vector<int> deltas;
    vector<double> uniranges;
    Size imsize;
    int dims = hist.dims();

    CV_Assert(dims > 0);
    _backProject.create(images[0].size(), images[0].depth());
    Mat backProject = _backProject.getMat();
    histPrepareImages(images, nimages, channels, backProject,
                      dims, hist.hdr->size, ranges,
                      uniform, ptrs, deltas, imsize, uniranges);
    const double *_uniranges = uniform ? &uniranges[0] : 0;

    int depth = images[0].depth();
    if (depth == CV_8U)
        calcSparseBackProj_8u(ptrs, deltas, imsize, hist, dims, ranges,
                              _uniranges, (float)scale, uniform);
    else if (depth == CV_16U)
        calcSparseBackProj_<ushort, ushort>(ptrs, deltas, imsize, hist, dims, ranges,
                                            _uniranges, (float)scale, uniform);
    else if (depth == CV_32F)
        calcSparseBackProj_<float, float>(ptrs, deltas, imsize, hist, dims, ranges,
                                          _uniranges, (float)scale, uniform);
    else
        CV_Error(CV_StsUnsupportedFormat, "");
}

void cv::calcBackProject(InputArrayOfArrays images, const vector<int> &channels,
                         InputArray hist, OutputArray dst,
                         const vector<float> &ranges,
                         double scale)
{
    Mat H0 = hist.getMat(), H;
    int hcn = H0.channels();
    if (hcn > 1)
    {
        CV_Assert(H0.isContinuous());
        int hsz[CV_CN_MAX + 1];
        memcpy(hsz, &H0.size[0], H0.dims * sizeof(hsz[0]));
        hsz[H0.dims] = hcn;
        H = Mat(H0.dims + 1, hsz, H0.depth(), H0.data);
    }
    else
        H = H0;
    bool _1d = H.rows == 1 || H.cols == 1;
    int i, dims = H.dims, rsz = (int)ranges.size(), csz = (int)channels.size();
    int nimages = (int)images.total();
    CV_Assert(nimages > 0);
    CV_Assert(rsz == dims * 2 || (rsz == 2 && _1d) || (rsz == 0 && images.depth(0) == CV_8U));
    CV_Assert(csz == 0 || csz == dims || (csz == 1 && _1d));
    float *_ranges[CV_MAX_DIM];
    if (rsz > 0)
    {
        for (i = 0; i < rsz / 2; i++)
            _ranges[i] = (float *)&ranges[i * 2];
    }

    AutoBuffer<Mat> buf(nimages);
    for (i = 0; i < nimages; i++)
        buf[i] = images.getMat(i);

    calcBackProject(&buf[0], nimages, csz ? &channels[0] : 0,
                    hist, dst, rsz ? (const float **)_ranges : 0, scale, true);
}

////////////////// C O M P A R E   H I S T O G R A M S ////////////////////////

double cv::compareHist(InputArray _H1, InputArray _H2, int method)
{
    Mat H1 = _H1.getMat(), H2 = _H2.getMat();
    const Mat *arrays[] = {&H1, &H2, 0};
    Mat planes[2];
    NAryMatIterator it(arrays, planes);
    double result = 0;
    int j, len = (int)it.size;

    CV_Assert(H1.type() == H2.type() && H1.type() == CV_32F);

    double s1 = 0, s2 = 0, s11 = 0, s12 = 0, s22 = 0;

    CV_Assert(it.planes[0].isContinuous() && it.planes[1].isContinuous());

    for (size_t i = 0; i < it.nplanes; i++, ++it)
    {
        const float *h1 = (const float *)it.planes[0].data;
        const float *h2 = (const float *)it.planes[1].data;
        len = it.planes[0].rows * it.planes[0].cols;

        if (method == CV_COMP_CHISQR)
        {
            for (j = 0; j < len; j++)
            {
                double a = h1[j] - h2[j];
                double b = h1[j];
                if (fabs(b) > DBL_EPSILON)
                    result += a * a / b;
            }
        }
        else if (method == CV_COMP_CORREL)
        {
            for (j = 0; j < len; j++)
            {
                double a = h1[j];
                double b = h2[j];

                s12 += a * b;
                s1 += a;
                s11 += a * a;
                s2 += b;
                s22 += b * b;
            }
        }
        else if (method == CV_COMP_INTERSECT)
        {
            for (j = 0; j < len; j++)
                result += std::min(h1[j], h2[j]);
        }
        else if (method == CV_COMP_BHATTACHARYYA)
        {
            for (j = 0; j < len; j++)
            {
                double a = h1[j];
                double b = h2[j];
                result += std::sqrt(a * b);
                s1 += a;
                s2 += b;
            }
        }
        else
            CV_Error(CV_StsBadArg, "Unknown comparison method");
    }

    if (method == CV_COMP_CORREL)
    {
        size_t total = H1.total();
        double scale = 1. / total;
        double num = s12 - s1 * s2 * scale;
        double denom2 = (s11 - s1 * s1 * scale) * (s22 - s2 * s2 * scale);
        result = std::abs(denom2) > DBL_EPSILON ? num / std::sqrt(denom2) : 1.;
    }
    else if (method == CV_COMP_BHATTACHARYYA)
    {
        s1 *= s2;
        s1 = fabs(s1) > FLT_EPSILON ? 1. / std::sqrt(s1) : 1.;
        result = std::sqrt(std::max(1. - result * s1, 0.));
    }

    return result;
}

double cv::compareHist(const SparseMat &H1, const SparseMat &H2, int method)
{
    double result = 0;
    int i, dims = H1.dims();

    CV_Assert(dims > 0 && dims == H2.dims() && H1.type() == H2.type() && H1.type() == CV_32F);
    for (i = 0; i < dims; i++)
        CV_Assert(H1.size(i) == H2.size(i));

    const SparseMat *PH1 = &H1, *PH2 = &H2;
    if (PH1->nzcount() > PH2->nzcount() && method != CV_COMP_CHISQR)
        std::swap(PH1, PH2);

    SparseMatConstIterator it = PH1->begin();
    int N1 = (int)PH1->nzcount(), N2 = (int)PH2->nzcount();

    if (method == CV_COMP_CHISQR)
    {
        for (i = 0; i < N1; i++, ++it)
        {
            float v1 = it.value<float>();
            const SparseMat::Node *node = it.node();
            float v2 = PH2->value<float>(node->idx, (size_t *)&node->hashval);
            double a = v1 - v2;
            double b = v1;
            if (fabs(b) > DBL_EPSILON)
                result += a * a / b;
        }
    }
    else if (method == CV_COMP_CORREL)
    {
        double s1 = 0, s2 = 0, s11 = 0, s12 = 0, s22 = 0;

        for (i = 0; i < N1; i++, ++it)
        {
            double v1 = it.value<float>();
            const SparseMat::Node *node = it.node();
            s12 += v1 * PH2->value<float>(node->idx, (size_t *)&node->hashval);
            s1 += v1;
            s11 += v1 * v1;
        }

        it = PH2->begin();
        for (i = 0; i < N2; i++, ++it)
        {
            double v2 = it.value<float>();
            s2 += v2;
            s22 += v2 * v2;
        }

        size_t total = 1;
        for (i = 0; i < H1.dims(); i++)
            total *= H1.size(i);
        double scale = 1. / total;
        double num = s12 - s1 * s2 * scale;
        double denom2 = (s11 - s1 * s1 * scale) * (s22 - s2 * s2 * scale);
        result = std::abs(denom2) > DBL_EPSILON ? num / std::sqrt(denom2) : 1.;
    }
    else if (method == CV_COMP_INTERSECT)
    {
        for (i = 0; i < N1; i++, ++it)
        {
            float v1 = it.value<float>();
            const SparseMat::Node *node = it.node();
            float v2 = PH2->value<float>(node->idx, (size_t *)&node->hashval);
            if (v2)
                result += std::min(v1, v2);
        }
    }
    else if (method == CV_COMP_BHATTACHARYYA)
    {
        double s1 = 0, s2 = 0;

        for (i = 0; i < N1; i++, ++it)
        {
            double v1 = it.value<float>();
            const SparseMat::Node *node = it.node();
            double v2 = PH2->value<float>(node->idx, (size_t *)&node->hashval);
            result += std::sqrt(v1 * v2);
            s1 += v1;
        }

        it = PH2->begin();
        for (i = 0; i < N2; i++, ++it)
            s2 += it.value<float>();

        s1 *= s2;
        s1 = fabs(s1) > FLT_EPSILON ? 1. / std::sqrt(s1) : 1.;
        result = std::sqrt(std::max(1. - result * s1, 0.));
    }
    else
        CV_Error(CV_StsBadArg, "Unknown comparison method");

    return result;
}

const int CV_HIST_DEFAULT_TYPE = CV_32F;

CV_IMPL void
cvReleaseHist(CvHistogram **hist)
{
    if (!hist)
        CV_Error(CV_StsNullPtr, "");

    if (*hist)
    {
        CvHistogram *temp = *hist;

        if (!CV_IS_HIST(temp))
            CV_Error(CV_StsBadArg, "Invalid histogram header");
        *hist = 0;

        if (CV_IS_SPARSE_HIST(temp))
            cvReleaseSparseMat((CvSparseMat **)&temp->bins);
        else
        {
            cvReleaseData(temp->bins);
            temp->bins = 0;
        }

        if (temp->thresh2)
            cvFree(&temp->thresh2);
        cvFree(&temp);
    }
}

CV_IMPL void
cvClearHist(CvHistogram *hist)
{
    if (!CV_IS_HIST(hist))
        CV_Error(CV_StsBadArg, "Invalid histogram header");
    cvZero(hist->bins);
}

// Clears histogram bins that are below than threshold
CV_IMPL void
cvThreshHist(CvHistogram *hist, double thresh)
{
    if (!CV_IS_HIST(hist))
        CV_Error(CV_StsBadArg, "Invalid histogram header");

    if (!CV_IS_SPARSE_MAT(hist->bins))
    {
        CvMat mat;
        cvGetMat(hist->bins, &mat, 0, 1);
        cvThreshold(&mat, &mat, thresh, 0, CV_THRESH_TOZERO);
    }
    else
    {
        CvSparseMat *mat = (CvSparseMat *)hist->bins;
        CvSparseMatIterator iterator;
        CvSparseNode *node;

        for (node = cvInitSparseMatIterator(mat, &iterator);
             node != 0; node = cvGetNextSparseNode(&iterator))
        {
            float *val = (float *)CV_NODE_VAL(mat, node);
            if (*val <= thresh)
                *val = 0;
        }
    }
}

// Normalizes histogram (make sum of the histogram bins == factor)
CV_IMPL void
cvNormalizeHist(CvHistogram *hist, double factor)
{
    double sum = 0;

    if (!CV_IS_HIST(hist))
        CV_Error(CV_StsBadArg, "Invalid histogram header");

    if (!CV_IS_SPARSE_HIST(hist))
    {
        CvMat mat;
        cvGetMat(hist->bins, &mat, 0, 1);
        sum = cvSum(&mat).val[0];
        if (fabs(sum) < DBL_EPSILON)
            sum = 1;
        cvScale(&mat, &mat, factor / sum, 0);
    }
    else
    {
        CvSparseMat *mat = (CvSparseMat *)hist->bins;
        CvSparseMatIterator iterator;
        CvSparseNode *node;
        float scale;

        for (node = cvInitSparseMatIterator(mat, &iterator);
             node != 0; node = cvGetNextSparseNode(&iterator))
        {
            sum += *(float *)CV_NODE_VAL(mat, node);
        }

        if (fabs(sum) < DBL_EPSILON)
            sum = 1;
        scale = (float)(factor / sum);

        for (node = cvInitSparseMatIterator(mat, &iterator);
             node != 0; node = cvGetNextSparseNode(&iterator))
        {
            *(float *)CV_NODE_VAL(mat, node) *= scale;
        }
    }
}

////////////////////// B A C K   P R O J E C T   P A T C H /////////////////////////

class EqualizeHistCalcHist_Invoker : public cv::ParallelLoopBody
{
  public:
    enum
    {
        HIST_SZ = 256
    };

    EqualizeHistCalcHist_Invoker(cv::Mat &src, int *histogram, cv::Mutex *histogramLock)
        : src_(src), globalHistogram_(histogram), histogramLock_(histogramLock)
    {
    }

    void operator()(const cv::Range &rowRange) const
    {
        int localHistogram[HIST_SZ] = {
            0,
        };

        const size_t sstep = src_.step;

        int width = src_.cols;
        int height = rowRange.end - rowRange.start;

        if (src_.isContinuous())
        {
            width *= height;
            height = 1;
        }

        for (const uchar *ptr = src_.ptr<uchar>(rowRange.start); height--; ptr += sstep)
        {
            int x = 0;
            for (; x <= width - 4; x += 4)
            {
                int t0 = ptr[x], t1 = ptr[x + 1];
                localHistogram[t0]++;
                localHistogram[t1]++;
                t0 = ptr[x + 2];
                t1 = ptr[x + 3];
                localHistogram[t0]++;
                localHistogram[t1]++;
            }

            for (; x < width; ++x)
                localHistogram[ptr[x]]++;
        }

        cv::AutoLock lock(*histogramLock_);

        for (int i = 0; i < HIST_SZ; i++)
            globalHistogram_[i] += localHistogram[i];
    }

    static bool isWorthParallel(const cv::Mat &src)
    {
        return (src.total() >= 640 * 480);
    }

  private:
    EqualizeHistCalcHist_Invoker &operator=(const EqualizeHistCalcHist_Invoker &);

    cv::Mat &src_;
    int *globalHistogram_;
    cv::Mutex *histogramLock_;
};

class EqualizeHistLut_Invoker : public cv::ParallelLoopBody
{
  public:
    EqualizeHistLut_Invoker(cv::Mat &src, cv::Mat &dst, int *lut)
        : src_(src),
          dst_(dst),
          lut_(lut)
    {
    }

    void operator()(const cv::Range &rowRange) const
    {
        const size_t sstep = src_.step;
        const size_t dstep = dst_.step;

        int width = src_.cols;
        int height = rowRange.end - rowRange.start;
        int *lut = lut_;

        if (src_.isContinuous() && dst_.isContinuous())
        {
            width *= height;
            height = 1;
        }

        const uchar *sptr = src_.ptr<uchar>(rowRange.start);
        uchar *dptr = dst_.ptr<uchar>(rowRange.start);

        for (; height--; sptr += sstep, dptr += dstep)
        {
            int x = 0;
            for (; x <= width - 4; x += 4)
            {
                int v0 = sptr[x];
                int v1 = sptr[x + 1];
                int x0 = lut[v0];
                int x1 = lut[v1];
                dptr[x] = (uchar)x0;
                dptr[x + 1] = (uchar)x1;

                v0 = sptr[x + 2];
                v1 = sptr[x + 3];
                x0 = lut[v0];
                x1 = lut[v1];
                dptr[x + 2] = (uchar)x0;
                dptr[x + 3] = (uchar)x1;
            }

            for (; x < width; ++x)
                dptr[x] = (uchar)lut[sptr[x]];
        }
    }

    static bool isWorthParallel(const cv::Mat &src)
    {
        return (src.total() >= 640 * 480);
    }

  private:
    EqualizeHistLut_Invoker &operator=(const EqualizeHistLut_Invoker &);

    cv::Mat &src_;
    cv::Mat &dst_;
    int *lut_;
};

CV_IMPL void cvEqualizeHist(const CvArr *srcarr, CvArr *dstarr)
{
    cv::equalizeHist(cv::cvarrToMat(srcarr), cv::cvarrToMat(dstarr));
}

void cv::equalizeHist(InputArray _src, OutputArray _dst)
{
    Mat src = _src.getMat();
    CV_Assert(src.type() == CV_8UC1);

    _dst.create(src.size(), src.type());
    Mat dst = _dst.getMat();

    if (src.empty())
        return;

    Mutex histogramLockInstance;

    const int hist_sz = EqualizeHistCalcHist_Invoker::HIST_SZ;
    int hist[hist_sz] = {
        0,
    };
    int lut[hist_sz];

    EqualizeHistCalcHist_Invoker calcBody(src, hist, &histogramLockInstance);
    EqualizeHistLut_Invoker lutBody(src, dst, lut);
    cv::Range heightRange(0, src.rows);

    calcBody(heightRange);

    int i = 0;
    while (!hist[i])
        ++i;

    int total = (int)src.total();
    if (hist[i] == total)
    {
        dst.setTo(i);
        return;
    }

    float scale = (hist_sz - 1.f) / (total - hist[i]);
    int sum = 0;

    for (lut[i++] = 0; i < hist_sz; ++i)
    {
        sum += hist[i];
        lut[i] = saturate_cast<uchar>(sum * scale);
    }
    lutBody(heightRange);
}

// ----------------------------------------------------------------------


/* End of file. */

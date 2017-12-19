
#include "core.precomp.hpp"

namespace cv
{

template <typename _Tp>
void JacobiSVDImpl_(_Tp *At, size_t astep, _Tp *_W, _Tp *Vt, size_t vstep,
                    int m, int n, int n1, double minval, _Tp eps)
{
    AutoBuffer<double> Wbuf(n);
    double *W = Wbuf;
    int i, j, k, iter, max_iter = std::max(m, 30);
    _Tp c, s;
    double sd;
    astep /= sizeof(At[0]);
    vstep /= sizeof(Vt[0]);

    for (i = 0; i < n; i++)
    {
        for (k = 0, sd = 0; k < m; k++)
        {
            _Tp t = At[i * astep + k];
            sd += (double)t * t;
        }
        W[i] = sd;

        if (Vt)
        {
            for (k = 0; k < n; k++)
                Vt[i * vstep + k] = 0;
            Vt[i * vstep + i] = 1;
        }
    }

    for (iter = 0; iter < max_iter; iter++)
    {
        bool changed = false;

        for (i = 0; i < n - 1; i++)
            for (j = i + 1; j < n; j++)
            {
                _Tp *Ai = At + i * astep, *Aj = At + j * astep;
                double a = W[i], p = 0, b = W[j];

                for (k = 0; k < m; k++)
                    p += (double)Ai[k] * Aj[k];

                if (std::abs(p) <= eps * std::sqrt((double)a * b))
                    continue;

                p *= 2;
                double beta = a - b, gamma = hypot((double)p, beta);
                if (beta < 0)
                {
                    double delta = (gamma - beta) * 0.5;
                    s = (_Tp)std::sqrt(delta / gamma);
                    c = (_Tp)(p / (gamma * s * 2));
                }
                else
                {
                    c = (_Tp)std::sqrt((gamma + beta) / (gamma * 2));
                    s = (_Tp)(p / (gamma * c * 2));
                }

                a = b = 0;
                for (k = 0; k < m; k++)
                {
                    _Tp t0 = c * Ai[k] + s * Aj[k];
                    _Tp t1 = -s * Ai[k] + c * Aj[k];
                    Ai[k] = t0;
                    Aj[k] = t1;

                    a += (double)t0 * t0;
                    b += (double)t1 * t1;
                }
                W[i] = a;
                W[j] = b;

                changed = true;

                if (Vt)
                {
                    _Tp *Vi = Vt + i * vstep, *Vj = Vt + j * vstep;

                    for (k = 0; k < n; k++)
                    {
                        _Tp t0 = c * Vi[k] + s * Vj[k];
                        _Tp t1 = -s * Vi[k] + c * Vj[k];
                        Vi[k] = t0;
                        Vj[k] = t1;
                    }
                }
            }
        if (!changed)
            break;
    }

    for (i = 0; i < n; i++)
    {
        for (k = 0, sd = 0; k < m; k++)
        {
            _Tp t = At[i * astep + k];
            sd += (double)t * t;
        }
        W[i] = std::sqrt(sd);
    }

    for (i = 0; i < n - 1; i++)
    {
        j = i;
        for (k = i + 1; k < n; k++)
        {
            if (W[j] < W[k])
                j = k;
        }
        if (i != j)
        {
            std::swap(W[i], W[j]);
            if (Vt)
            {
                for (k = 0; k < m; k++)
                    std::swap(At[i * astep + k], At[j * astep + k]);

                for (k = 0; k < n; k++)
                    std::swap(Vt[i * vstep + k], Vt[j * vstep + k]);
            }
        }
    }

    for (i = 0; i < n; i++)
        _W[i] = (_Tp)W[i];

    if (!Vt)
        return;

    RNG rng(0x12345678);
    for (i = 0; i < n1; i++)
    {
        sd = i < n ? W[i] : 0;

        while (sd <= minval)
        {
            // if we got a zero singular value, then in order to get the corresponding left singular vector
            // we generate a random vector, project it to the previously computed left singular vectors,
            // subtract the projection and normalize the difference.
            const _Tp val0 = (_Tp)(1. / m);
            for (k = 0; k < m; k++)
            {
                _Tp val = (rng.next() & 256) != 0 ? val0 : -val0;
                At[i * astep + k] = val;
            }
            for (iter = 0; iter < 2; iter++)
            {
                for (j = 0; j < i; j++)
                {
                    sd = 0;
                    for (k = 0; k < m; k++)
                        sd += At[i * astep + k] * At[j * astep + k];
                    _Tp asum = 0;
                    for (k = 0; k < m; k++)
                    {
                        _Tp t = (_Tp)(At[i * astep + k] - sd * At[j * astep + k]);
                        At[i * astep + k] = t;
                        asum += std::abs(t);
                    }
                    asum = asum > eps * 100 ? 1 / asum : 0;
                    for (k = 0; k < m; k++)
                        At[i * astep + k] *= asum;
                }
            }
            sd = 0;
            for (k = 0; k < m; k++)
            {
                _Tp t = At[i * astep + k];
                sd += (double)t * t;
            }
            sd = std::sqrt(sd);
        }

        s = (_Tp)(1 / sd);
        for (k = 0; k < m; k++)
            At[i * astep + k] *= s;
    }
}


static void JacobiSVD(double *At, size_t astep, double *W, double *Vt, size_t vstep, int m, int n, int n1 = -1)
{
    JacobiSVDImpl_(At, astep, W, Vt, vstep, m, n, !Vt ? 0 : n1 < 0 ? n : n1, DBL_MIN, DBL_EPSILON * 10);
}

/* y[0:m,0:n] += diag(a[0:1,0:m]) * x[0:m,0:n] */
template <typename T1, typename T2, typename T3>
static void
MatrAXPY(int m, int n, const T1 *x, int dx,
         const T2 *a, int inca, T3 *y, int dy)
{
    int i, j;
    for (i = 0; i < m; i++, x += dx, y += dy)
    {
        T2 s = a[i * inca];
        j = 0;
        for (; j < n; j++)
            y[j] = (T3)(y[j] + s * x[j]);
    }
}

template <typename T>
static void
SVBkSbImpl_(int m, int n, const T *w, int incw,
            const T *u, int ldu, bool uT,
            const T *v, int ldv, bool vT,
            const T *b, int ldb, int nb,
            T *x, int ldx, double *buffer, T eps)
{
    double threshold = 0;
    int udelta0 = uT ? ldu : 1, udelta1 = uT ? 1 : ldu;
    int vdelta0 = vT ? ldv : 1, vdelta1 = vT ? 1 : ldv;
    int i, j, nm = std::min(m, n);

    if (!b)
        nb = m;

    for (i = 0; i < n; i++)
        for (j = 0; j < nb; j++)
            x[i * ldx + j] = 0;

    for (i = 0; i < nm; i++)
        threshold += w[i * incw];
    threshold *= eps;

    // v * inv(w) * uT * b
    for (i = 0; i < nm; i++, u += udelta0, v += vdelta0)
    {
        double wi = w[i * incw];
        if ((double)std::abs(wi) <= threshold)
            continue;
        wi = 1 / wi;

        if (nb == 1)
        {
            double s = 0;
            if (b)
                for (j = 0; j < m; j++)
                    s += u[j * udelta1] * b[j * ldb];
            else
                s = u[0];
            s *= wi;

            for (j = 0; j < n; j++)
                x[j * ldx] = (T)(x[j * ldx] + s * v[j * vdelta1]);
        }
        else
        {
            if (b)
            {
                for (j = 0; j < nb; j++)
                    buffer[j] = 0;
                MatrAXPY(m, nb, b, ldb, u, udelta1, buffer, 0);
                for (j = 0; j < nb; j++)
                    buffer[j] *= wi;
            }
            else
            {
                for (j = 0; j < nb; j++)
                    buffer[j] = u[j * udelta1] * wi;
            }
            MatrAXPY(n, nb, buffer, 0, v, vdelta1, x, ldx);
        }
    }
}

static void
SVBkSb(int m, int n, const double *w, size_t wstep,
       const double *u, size_t ustep, bool uT,
       const double *v, size_t vstep, bool vT,
       const double *b, size_t bstep, int nb,
       double *x, size_t xstep, uchar *buffer)
{
    SVBkSbImpl_(m, n, w, wstep ? (int)(wstep / sizeof(w[0])) : 1,
                u, (int)(ustep / sizeof(u[0])), uT,
                v, (int)(vstep / sizeof(v[0])), vT,
                b, (int)(bstep / sizeof(b[0])), nb,
                x, (int)(xstep / sizeof(x[0])),
                (double *)alignPtr(buffer, sizeof(double)), DBL_EPSILON * 2);
}
}


/****************************************************************************************\
*                              Solving a linear system                                   *
\****************************************************************************************/

bool cv::solve(InputArray _src, InputArray _src2arg, OutputArray _dst, int method)
{
    bool result = true;
    Mat src = _src.getMat(), _src2 = _src2arg.getMat();
    int type = src.type();
    bool is_normal = (method & DECOMP_NORMAL) != 0;

    CV_Assert(type == _src2.type() && (type == CV_64F));

    method &= ~DECOMP_NORMAL;
    CV_Assert(((method == DECOMP_SVD) && (!is_normal)) || (src.rows == src.cols));

    int m = src.rows, m_ = m, n = src.cols, nb = _src2.cols;
    size_t esz = CV_ELEM_SIZE(type), bufsize = 0;
    size_t vstep = alignSize(n * esz, 16);
    size_t astep = alignSize(m * esz, 16);
    AutoBuffer<uchar> buffer;

    Mat src2 = _src2;
    _dst.create(src.cols, src2.cols, src.type());
    Mat dst = _dst.getMat();

    if (m < n)
        CV_Error(CV_StsBadArg, "The function can not solve under-determined linear systems");

    if (m == n)
        is_normal = false;
    size_t asize = astep * n;
    bufsize += asize + 32;
    bufsize += n * 5 * esz + n * vstep + nb * sizeof(double) + 32;

    buffer.allocate(bufsize);
    uchar *ptr = alignPtr((uchar *)buffer, 16);

    Mat a(m_, n, type, ptr, astep);

    a = Mat(n, m_, type, ptr, astep);
    transpose(src, a);

    ptr += asize;

    {
        ptr = alignPtr(ptr, 16);
        Mat v(n, n, type, ptr, vstep), w(n, 1, type, ptr + vstep * n), u;
        ptr += n * (vstep + esz);

        JacobiSVD(a.ptr<double>(), a.step, w.ptr<double>(), v.ptr<double>(), v.step, m_, n);

        u = a;

        SVBkSb(m_, n, w.ptr<double>(), 0, u.ptr<double>(), u.step, true,
               v.ptr<double>(), v.step, true, src2.ptr<double>(),
               src2.step, nb, dst.ptr<double>(), dst.step, ptr);

        result = true;
    }

    if (!result)
        dst = Scalar(0);

    return result;
}

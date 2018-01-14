#include "core.h"
#include "misc.h"

#include <float.h>

#define iGivens_64f(n, x, y, c, s)     \
    \
{                                 \
        int _i;                        \
        double *_x = (x);              \
        double *_y = (y);              \
                                       \
        for (_i = 0; _i < n; _i++)     \
        {                              \
            double t0 = _x[_i];        \
            double t1 = _y[_i];        \
            _x[_i] = t0 * c + t1 * s;  \
            _y[_i] = -t0 * s + t1 * c; \
        }                              \
    \
}

static void iMatrAXPY_64f(int m, int n, const double *x, int dx,
              const double *a, double *y, int dy)
{
    int i, j;

    for (i = 0; i < m; i++, x += dx, y += dy)
    {
        double s = a[i];

        for (j = 0; j <= n - 4; j += 4)
        {
            double t0 = y[j] + s * x[j];
            double t1 = y[j + 1] + s * x[j + 1];
            y[j] = t0;
            y[j + 1] = t1;
            t0 = y[j + 2] + s * x[j + 2];
            t1 = y[j + 3] + s * x[j + 3];
            y[j + 2] = t0;
            y[j + 3] = t1;
        }

        for (; j < n; j++)
            y[j] += s * x[j];
    }
}

static void iMatrAXPY3_64f(int m, int n, const double *x, int l, double *y, double h)
{
    int i, j;

    for (i = 1; i < m; i++)
    {
        double s = 0;

        y += l;

        for (j = 0; j <= n - 4; j += 4)
            s += x[j] * y[j] + x[j + 1] * y[j + 1] + x[j + 2] * y[j + 2] + x[j + 3] * y[j + 3];

        for (; j < n; j++)
            s += x[j] * y[j];

        s *= h;
        y[-1] = s * x[-1];

        for (j = 0; j <= n - 4; j += 4)
        {
            double t0 = y[j] + s * x[j];
            double t1 = y[j + 1] + s * x[j + 1];
            y[j] = t0;
            y[j + 1] = t1;
            t0 = y[j + 2] + s * x[j + 2];
            t1 = y[j + 3] + s * x[j + 3];
            y[j + 2] = t0;
            y[j + 3] = t1;
        }

        for (; j < n; j++)
            y[j] += s * x[j];
    }
}

static double pythag(double a, double b)
{
    a = fabs(a);
    b = fabs(b);
    if (a > b)
    {
        b /= a;
        a *= SYS_SQRT(1. + b * b);
    }
    else if (0 != b)
    {
        a /= b;
        a = b * SYS_SQRT(1. + a * a);
    }

    return a;
}

#define MAX_ITERS 30

static void iSVD_64f(double *a, int lda, int m, int n,
         double *w,
         double *uT, int lduT, int nu,
         double *vT, int ldvT,
         double *buffer)
{
    double *e;
    double *temp;
    double *w1, *e1;
    double *hv;
    double ku0 = 0, kv0 = 0;
    double anorm = 0;
    double *a1, *u0 = uT, *v0 = vT;
    double scale, h;
    int i, j, k, l;
    int nm, m1, n1;
    int nv = n;
    int iters = 0;
    double *hv0 = (double *)sysStackAlloc((m + 2) * sizeof(hv0[0])) + 1;

    e = buffer;
    w1 = w;
    e1 = e + 1;
    nm = n;

    temp = buffer + nm;

    VOS_MEMSET(w, 0, nm * sizeof(w[0]));
    VOS_MEMSET(e, 0, nm * sizeof(e[0]));

    m1 = m;
    n1 = n;
    int update_u;
    int update_v;
    for (;;)
    {

        if (0 == m1)
            break;

        scale = h = 0;
        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for (j = 0, a1 = a; j < m1; j++, a1 += lda)
        {
            double t = a1[0];
            scale += fabs(hv[j] = t);
        }

        if (0 != scale)
        {
            double f = 1. / scale, g, s = 0;

            for (j = 0; j < m1; j++)
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt(s);
            f = hv[0];
            if (f >= 0)
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);

            VOS_MEMSET(temp, 0, n1 * sizeof(temp[0]));

            iMatrAXPY_64f(m1, n1 - 1, a + 1, lda, hv, temp + 1, 0);
            for (k = 1; k < n1; k++)
                temp[k] *= h;

            iMatrAXPY_64f(m1, n1 - 1, temp + 1, 0, hv, a + 1, lda);
            *w1 = g * scale;
        }
        w1++;

        if (update_u)
        {
            if (m1 == m)
                ku0 = h;
            else
                hv[-1] = h;
        }

        a++;
        n1--;
        if (vT)
            vT += ldvT + 1;

        if (0 == n1)
            break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;

        hv = update_v ? vT : hv0;

        for (j = 0; j < n1; j++)
        {
            double t = a[j];
            scale += fabs(hv[j] = t);
        }

        if (0 != scale)
        {
            double f = 1. / scale, g, s = 0;

            for (j = 0; j < n1; j++)
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt(s);
            f = hv[0];
            if (0 <= f)
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);
            hv[-1] = 0.;

            /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
            iMatrAXPY3_64f(m1, n1, hv, lda, a, h);

            *e1 = g * scale;
        }
        e1++;

        /* store -2/(hv'*hv) */
        if (update_v)
        {
            if (n1 == n)
                kv0 = h;
            else
                hv[-1] = h;
        }

        a += lda;
        m1--;
        if (uT)
            uT += lduT + 1;
    }

    m1 -= m1 != 0;
    n1 -= n1 != 0;

    /* accumulate left transformations */
    if (uT)
    {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for (i = m1; i < nu; i++, uT += lduT)
        {
            VOS_MEMSET(uT + m1, 0, (m - m1) * sizeof(uT[0]));
            uT[i] = 1.;
        }

        for (i = m1 - 1; i >= 0; i--)
        {
            double s;
            int lh = nu - i;

            l = m - i;

            hv = u0 + (lduT + 1) * i;
            h = i == 0 ? ku0 : hv[-1];

            assert(h <= 0);

            if (0 != h)
            {
                uT = hv;
                iMatrAXPY3_64f(lh, l - 1, hv + 1, lduT, uT + 1, h);

                s = hv[0] * h;
                for (k = 0; k < l; k++)
                    hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for (j = 1; j < l; j++)
                    hv[j] = 0;
                for (j = 1; j < lh; j++)
                    hv[j * lduT] = 0;
                hv[0] = 1;
            }
        }
        uT = u0;
    }

    /* accumulate right transformations */
    if (vT)
    {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for (i = n1; i < nv; i++, vT += ldvT)
        {
            VOS_MEMSET(vT + n1, 0, (n - n1) * sizeof(vT[0]));
            vT[i] = 1.;
        }

        for (i = n1 - 1; i >= 0; i--)
        {
            double s;
            int lh = nv - i;

            l = n - i;
            hv = v0 + (ldvT + 1) * i;
            h = i == 0 ? kv0 : hv[-1];

            assert(h <= 0);

            if (0 != h)
            {
                vT = hv;
                iMatrAXPY3_64f(lh, l - 1, hv + 1, ldvT, vT + 1, h);

                s = hv[0] * h;
                for (k = 0; k < l; k++)
                    hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for (j = 1; j < l; j++)
                    hv[j] = 0;
                for (j = 1; j < lh; j++)
                    hv[j * ldvT] = 0;
                hv[0] = 1;
            }
        }
        vT = v0;
    }

    for (i = 0; i < nm; i++)
    {
        double tnorm = fabs(w[i]);
        tnorm += fabs(e[i]);

        if (anorm < tnorm)
            anorm = tnorm;
    }

    anorm *= DBL_EPSILON;

    /* diagonalization of the bidiagonal form */
    for (k = nm - 1; k >= 0; k--)
    {
        double z = 0;
        iters = 0;

        for (;;) /* do iterations */
        {
            double c, s, f, g, x, y;
            int flag = 0;

            /* test for splitting */
            for (l = k; l >= 0; l--)
            {
                if (fabs(e[l]) <= anorm)
                {
                    flag = 1;
                    break;
                }
                assert(l > 0);
                if (fabs(w[l - 1]) <= anorm)
                    break;
            }

            if (!flag)
            {
                c = 0;
                s = 1;

                for (i = l; i <= k; i++)
                {
                    f = s * e[i];

                    e[i] *= c;

                    if (anorm + fabs(f) == anorm)
                        break;

                    g = w[i];
                    h = pythag(f, g);
                    w[i] = h;
                    c = g / h;
                    s = -f / h;

                    if (uT)
                        iGivens_64f(m, uT + lduT * (l - 1), uT + lduT * i, c, s);
                }
            }

            z = w[k];
            if (l == k || iters++ == MAX_ITERS)
                break;

            /* shift from bottom 2x2 minor */
            x = w[l];
            y = w[k - 1];
            g = e[k - 1];
            h = e[k];
            f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
            g = pythag(f, 1);
            if (f < 0)
                g = -g;
            f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
            /* next QR transformation */
            c = s = 1;

            for (i = l + 1; i <= k; i++)
            {
                g = e[i];
                y = w[i];
                h = s * g;
                g *= c;
                z = pythag(f, h);
                e[i - 1] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;

                if (vT)
                    iGivens_64f(n, vT + ldvT * (i - 1), vT + ldvT * i, c, s);

                z = pythag(f, h);
                w[i - 1] = z;

                /* rotation can be arbitrary if z == 0 */
                if (0 != z)
                {
                    c = f / z;
                    s = h / z;
                }
                f = c * g + s * y;
                x = -s * g + c * y;

                if (uT)
                    iGivens_64f(m, uT + lduT * (i - 1), uT + lduT * i, c, s);
            }

            e[l] = 0;
            e[k] = f;
            w[k] = x;
        } /* end of iteration loop */

        if (iters > MAX_ITERS)
            break;

        if (z < 0)
        {
            w[k] = -z;
            if (vT)
            {
                for (j = 0; j < n; j++)
                    vT[j + k * ldvT] = -vT[j + k * ldvT];
            }
        }
    } /* end of diagonalization loop */

    /* sort singular values and corresponding values */
    for (i = 0; i < nm; i++)
    {
        k = i;
        for (j = i + 1; j < nm; j++)
            if (w[k] < w[j])
                k = j;

        if (k != i)
        {
            double t;
            VOS_SWAP(w[i], w[k], t);

            if (vT)
                for (j = 0; j < n; j++)
                    VOS_SWAP(vT[j + ldvT * k], vT[j + ldvT * i], t);

            if (uT)
                for (j = 0; j < m; j++)
                    VOS_SWAP(uT[j + lduT * k], uT[j + lduT * i], t);
        }
    }
}

static void iSVBkSb_64f(int m, int n, const double *w,
            const double *uT, int lduT,
            const double *vT, int ldvT,
            const double *b, int ldb, int nb,
            double *x, int ldx, double *buffer)
{
    double threshold = 0;
    int i, j, nm = VOS_MIN(m, n);

    if (!b)
        nb = m;

    for (i = 0; i < n; i++)
        VOS_MEMSET(x + i * ldx, 0, nb * sizeof(x[0]));

    for (i = 0; i < nm; i++)
        threshold += w[i];
    threshold *= 2 * DBL_EPSILON;

    /* vT * inv(w) * uT * b */
    for (i = 0; i < nm; i++, uT += lduT, vT += ldvT)
    {
        double wi = w[i];

        if (wi > threshold)
        {
            wi = 1. / wi;

            if (nb == 1)
            {
                double s = 0;
                if (b)
                {
                    if (1 == ldb)
                    {
                        for (j = 0; j <= m - 4; j += 4)
                            s += uT[j] * b[j] + uT[j + 1] * b[j + 1] + uT[j + 2] * b[j + 2] + uT[j + 3] * b[j + 3];
                        for (; j < m; j++)
                            s += uT[j] * b[j];
                    }
                    else
                    {
                        for (j = 0; j < m; j++)
                            s += uT[j] * b[j * ldb];
                    }
                }
                else
                    s = uT[0];
                s *= wi;
                if (1 == ldx)
                {
                    for (j = 0; j <= n - 4; j += 4)
                    {
                        double t0 = x[j] + s * vT[j];
                        double t1 = x[j + 1] + s * vT[j + 1];
                        x[j] = t0;
                        x[j + 1] = t1;
                        t0 = x[j + 2] + s * vT[j + 2];
                        t1 = x[j + 3] + s * vT[j + 3];
                        x[j + 2] = t0;
                        x[j + 3] = t1;
                    }

                    for (; j < n; j++)
                        x[j] += s * vT[j];
                }
                else
                {
                    for (j = 0; j < n; j++)
                        x[j * ldx] += s * vT[j];
                }
            }
            else
            {
                if (b)
                {
                    VOS_MEMSET(buffer, 0, nb * sizeof(buffer[0]));
                    iMatrAXPY_64f(m, nb, b, ldb, uT, buffer, 0);
                    for (j = 0; j < nb; j++)
                        buffer[j] *= wi;
                }
                else
                {
                    for (j = 0; j < nb; j++)
                        buffer[j] = uT[j] * wi;
                }
                iMatrAXPY_64f(n, nb, buffer, 0, vT, x, ldx);
            }
        }
    }
}

void SVD(VOID *aarr, VOID *warr, VOID *uarr, VOID *varr, int flags)
{
    uchar *buffer = NULL;
    int local_alloc = 0;

    VOS_FUNCNAME("SVD");

    __BEGIN__;

    Mat astub, *a = (Mat *)aarr;
    Mat wstub, *w = (Mat *)warr;
    Mat ustub, *u;
    Mat vstub, *v;
    Mat tmat;
    uchar *tw = NULL;
    int type;
    int a_buf_offset = 0, u_buf_offset = 0, buf_size, pix_size;
    int temp_u = 0, /* temporary storage for U is needed */
        t_svd;      /* special case: a->rows < a->cols */
    int m, n;
    int w_rows, w_cols;
    int u_rows = 0, u_cols = 0;
    int w_is_mat = 0;

    if (!VOS_IS_MAT(a))
        VOS_CALL(a = GetMat(a, &astub));

    if (!VOS_IS_MAT(w))
        VOS_CALL(w = GetMat(w, &wstub));

    if (!VOS_ARE_TYPES_EQ(a, w))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    if ((VOS_SVD_U_T | VOS_SVD_V_T) != flags)
    {
        VOS_ERROR(VOS_StsUnsupportedFormat, "");
    }
    if (a->rows >= a->cols)
    {
        m = a->rows;
        n = a->cols;
        w_rows = w->rows;
        w_cols = w->cols;
        t_svd = 0;
    }
    else
    {
        VOID *t;
        VOS_SWAP(uarr, varr, t);
        m = a->cols;
        n = a->rows;
        w_rows = w->cols;
        w_cols = w->rows;
        t_svd = 1;
    }

    u = (Mat *)uarr;
    v = (Mat *)varr;

    w_is_mat = w_cols > 1 && w_rows > 1;

    tw = w->data.ptr;

    if (u)
    {
        if (!VOS_IS_MAT(u))
        {
            VOS_CALL(u = GetMat(u, &ustub));
        }

        u_rows = u->cols;
        u_cols = u->rows;

        if (!VOS_ARE_TYPES_EQ(a, u))
            VOS_ERROR(VOS_StsUnmatchedFormats, "");

        if (u_rows != m || (u_cols != m && u_cols != n))
            VOS_ERROR(VOS_StsUnmatchedSizes, "");

        temp_u = (u_rows != u_cols) || (u->data.ptr == a->data.ptr);

        if (w_is_mat && u_cols != w_rows)
            VOS_ERROR(VOS_StsUnmatchedSizes, "");
    }
    else
    {
        u = &ustub;
        u->data.ptr = NULL;
        u->step = 0;
    }

    if (v)
    {
        int v_rows, v_cols;

        if (!VOS_IS_MAT(v))
            VOS_CALL(v = GetMat(v, &vstub));

        v_rows = v->cols;
        v_cols = v->rows;

        if (!VOS_ARE_TYPES_EQ(a, v))
            VOS_ERROR(VOS_StsUnmatchedFormats, "");

        if (v_rows != n || v_cols != n)
            VOS_ERROR(VOS_StsUnmatchedSizes, "");

        if (w_is_mat && w_cols != v_cols)
            VOS_ERROR(VOS_StsUnmatchedSizes, "");
    }
    else
    {
        v = &vstub;
        v->data.ptr = NULL;
        v->step = 0;
    }

    type = VOS_MAT_TYPE(a->type);
    pix_size = VOS_ELEM_SIZE(type);
    buf_size = n * 2 + m;

    a_buf_offset = buf_size;
    buf_size += a->rows * a->cols;

    if (temp_u)
    {
        u_buf_offset = buf_size;
        buf_size += u->rows * u->cols;
    }

    buf_size *= pix_size;

    if (VOS_MAX_LOCAL_SIZE >= buf_size)
    {
        buffer = (uchar *)sysStackAlloc(buf_size);
        local_alloc = 1;
    }
    else
    {
        VOS_CALL(buffer = (uchar *)SysAlloc(buf_size));
    }

    InitMatHeader(&tmat, m, n, type,
                  buffer + a_buf_offset * pix_size);
    if (!t_svd)
        Copy(a, &tmat);
    else
    {
        VOS_ERROR(VOS_StsUnmatchedFormats, "");
    }

    a = &tmat;

    if (temp_u)
    {
        InitMatHeader(&ustub, u_cols, u_rows, type, buffer + u_buf_offset * pix_size);
        u = &ustub;
    }

    if (NULL == tw)
    {
        tw = buffer + (n + m) * pix_size;
    }

    iSVD_64f(a->data.db, a->step / sizeof(double), a->rows, a->cols,
             (double *)tw, u->data.db, u->step / sizeof(double), u_cols,
             v->data.db, v->step / sizeof(double), (double *)buffer);

    if (uarr)
    {
        if (temp_u)
            Copy(u, uarr);
    }

    __END__;

    if (buffer && !local_alloc)
        SYS_FREE(&buffer);
}

void SVBkSb(const VOID *warr, const VOID *uarr,
            const VOID *varr, const VOID *barr,
            VOID *xarr, int flags)
{
    uchar *buffer = NULL;
    int local_alloc = 0;

    VOS_FUNCNAME("SVBkSb");

    __BEGIN__;

    Mat wstub, *w = (Mat *)warr;
    Mat bstub, *b = (Mat *)barr;
    Mat xstub, *x = (Mat *)xarr;
    Mat ustub, *u = (Mat *)uarr;
    Mat vstub, *v = (Mat *)varr;
    uchar *tw = NULL;
    int type;
    int w_buf_offset = 0, t_buf_offset = 0;
    int buf_size = 0, pix_size;
    int m, n, nm;
    int u_rows, u_cols;
    int v_rows, v_cols;

    if (!VOS_IS_MAT(w))
        VOS_CALL(w = GetMat(w, &wstub));

    if (!VOS_IS_MAT(u))
        VOS_CALL(u = GetMat(u, &ustub));

    if (!VOS_IS_MAT(v))
        VOS_CALL(v = GetMat(v, &vstub));

    if (!VOS_IS_MAT(x))
        VOS_CALL(x = GetMat(x, &xstub));

    if (!VOS_ARE_TYPES_EQ(w, u) || !VOS_ARE_TYPES_EQ(w, v) || !VOS_ARE_TYPES_EQ(w, x))
        VOS_ERROR(VOS_StsUnmatchedFormats, "");

    if ((VOS_SVD_U_T | VOS_SVD_V_T) != flags)
    {
        VOS_ERROR(VOS_StsUnsupportedFormat, "");
    }

    type = VOS_MAT_TYPE(w->type);
    pix_size = VOS_ELEM_SIZE(type);

    u_rows = u->cols;
    u_cols = u->rows;

    v_rows = v->cols;
    v_cols = v->rows;

    m = u_rows;
    n = v_rows;
    nm = VOS_MIN(n, m);

    if ((u_rows != u_cols && v_rows != v_cols) || x->rows != v_rows)
        VOS_ERROR(VOS_StsBadSize, "");

    if ((1 == w->rows || 1 == w->cols) && w->rows + w->cols - 1 == nm)
    {
        if (VOS_IS_MAT_CONT(w->type))
            tw = w->data.ptr;
        else
        {
            w_buf_offset = buf_size;
            buf_size += nm * pix_size;
        }
    }
    else
    {
        if (w->cols != v_cols || w->rows != u_cols)
            VOS_ERROR(VOS_StsBadSize, "");
        w_buf_offset = buf_size;
        buf_size += nm * pix_size;
    }

    if (b)
    {
        if (!VOS_IS_MAT(b))
            VOS_CALL(b = GetMat(b, &bstub));
        if (!VOS_ARE_TYPES_EQ(w, b))
            VOS_ERROR(VOS_StsUnmatchedFormats, "");
        if (b->cols != x->cols || b->rows != m)
            VOS_ERROR(VOS_StsUnmatchedSizes, "");
    }
    else
    {
        b = &bstub;
        VOS_MEMSET(b, 0, sizeof(*b));
    }

    t_buf_offset = buf_size;
    buf_size += (VOS_MAX(m, n) + b->cols) * pix_size;

    if (VOS_MAX_LOCAL_SIZE >= buf_size)
    {
        buffer = (uchar *)sysStackAlloc(buf_size);
        local_alloc = 1;
    }
    else
        VOS_CALL(buffer = (uchar *)SysAlloc(buf_size));

    if (!tw)
    {
        int i, shift = w->cols > 1 ? pix_size : 0;
        tw = buffer + w_buf_offset;
        for (i = 0; i < nm; i++)
            VOS_MEMCPY(tw + i * pix_size, w->data.ptr + i * (w->step + shift), pix_size);
    }

    iSVBkSb_64f(m, n, (double *)tw, u->data.db, u->step / sizeof(double),
                v->data.db, v->step / sizeof(double),
                b->data.db, b->step / sizeof(double), b->cols,
                x->data.db, x->step / sizeof(double),
                (double *)(buffer + t_buf_offset));

    __END__;

    if (buffer && !local_alloc)
        SYS_FREE(&buffer);
}

/* End of file. */


#include "_cv.h"

 double
ArcLength(const void *array, Slice slice, int is_closed)
{
    double perimeter = 0;

    VOS_FUNCNAME("ArcLength");

    __BEGIN__;

    int i, j = 0, count;
    const int N = 16;
    float buf[N];
    Mat buffer = InitMat(1, N, VOS_32F, buf);
    SeqReader reader;
    Seq *contour = 0;

    if (VOS_IS_SEQ(array))
    {
        contour = (Seq *)array;
        if (!VOS_IS_SEQ_POLYLINE(contour))
            VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
        if (is_closed < 0)
            is_closed = VOS_IS_SEQ_CLOSED(contour);
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
    }

    if (contour->total > 1)
    {
        int is_float = VOS_SEQ_ELTYPE(contour) == VOS_32FC2;

        StartReadSeq(contour, &reader, 0);
        SetSeqReaderPos(&reader, slice.start_index);
        count = SliceLength(slice, contour);

        count -= !is_closed && count == contour->total;

        /* scroll the reader by 1 point */
        reader.prev_elem = reader.ptr;
        VOS_NEXT_SEQ_ELEM(sizeof(Point), reader);

        for (i = 0; i < count; i++)
        {
            float dx, dy;

            if (!is_float)
            {
                Point *pt = (Point *)reader.ptr;
                Point *prev_pt = (Point *)reader.prev_elem;

                dx = (float)pt->x - (float)prev_pt->x;
                dy = (float)pt->y - (float)prev_pt->y;
            }
            else
            {
                Point2D32f *pt = (Point2D32f *)reader.ptr;
                Point2D32f *prev_pt = (Point2D32f *)reader.prev_elem;

                dx = pt->x - prev_pt->x;
                dy = pt->y - prev_pt->y;
            }

            reader.prev_elem = reader.ptr;
            VOS_NEXT_SEQ_ELEM(contour->elem_size, reader);

            buffer.data.fl[j] = dx * dx + dy * dy;
            if (++j == N || i == count - 1)
            {
                buffer.cols = j;
                SysPow(&buffer, &buffer, 0.5);
                for (; j > 0; j--)
                    perimeter += buffer.data.fl[j - 1];
            }
        }
    }

    __END__;

    return perimeter;
}

/* area of a whole sequence */
static CvStatus
iContourArea(const Seq *contour, double *area)
{
    if (contour->total)
    {
        SeqReader reader;
        int lpt = contour->total;
        double a00 = 0, xi_1, yi_1;
        int is_float = VOS_SEQ_ELTYPE(contour) == VOS_32FC2;

        StartReadSeq(contour, &reader, 0);

        if (!is_float)
        {
            xi_1 = ((Point *)(reader.ptr))->x;
            yi_1 = ((Point *)(reader.ptr))->y;
        }
        else
        {
            xi_1 = ((Point2D32f *)(reader.ptr))->x;
            yi_1 = ((Point2D32f *)(reader.ptr))->y;
        }
        VOS_NEXT_SEQ_ELEM(contour->elem_size, reader);

        while (lpt-- > 0)
        {
            double dxy, xi, yi;

            if (!is_float)
            {
                xi = ((Point *)(reader.ptr))->x;
                yi = ((Point *)(reader.ptr))->y;
            }
            else
            {
                xi = ((Point2D32f *)(reader.ptr))->x;
                yi = ((Point2D32f *)(reader.ptr))->y;
            }
            VOS_NEXT_SEQ_ELEM(contour->elem_size, reader);

            dxy = xi_1 * yi - xi * yi_1;
            a00 += dxy;
            xi_1 = xi;
            yi_1 = yi;
        }

        *area = a00 * 0.5;
    }
    else
        *area = 0;

    return VOS_OK;
}

/****************************************************************************************\

 copy data from one buffer to other buffer

\****************************************************************************************/

static CvStatus
iMemCopy(double **buf1, double **buf2, double **buf3, int *b_max)
{
    int bb;

    if (*buf1 == NULL && *buf2 == NULL || *buf3 == NULL)
        return VOS_NULLPTR_ERR;

    bb = *b_max;
    if (*buf2 == NULL)
    {
        *b_max = 2 * (*b_max);
        *buf2 = (double *)SysAlloc((*b_max) * sizeof(double));

        if (*buf2 == NULL)
            return VOS_OUTOFMEM_ERR;

        memcpy(*buf2, *buf3, bb * sizeof(double));

        *buf3 = *buf2;
        SYS_FREE(buf1);
        *buf1 = NULL;
    }
    else
    {
        *b_max = 2 * (*b_max);
        *buf1 = (double *)SysAlloc((*b_max) * sizeof(double));

        if (*buf1 == NULL)
            return VOS_OUTOFMEM_ERR;

        memcpy(*buf1, *buf3, bb * sizeof(double));

        *buf3 = *buf1;
        SYS_FREE(buf2);
        *buf2 = NULL;
    }
    return VOS_OK;
}

/* area of a contour sector */
static CvStatus iContourSecArea(Seq *contour, Slice slice, double *area)
{
    Point pt;         /*  pointer to points   */
    Point pt_s, pt_e; /*  first and last points  */
    SeqReader reader; /*  points reader of contour   */

    int p_max = 2, p_ind;
    int lpt, flag, i;
    double a00; /* unnormalized moments m00    */
    double xi, yi, xi_1, yi_1, x0, y0, dxy, sk, sk1, t;
    double x_s, y_s, nx, ny, dx, dy, du, dv;
    double eps = 1.e-5;
    double *p_are1, *p_are2, *p_are;

    assert(contour != NULL);

    if (contour == NULL)
        return VOS_NULLPTR_ERR;

    if (!VOS_IS_SEQ_POLYGON(contour))
        return VOS_BADFLAG_ERR;

    lpt = SliceLength(slice, contour);
    /*if( n2 >= n1 )
        lpt = n2 - n1 + 1;
    else
        lpt = contour->total - n1 + n2 + 1;*/

    if (contour->total && lpt > 2)
    {
        a00 = x0 = y0 = xi_1 = yi_1 = 0;
        sk1 = 0;
        flag = 0;
        dxy = 0;
        p_are1 = (double *)SysAlloc(p_max * sizeof(double));

        if (p_are1 == NULL)
            return VOS_OUTOFMEM_ERR;

        p_are = p_are1;
        p_are2 = NULL;

        StartReadSeq(contour, &reader, 0);
        SetSeqReaderPos(&reader, slice.start_index);
        VOS_READ_SEQ_ELEM(pt_s, reader);
        p_ind = 0;
        SetSeqReaderPos(&reader, slice.end_index);
        VOS_READ_SEQ_ELEM(pt_e, reader);

        /*    normal coefficients    */
        nx = pt_s.y - pt_e.y;
        ny = pt_e.x - pt_s.x;
        SetSeqReaderPos(&reader, slice.start_index);

        while (lpt-- > 0)
        {
            VOS_READ_SEQ_ELEM(pt, reader);

            if (flag == 0)
            {
                xi_1 = (double)pt.x;
                yi_1 = (double)pt.y;
                x0 = xi_1;
                y0 = yi_1;
                sk1 = 0;
                flag = 1;
            }
            else
            {
                xi = (double)pt.x;
                yi = (double)pt.y;

                /****************   edges intersection examination   **************************/
                sk = nx * (xi - pt_s.x) + ny * (yi - pt_s.y);
                if (fabs(sk) < eps && lpt > 0 || sk * sk1 < -eps)
                {
                    if (fabs(sk) < eps)
                    {
                        dxy = xi_1 * yi - xi * yi_1;
                        a00 = a00 + dxy;
                        dxy = xi * y0 - x0 * yi;
                        a00 = a00 + dxy;

                        if (p_ind >= p_max)
                            iMemCopy(&p_are1, &p_are2, &p_are, &p_max);

                        p_are[p_ind] = a00 / 2.;
                        p_ind++;
                        a00 = 0;
                        sk1 = 0;
                        x0 = xi;
                        y0 = yi;
                        dxy = 0;
                    }
                    else
                    {
                        /*  define intersection point    */
                        dv = yi - yi_1;
                        du = xi - xi_1;
                        dx = ny;
                        dy = -nx;
                        if (fabs(du) > eps)
                            t = ((yi_1 - pt_s.y) * du + dv * (pt_s.x - xi_1)) /
                                (du * dy - dx * dv);
                        else
                            t = (xi_1 - pt_s.x) / dx;
                        if (t > eps && t < 1 - eps)
                        {
                            x_s = pt_s.x + t * dx;
                            y_s = pt_s.y + t * dy;
                            dxy = xi_1 * y_s - x_s * yi_1;
                            a00 += dxy;
                            dxy = x_s * y0 - x0 * y_s;
                            a00 += dxy;
                            if (p_ind >= p_max)
                                iMemCopy(&p_are1, &p_are2, &p_are, &p_max);

                            p_are[p_ind] = a00 / 2.;
                            p_ind++;

                            a00 = 0;
                            sk1 = 0;
                            x0 = x_s;
                            y0 = y_s;
                            dxy = x_s * yi - xi * y_s;
                        }
                    }
                }
                else
                    dxy = xi_1 * yi - xi * yi_1;

                a00 += dxy;
                xi_1 = xi;
                yi_1 = yi;
                sk1 = sk;
            }
        }

        xi = x0;
        yi = y0;
        dxy = xi_1 * yi - xi * yi_1;

        a00 += dxy;

        if (p_ind >= p_max)
            iMemCopy(&p_are1, &p_are2, &p_are, &p_max);

        p_are[p_ind] = a00 / 2.;
        p_ind++;

        /*     common area calculation    */
        *area = 0;
        for (i = 0; i < p_ind; i++)
            (*area) += fabs(p_are[i]);

        if (p_are1 != NULL)
            SYS_FREE(&p_are1);
        else if (p_are2 != NULL)
            SYS_FREE(&p_are2);

        return VOS_OK;
    }
    else
        return VOS_BADSIZE_ERR;
}

/* external contour area function */
 double
ContourArea(const void *array, Slice slice)
{
    double area = 0;

    VOS_FUNCNAME("ContourArea");

    __BEGIN__;

    Seq *contour = 0;
    if (VOS_IS_SEQ(array))
    {
        contour = (Seq *)array;
        if (!VOS_IS_SEQ_POLYLINE(contour))
            VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
    }

    if (SliceLength(slice, contour) == contour->total)
    {
        FUN_CALL(iContourArea(contour, &area));
    }
    else
    {
        if (VOS_SEQ_ELTYPE(contour) != VOS_32SC2)
            VOS_ERROR(VOS_StsUnsupportedFormat,
                     "Only curves with integer coordinates are supported in case of contour slice");
        FUN_CALL(iContourSecArea(contour, slice, &area));
    }

    __END__;

    return area;
}

/* Calculates bounding rectagnle of a point set or retrieves already calculated */
 Rect
BoundingRect(CvArr *array, int update)
{
    SeqReader reader;
    Rect rect = {0, 0, 0, 0};
    Seq *ptseq = 0;

    VOS_FUNCNAME("BoundingRect");

    __BEGIN__;

    int xmin = 0, ymin = 0, xmax = -1, ymax = -1, i;
    int calculate = update;

    if (VOS_IS_SEQ(array))
    {
        ptseq = (Seq *)array;
        if (!VOS_IS_SEQ_POINT_SET(ptseq))
            VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");

        if (ptseq->header_size < (int)sizeof(CvContour))
        {
            /*if( update == 1 )
                VOS_ERROR( VOS_StsBadArg, "The header is too small to fit the rectangle, "
                                        "so it could not be updated" );*/
            update = 0;
            calculate = 1;
        }
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
    }

    if (!calculate)
    {
        rect = ((CvContour *)ptseq)->rect;
        EXIT;
    }

    if (ptseq->total)
    {
        int is_float = VOS_SEQ_ELTYPE(ptseq) == VOS_32FC2;
        StartReadSeq(ptseq, &reader, 0);
        assert(!is_float);

        Point pt;
        /* init values */
        VOS_READ_SEQ_ELEM(pt, reader);
        xmin = xmax = pt.x;
        ymin = ymax = pt.y;

        for (i = 1; i < ptseq->total; i++)
        {
            VOS_READ_SEQ_ELEM(pt, reader);

            if (xmin > pt.x)
                xmin = pt.x;

            if (xmax < pt.x)
                xmax = pt.x;

            if (ymin > pt.y)
                ymin = pt.y;

            if (ymax < pt.y)
                ymax = pt.y;
        }
    }

    rect.x = xmin;
    rect.y = ymin;
    rect.width = xmax - xmin + 1;
    rect.height = ymax - ymin + 1;

    if (update)
        ((CvContour *)ptseq)->rect = rect;

    __END__;

    return rect;
}

/* End of file. */

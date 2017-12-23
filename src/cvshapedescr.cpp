
#include "_cv.h"

CV_IMPL double
cvArcLength(const void *array, CvSlice slice, int is_closed)
{
    double perimeter = 0;

    CV_FUNCNAME("cvArcLength");

    __BEGIN__;

    int i, j = 0, count;
    const int N = 16;
    float buf[N];
    CvMat buffer = cvMat(1, N, CV_32F, buf);
    CvSeqReader reader;
    CvSeq *contour = 0;

    if (CV_IS_SEQ(array))
    {
        contour = (CvSeq *)array;
        if (!CV_IS_SEQ_POLYLINE(contour))
            CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
        if (is_closed < 0)
            is_closed = CV_IS_SEQ_CLOSED(contour);
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
    }

    if (contour->total > 1)
    {
        int is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;

        cvStartReadSeq(contour, &reader, 0);
        cvSetSeqReaderPos(&reader, slice.start_index);
        count = cvSliceLength(slice, contour);

        count -= !is_closed && count == contour->total;

        /* scroll the reader by 1 point */
        reader.prev_elem = reader.ptr;
        CV_NEXT_SEQ_ELEM(sizeof(CvPoint), reader);

        for (i = 0; i < count; i++)
        {
            float dx, dy;

            if (!is_float)
            {
                CvPoint *pt = (CvPoint *)reader.ptr;
                CvPoint *prev_pt = (CvPoint *)reader.prev_elem;

                dx = (float)pt->x - (float)prev_pt->x;
                dy = (float)pt->y - (float)prev_pt->y;
            }
            else
            {
                CvPoint2D32f *pt = (CvPoint2D32f *)reader.ptr;
                CvPoint2D32f *prev_pt = (CvPoint2D32f *)reader.prev_elem;

                dx = pt->x - prev_pt->x;
                dy = pt->y - prev_pt->y;
            }

            reader.prev_elem = reader.ptr;
            CV_NEXT_SEQ_ELEM(contour->elem_size, reader);

            buffer.data.fl[j] = dx * dx + dy * dy;
            if (++j == N || i == count - 1)
            {
                buffer.cols = j;
                cvPow(&buffer, &buffer, 0.5);
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
icvContourArea(const CvSeq *contour, double *area)
{
    if (contour->total)
    {
        CvSeqReader reader;
        int lpt = contour->total;
        double a00 = 0, xi_1, yi_1;
        int is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;

        cvStartReadSeq(contour, &reader, 0);

        if (!is_float)
        {
            xi_1 = ((CvPoint *)(reader.ptr))->x;
            yi_1 = ((CvPoint *)(reader.ptr))->y;
        }
        else
        {
            xi_1 = ((CvPoint2D32f *)(reader.ptr))->x;
            yi_1 = ((CvPoint2D32f *)(reader.ptr))->y;
        }
        CV_NEXT_SEQ_ELEM(contour->elem_size, reader);

        while (lpt-- > 0)
        {
            double dxy, xi, yi;

            if (!is_float)
            {
                xi = ((CvPoint *)(reader.ptr))->x;
                yi = ((CvPoint *)(reader.ptr))->y;
            }
            else
            {
                xi = ((CvPoint2D32f *)(reader.ptr))->x;
                yi = ((CvPoint2D32f *)(reader.ptr))->y;
            }
            CV_NEXT_SEQ_ELEM(contour->elem_size, reader);

            dxy = xi_1 * yi - xi * yi_1;
            a00 += dxy;
            xi_1 = xi;
            yi_1 = yi;
        }

        *area = a00 * 0.5;
    }
    else
        *area = 0;

    return CV_OK;
}

/****************************************************************************************\

 copy data from one buffer to other buffer

\****************************************************************************************/

static CvStatus
icvMemCopy(double **buf1, double **buf2, double **buf3, int *b_max)
{
    int bb;

    if (*buf1 == NULL && *buf2 == NULL || *buf3 == NULL)
        return CV_NULLPTR_ERR;

    bb = *b_max;
    if (*buf2 == NULL)
    {
        *b_max = 2 * (*b_max);
        *buf2 = (double *)cvAlloc((*b_max) * sizeof(double));

        if (*buf2 == NULL)
            return CV_OUTOFMEM_ERR;

        memcpy(*buf2, *buf3, bb * sizeof(double));

        *buf3 = *buf2;
        cvFree(buf1);
        *buf1 = NULL;
    }
    else
    {
        *b_max = 2 * (*b_max);
        *buf1 = (double *)cvAlloc((*b_max) * sizeof(double));

        if (*buf1 == NULL)
            return CV_OUTOFMEM_ERR;

        memcpy(*buf1, *buf3, bb * sizeof(double));

        *buf3 = *buf1;
        cvFree(buf2);
        *buf2 = NULL;
    }
    return CV_OK;
}

/* area of a contour sector */
static CvStatus icvContourSecArea(CvSeq *contour, CvSlice slice, double *area)
{
    CvPoint pt;         /*  pointer to points   */
    CvPoint pt_s, pt_e; /*  first and last points  */
    CvSeqReader reader; /*  points reader of contour   */

    int p_max = 2, p_ind;
    int lpt, flag, i;
    double a00; /* unnormalized moments m00    */
    double xi, yi, xi_1, yi_1, x0, y0, dxy, sk, sk1, t;
    double x_s, y_s, nx, ny, dx, dy, du, dv;
    double eps = 1.e-5;
    double *p_are1, *p_are2, *p_are;

    assert(contour != NULL);

    if (contour == NULL)
        return CV_NULLPTR_ERR;

    if (!CV_IS_SEQ_POLYGON(contour))
        return CV_BADFLAG_ERR;

    lpt = cvSliceLength(slice, contour);
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
        p_are1 = (double *)cvAlloc(p_max * sizeof(double));

        if (p_are1 == NULL)
            return CV_OUTOFMEM_ERR;

        p_are = p_are1;
        p_are2 = NULL;

        cvStartReadSeq(contour, &reader, 0);
        cvSetSeqReaderPos(&reader, slice.start_index);
        CV_READ_SEQ_ELEM(pt_s, reader);
        p_ind = 0;
        cvSetSeqReaderPos(&reader, slice.end_index);
        CV_READ_SEQ_ELEM(pt_e, reader);

        /*    normal coefficients    */
        nx = pt_s.y - pt_e.y;
        ny = pt_e.x - pt_s.x;
        cvSetSeqReaderPos(&reader, slice.start_index);

        while (lpt-- > 0)
        {
            CV_READ_SEQ_ELEM(pt, reader);

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
                            icvMemCopy(&p_are1, &p_are2, &p_are, &p_max);

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
                                icvMemCopy(&p_are1, &p_are2, &p_are, &p_max);

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
            icvMemCopy(&p_are1, &p_are2, &p_are, &p_max);

        p_are[p_ind] = a00 / 2.;
        p_ind++;

        /*     common area calculation    */
        *area = 0;
        for (i = 0; i < p_ind; i++)
            (*area) += fabs(p_are[i]);

        if (p_are1 != NULL)
            cvFree(&p_are1);
        else if (p_are2 != NULL)
            cvFree(&p_are2);

        return CV_OK;
    }
    else
        return CV_BADSIZE_ERR;
}

/* external contour area function */
CV_IMPL double
cvContourArea(const void *array, CvSlice slice)
{
    double area = 0;

    CV_FUNCNAME("cvContourArea");

    __BEGIN__;

    CvSeq *contour = 0;
    if (CV_IS_SEQ(array))
    {
        contour = (CvSeq *)array;
        if (!CV_IS_SEQ_POLYLINE(contour))
            CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
    }

    if (cvSliceLength(slice, contour) == contour->total)
    {
        IPPI_CALL(icvContourArea(contour, &area));
    }
    else
    {
        if (CV_SEQ_ELTYPE(contour) != CV_32SC2)
            CV_ERROR(CV_StsUnsupportedFormat,
                     "Only curves with integer coordinates are supported in case of contour slice");
        IPPI_CALL(icvContourSecArea(contour, slice, &area));
    }

    __END__;

    return area;
}

/* Calculates bounding rectagnle of a point set or retrieves already calculated */
CV_IMPL CvRect
cvBoundingRect(CvArr *array, int update)
{
    CvSeqReader reader;
    CvRect rect = {0, 0, 0, 0};
    CvSeq *ptseq = 0;

    CV_FUNCNAME("cvBoundingRect");

    __BEGIN__;

    CvMat *mat = 0;
    int xmin = 0, ymin = 0, xmax = -1, ymax = -1, i, j, k;
    int calculate = update;

    if (CV_IS_SEQ(array))
    {
        ptseq = (CvSeq *)array;
        if (!CV_IS_SEQ_POINT_SET(ptseq))
            CV_ERROR(CV_StsBadArg, "Unsupported sequence type");

        if (ptseq->header_size < (int)sizeof(CvContour))
        {
            /*if( update == 1 )
                CV_ERROR( CV_StsBadArg, "The header is too small to fit the rectangle, "
                                        "so it could not be updated" );*/
            update = 0;
            calculate = 1;
        }
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
    }

    if (!calculate)
    {
        rect = ((CvContour *)ptseq)->rect;
        EXIT;
    }

    if (ptseq->total)
    {
        int is_float = CV_SEQ_ELTYPE(ptseq) == CV_32FC2;
        cvStartReadSeq(ptseq, &reader, 0);
        assert(!is_float);

        CvPoint pt;
        /* init values */
        CV_READ_SEQ_ELEM(pt, reader);
        xmin = xmax = pt.x;
        ymin = ymax = pt.y;

        for (i = 1; i < ptseq->total; i++)
        {
            CV_READ_SEQ_ELEM(pt, reader);

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

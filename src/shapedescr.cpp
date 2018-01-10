

#include "process.h"
#include "misc.h"

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
    Seq *contour = NULL;

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

            Point *pt = (Point *)reader.ptr;
            Point *prev_pt = (Point *)reader.prev_elem;

            dx = (float)pt->x - (float)prev_pt->x;
            dy = (float)pt->y - (float)prev_pt->y;

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
static int
iContourArea(const Seq *contour, double *area)
{
    if (contour->total)
    {
        SeqReader reader;
        int lpt = contour->total;
        double a00 = 0, xi_1, yi_1;
        StartReadSeq(contour, &reader, 0);

        xi_1 = ((Point *)(reader.ptr))->x;
        yi_1 = ((Point *)(reader.ptr))->y;

        VOS_NEXT_SEQ_ELEM(contour->elem_size, reader);

        while (lpt-- > 0)
        {
            double dxy, xi, yi;

            xi = ((Point *)(reader.ptr))->x;
            yi = ((Point *)(reader.ptr))->y;

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

    return VOS_StsOk;
}

/* external contour area function */
double
ContourArea(const Seq *contour, Slice slice)
{
    double area = 0;

    VOS_FUNCNAME("ContourArea");

    __BEGIN__;

    if (NULL == contour)
        VOS_ERROR( VOS_StsNullPtr, "" );


    if (SliceLength(slice, contour) == contour->total)
    {
        VOS_FUN_CALL(iContourArea(contour, &area));
    }
    else
    {
            VOS_ERROR(VOS_StsUnsupportedFormat,
                      "Only curves with integer coordinates are supported in case of contour slice");
    }

    __END__;

    return area;
}

/* Calculates bounding rectagnle of a point set or retrieves already calculated */
Rect BoundingRect(VOID *array, int update)
{
    SeqReader reader;
    Rect rect = {0, 0, 0, 0};
    Seq *ptseq = NULL;

    VOS_FUNCNAME("BoundingRect");

    __BEGIN__;

    int xmin = 0, ymin = 0, xmax = -1, ymax = -1, i;
    int calculate = update;

    if (VOS_IS_SEQ(array))
    {
        ptseq = (Seq *)array;
        if (!VOS_IS_SEQ_POINT_SET(ptseq))
            VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");

        if (ptseq->header_size < (int)sizeof(Contour))
        {
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
        rect = ((Contour *)ptseq)->rect;
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
        ((Contour *)ptseq)->rect = rect;

    __END__;

    return rect;
}

/* End of file. */

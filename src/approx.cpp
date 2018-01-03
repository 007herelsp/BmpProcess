
#include "_cv.h"

/****************************************************************************************\
*                               Polygonal Approximation                                  *
\****************************************************************************************/
/* the version for integer point coordinates */
static CvStatus
iApproxPolyDP_32s(Seq *src_contour, int header_size,
                    MemStorage *storage,
                    Seq **dst_contour, float eps)
{
    int init_iters = 3;
    Slice slice = {0, 0}, right_slice = {0, 0};
    SeqReader reader, reader2;
    SeqWriter writer;
    Point start_pt = {INT_MIN, INT_MIN}, end_pt = {0, 0}, pt = {0, 0};
    int i = 0, j, count = src_contour->total, new_count;
    int is_closed = VOS_IS_SEQ_CLOSED(src_contour);
    int le_eps = 0;
    MemStorage *temp_storage = 0;
    Seq *stack = 0;

    assert(VOS_SEQ_ELTYPE(src_contour) == VOS_32SC2);
    StartWriteSeq(src_contour->flags, header_size, sizeof(pt), storage, &writer);

    if (0 == src_contour->total)
    {
        *dst_contour = EndWriteSeq(&writer);
        return VOS_OK;
    }

    temp_storage = CreateChildMemStorage(storage);

    assert(src_contour->first != 0);
    stack = CreateSeq(0, sizeof(Seq), sizeof(Slice), temp_storage);
    eps *= eps;
    StartReadSeq(src_contour, &reader, 0);

    if (!is_closed)
    {
        right_slice.start_index = count;
        end_pt = *(Point *)(reader.ptr);
        start_pt = *(Point *)GetSeqElem(src_contour, -1);

        if (start_pt.x != end_pt.x || start_pt.y != end_pt.y)
        {
            slice.start_index = 0;
            slice.end_index = count - 1;
            SeqPush(stack, &slice);
        }
        else
        {
            is_closed = 1;
            init_iters = 1;
        }
    }

    if (is_closed)
    {
        /* 1. Find approximately two farthest points of the contour */
        right_slice.start_index = 0;

        for (i = 0; i < init_iters; i++)
        {
            int max_dist = 0;
            SetSeqReaderPos(&reader, right_slice.start_index, 1);
            VOS_READ_SEQ_ELEM(start_pt, reader); /* read the first point */

            for (j = 1; j < count; j++)
            {
                int dx, dy, dist;

                VOS_READ_SEQ_ELEM(pt, reader);
                dx = pt.x - start_pt.x;
                dy = pt.y - start_pt.y;

                dist = dx * dx + dy * dy;

                if (dist > max_dist)
                {
                    max_dist = dist;
                    right_slice.start_index = j;
                }
            }

            le_eps = max_dist <= eps;
        }

        /* 2. initialize the stack */
        if (!le_eps)
        {
            slice.start_index = GetSeqReaderPos(&reader);
            slice.end_index = right_slice.start_index += slice.start_index;

            right_slice.start_index -= right_slice.start_index >= count ? count : 0;
            right_slice.end_index = slice.start_index;
            if (right_slice.end_index < right_slice.start_index)
                right_slice.end_index += count;

            SeqPush(stack, &right_slice);
            SeqPush(stack, &slice);
        }
        else
            VOS_WRITE_SEQ_ELEM(start_pt, writer);
    }

    /* 3. run recursive process */
    while (stack->total != 0)
    {
        SeqPop(stack, &slice);

        if (slice.end_index > slice.start_index + 1)
        {
            int dx, dy, dist, max_dist = 0;

            SetSeqReaderPos(&reader, slice.end_index);
            VOS_READ_SEQ_ELEM(end_pt, reader);

            SetSeqReaderPos(&reader, slice.start_index);
            VOS_READ_SEQ_ELEM(start_pt, reader);

            dx = end_pt.x - start_pt.x;
            dy = end_pt.y - start_pt.y;

            assert(dx != 0 || dy != 0);

            for (i = slice.start_index + 1; i < slice.end_index; i++)
            {
                VOS_READ_SEQ_ELEM(pt, reader);
                dist = abs((pt.y - start_pt.y) * dx - (pt.x - start_pt.x) * dy);

                if (dist > max_dist)
                {
                    max_dist = dist;
                    right_slice.start_index = i;
                }
            }

            le_eps = (double)max_dist * max_dist <= eps * ((double)dx * dx + (double)dy * dy);
        }
        else
        {
            assert(slice.end_index > slice.start_index);
            le_eps = 1;
            /* read starting point */
            SetSeqReaderPos(&reader, slice.start_index);
            VOS_READ_SEQ_ELEM(start_pt, reader);
        }

        if (le_eps)
        {
            VOS_WRITE_SEQ_ELEM(start_pt, writer);
        }
        else
        {
            right_slice.end_index = slice.end_index;
            slice.end_index = right_slice.start_index;
            SeqPush(stack, &right_slice);
            SeqPush(stack, &slice);
        }
    }

    is_closed = VOS_IS_SEQ_CLOSED(src_contour);
    if (!is_closed)
        VOS_WRITE_SEQ_ELEM(end_pt, writer);

    *dst_contour = EndWriteSeq(&writer);

    StartReadSeq(*dst_contour, &reader, is_closed);
    VOS_READ_SEQ_ELEM(start_pt, reader);

    reader2 = reader;
    VOS_READ_SEQ_ELEM(pt, reader);

    new_count = count = (*dst_contour)->total;
    for (i = !is_closed; i < count - !is_closed && new_count > 2; i++)
    {
        int dx, dy, dist;
        VOS_READ_SEQ_ELEM(end_pt, reader);

        dx = end_pt.x - start_pt.x;
        dy = end_pt.y - start_pt.y;
        dist = abs((pt.x - start_pt.x) * dy - (pt.y - start_pt.y) * dx);
        if ((double)dist * dist <= 0.5 * eps * ((double)dx * dx + (double)dy * dy) && dx != 0 && dy != 0)
        {
            new_count--;
            *((Point *)reader2.ptr) = start_pt = end_pt;
            VOS_NEXT_SEQ_ELEM(sizeof(pt), reader2);
            VOS_READ_SEQ_ELEM(pt, reader);
            i++;
            continue;
        }
        *((Point *)reader2.ptr) = start_pt = pt;
        VOS_NEXT_SEQ_ELEM(sizeof(pt), reader2);
        pt = end_pt;
    }

    if (!is_closed)
        *((Point *)reader2.ptr) = pt;

    if (new_count < count)
        SeqPopMulti(*dst_contour, 0, count - new_count);

    ReleaseMemStorage(&temp_storage);

    return VOS_OK;
}

Seq *
ApproxPoly(const void *array, int header_size,
             MemStorage *storage, int method,
             double parameter, int parameter2)
{
    Seq *dst_seq = 0;
    Seq *prev_contour = 0, *parent = 0;
    Seq *src_seq = 0;
    int recursive = 0;

    VOS_FUNCNAME("ApproxPoly");

    __BEGIN__;

    if (VOS_IS_SEQ(array))
    {
        src_seq = (Seq *)array;
        if (!VOS_IS_SEQ_POLYLINE(src_seq))
            VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");

        recursive = parameter2;

        if (!storage)
            storage = src_seq->storage;
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "Unsupported sequence type");
    }

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "NULL storage pointer ");

    if (header_size < 0)
        VOS_ERROR(VOS_StsOutOfRange, "header_size is negative. "
                                   "Pass 0 to make the destination header_size == input header_size");

    if (header_size == 0)
        header_size = src_seq->header_size;

    if (!VOS_IS_SEQ_POLYLINE(src_seq))
    {
        if (VOS_IS_SEQ_CHAIN(src_seq))
        {
            VOS_ERROR(VOS_StsBadArg, "Input curves are not polygonal. "
                                   "Use cvApproxChains first");
        }
        else
        {
            VOS_ERROR(VOS_StsBadArg, "Input curves have uknown type");
        }
    }

    if (header_size == 0)
        header_size = src_seq->header_size;

    if (header_size < (int)sizeof(CvContour))
        VOS_ERROR(VOS_StsBadSize, "New header size must be non-less than sizeof(CvContour)");

    if (method != VOS_POLY_APPROX_DP)
        VOS_ERROR(VOS_StsOutOfRange, "Unknown approximation method");

    if (VOS_SEQ_ELTYPE(src_seq) != VOS_32SC2)
        VOS_ERROR(VOS_StsOutOfRange, "Unknown type");

    if (parameter < 0)
        VOS_ERROR(VOS_StsOutOfRange, "Accuracy must be non-negative");

    while (src_seq != 0)
    {
        Seq *contour = 0;

        FUN_CALL(iApproxPolyDP_32s(src_seq, header_size, storage,
                                      &contour, (float)parameter));

        assert(contour);

        if (header_size >= (int)sizeof(CvContour))
            BoundingRect(contour, 1);

        contour->v_prev = parent;
        contour->h_prev = prev_contour;

        if (prev_contour)
            prev_contour->h_next = contour;
        else if (parent)
            parent->v_next = contour;
        prev_contour = contour;
        if (!dst_seq)
            dst_seq = prev_contour;

        if (!recursive)
            break;

        if (src_seq->v_next)
        {
            assert(prev_contour != 0);
            parent = prev_contour;
            prev_contour = 0;
            src_seq = src_seq->v_next;
        }
        else
        {
            while (src_seq->h_next == 0)
            {
                src_seq = src_seq->v_prev;
                if (src_seq == 0)
                    break;
                prev_contour = parent;
                if (parent)
                    parent = parent->v_prev;
            }
            if (src_seq)
                src_seq = src_seq->h_next;
        }
    }

    __END__;

    return dst_seq;
}

/* End of file. */

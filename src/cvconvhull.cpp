

#include "_cv.h"

static int
icvSklansky_32s(CvPoint **array, int start, int end, int *stack, int nsign, int sign2)
{
    int incr = end > start ? 1 : -1;
    /* prepare first triangle */
    int pprev = start, pcur = pprev + incr, pnext = pcur + incr;
    int stacksize = 3;

    if (start == end ||
        (array[start]->x == array[end]->x &&
         array[start]->y == array[end]->y))
    {
        stack[0] = start;
        return 1;
    }

    stack[0] = pprev;
    stack[1] = pcur;
    stack[2] = pnext;

    end += incr; /* make end = afterend */

    while (pnext != end)
    {
        /* check the angle p1,p2,p3 */
        int cury = array[pcur]->y;
        int nexty = array[pnext]->y;
        int by = nexty - cury;

        if (CV_SIGN(by) != nsign)
        {
            int ax = array[pcur]->x - array[pprev]->x;
            int bx = array[pnext]->x - array[pcur]->x;
            int ay = cury - array[pprev]->y;
            int convexity = ay * bx - ax * by; /* if >0 then convex angle */

            if (CV_SIGN(convexity) == sign2 && (ax != 0 || ay != 0))
            {
                pprev = pcur;
                pcur = pnext;
                pnext += incr;
                stack[stacksize] = pnext;
                stacksize++;
            }
            else
            {
                if (pprev == start)
                {
                    pcur = pnext;
                    stack[1] = pcur;
                    pnext += incr;
                    stack[2] = pnext;
                }
                else
                {
                    stack[stacksize - 2] = pnext;
                    pcur = pprev;
                    pprev = stack[stacksize - 4];
                    stacksize--;
                }
            }
        }
        else
        {
            pnext += incr;
            stack[stacksize - 1] = pnext;
        }
    }

    return --stacksize;
}

typedef int (*sklansky_func)(CvPoint **points, int start, int end,
                             int *stack, int sign, int sign2);

#define cmp_pts(pt1, pt2) \
    ((pt1)->x < (pt2)->x || (pt1)->x <= (pt2)->x && (pt1)->y < (pt2)->y)
static CV_IMPLEMENT_QSORT(icvSortPointsByPointers_32s, CvPoint *, cmp_pts)


CV_IMPL CvSeq *
cvConvexHull2(const CvArr *array, void *hull_storage,
              int orientation, int return_points)
{
    union {
        CvContour *c;
        CvSeq *s;
    } hull;
    CvPoint **pointer = 0;
    CvPoint2D32f **pointerf = 0;
    int *stack = 0;

    CV_FUNCNAME("cvConvexHull2");

    hull.s = 0;

    __BEGIN__;

    CvMat *mat = 0;
    CvSeqReader reader;
    CvSeqWriter writer;
    CvContour contour_header;
    union {
        CvContour c;
        CvSeq s;
    } hull_header;
    CvSeqBlock hullblock;
    CvSeq *ptseq = 0;
    CvSeq *hullseq = 0;
    int is_float;
    int *t_stack;
    int t_count;
    int i, miny_ind = 0, maxy_ind = 0, total;
    int hulltype;
    int stop_idx;
    sklansky_func sklansky;

    if (CV_IS_SEQ(array))
    {
        ptseq = (CvSeq *)array;
        if (!CV_IS_SEQ_POINT_SET(ptseq))
            CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
        if (hull_storage == 0)
            hull_storage = ptseq->storage;
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "herelsp removed Unsupported sequence type");
    }

    if (CV_IS_STORAGE(hull_storage))
    {
        if (return_points)
        {
            CV_CALL(hullseq = cvCreateSeq(
                        CV_SEQ_KIND_CURVE | CV_SEQ_ELTYPE(ptseq) |
                            CV_SEQ_FLAG_CLOSED | CV_SEQ_FLAG_CONVEX,
                        sizeof(CvContour), sizeof(CvPoint), (CvMemStorage *)hull_storage));
        }
        else
        {
            CV_ERROR(CV_StsBadArg, "herelsp remove");
        }
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "herelsp remove");
    }

    total = ptseq->total;
    if (total == 0)
    {
        if (mat)
            CV_ERROR(CV_StsBadSize,
                     "Point sequence can not be empty if the output is matrix");
        EXIT;
    }

    cvStartAppendToSeq(hullseq, &writer);

    if (CV_SEQ_ELTYPE(ptseq) == CV_32FC2)
    {
        CV_ERROR(CV_StsBadArg, "herelsp remove");
    }
    hulltype = CV_SEQ_ELTYPE(hullseq);
    sklansky = (sklansky_func)icvSklansky_32s;

    CV_CALL(pointer = (CvPoint **)cvAlloc(ptseq->total * sizeof(pointer[0])));
    CV_CALL(stack = (int *)cvAlloc((ptseq->total + 2) * sizeof(stack[0])));
    pointerf = (CvPoint2D32f **)pointer;

    cvStartReadSeq(ptseq, &reader);

    for (i = 0; i < total; i++)
    {
        pointer[i] = (CvPoint *)reader.ptr;
        CV_NEXT_SEQ_ELEM(ptseq->elem_size, reader);
    }

    // sort the point set by x-coordinate, find min and max y

    icvSortPointsByPointers_32s(pointer, total, 0);
    for (i = 1; i < total; i++)
    {
        int y = pointer[i]->y;
        if (pointer[miny_ind]->y > y)
            miny_ind = i;
        if (pointer[maxy_ind]->y < y)
            maxy_ind = i;
    }

    if (pointer[0]->x == pointer[total - 1]->x &&
        pointer[0]->y == pointer[total - 1]->y)
    {
        CvPoint pt = pointer[0][0];
        CV_WRITE_SEQ_ELEM(pt, writer);
        goto finish_hull;
    }

    /*upper half */
    {
        int *tl_stack = stack;
        int tl_count = sklansky(pointer, 0, maxy_ind, tl_stack, -1, 1);
        int *tr_stack = tl_stack + tl_count;
        int tr_count = sklansky(pointer, ptseq->total - 1, maxy_ind, tr_stack, -1, -1);

        /* gather upper part of convex hull to output */
        if (orientation == CV_COUNTER_CLOCKWISE)
        {
            CV_SWAP(tl_stack, tr_stack, t_stack);
            CV_SWAP(tl_count, tr_count, t_count);
        }

        for (i = 0; i < tl_count - 1; i++)
            CV_WRITE_SEQ_ELEM(pointer[tl_stack[i]][0], writer);

        for (i = tr_count - 1; i > 0; i--)
            CV_WRITE_SEQ_ELEM(pointer[tr_stack[i]][0], writer);

        stop_idx = tr_count > 2 ? tr_stack[1] : tl_count > 2 ? tl_stack[tl_count - 2] : -1;
    }

    /* lower half */
    {
        int *bl_stack = stack;
        int bl_count = sklansky(pointer, 0, miny_ind, bl_stack, 1, -1);
        int *br_stack = stack + bl_count;
        int br_count = sklansky(pointer, ptseq->total - 1, miny_ind, br_stack, 1, 1);

        if (orientation != CV_COUNTER_CLOCKWISE)
        {
            CV_SWAP(bl_stack, br_stack, t_stack);
            CV_SWAP(bl_count, br_count, t_count);
        }

        if (stop_idx >= 0)
        {
            int check_idx = bl_count > 2 ? bl_stack[1] : bl_count + br_count > 2 ? br_stack[2 - bl_count] : -1;
            if (check_idx == stop_idx || check_idx >= 0 &&
                                             pointer[check_idx]->x == pointer[stop_idx]->x &&
                                             pointer[check_idx]->y == pointer[stop_idx]->y)
            {
                /* if all the points lie on the same line, then
                   the bottom part of the convex hull is the mirrored top part
                   (except the exteme points).*/
                bl_count = MIN(bl_count, 2);
                br_count = MIN(br_count, 2);
            }
        }

        for (i = 0; i < bl_count - 1; i++)
            CV_WRITE_SEQ_ELEM(pointer[bl_stack[i]][0], writer);

        for (i = br_count - 1; i > 0; i--)
            CV_WRITE_SEQ_ELEM(pointer[br_stack[i]][0], writer);
    }

finish_hull:
    CV_CALL(cvEndWriteSeq(&writer));

    if (mat)
    {
        if (mat->rows > mat->cols)
            mat->rows = hullseq->total;
        else
            mat->cols = hullseq->total;
    }
    else
    {
        hull.s = hullseq;
        hull.c->rect = cvBoundingRect(ptseq,
                                      ptseq->header_size < (int)sizeof(CvContour) ||
                                          &ptseq->flags == &contour_header.flags);

        /*if( ptseq != (CvSeq*)&contour_header )
            hullseq->v_prev = ptseq;*/
    }

    __END__;

    cvFree(&pointer);
    cvFree(&stack);

    return hull.s;
}

CV_IMPL int
cvCheckContourConvexity(const CvArr *array)
{
    int flag = -1;

    CV_FUNCNAME("cvCheckContourConvexity");

    __BEGIN__;

    int i;
    int orientation = 0;
    CvSeqReader reader;
    CvContour contour_header;

    CvSeq *contour = (CvSeq *)array;

    if (CV_IS_SEQ(contour))
    {
        if (!CV_IS_SEQ_POLYGON(contour))
            CV_ERROR(CV_StsUnsupportedFormat,
                     "Input sequence must be polygon (closed 2d curve)");
    }
    else
    {
        CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
    }

    if (contour->total == 0)
        EXIT;

    cvStartReadSeq(contour, &reader, 0);

    flag = 1;

    if (CV_SEQ_ELTYPE(contour) == CV_32SC2)
    {
        CvPoint *prev_pt = (CvPoint *)reader.prev_elem;
        CvPoint *cur_pt = (CvPoint *)reader.ptr;

        int dx0 = cur_pt->x - prev_pt->x;
        int dy0 = cur_pt->y - prev_pt->y;

        for (i = 0; i < contour->total; i++)
        {
            int dxdy0, dydx0;
            int dx, dy;

            /*int orient; */
            CV_NEXT_SEQ_ELEM(sizeof(CvPoint), reader);
            prev_pt = cur_pt;
            cur_pt = (CvPoint *)reader.ptr;

            dx = cur_pt->x - prev_pt->x;
            dy = cur_pt->y - prev_pt->y;
            dxdy0 = dx * dy0;
            dydx0 = dy * dx0;

            /* find orientation */
            /*orient = -dy0 * dx + dx0 * dy;
               orientation |= (orient > 0) ? 1 : 2;
             */
            orientation |= (dydx0 > dxdy0) ? 1 : ((dydx0 < dxdy0) ? 2 : 3);

            if (orientation == 3)
            {
                flag = 0;
                break;
            }

            dx0 = dx;
            dy0 = dy;
        }
    }
    else
    	{
		CV_ERROR(CV_StsBadArg, "Unsupported sequence type");
	}

    __END__;

    return flag;
}

/* End of file. */

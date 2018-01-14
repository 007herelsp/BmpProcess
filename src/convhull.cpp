#include "process.h"
#include "misc.h"

static int
iSklansky_32s(Point **array, int start, int end, int *stack, int nsign, int sign2)
{
    int incr = end > start ? 1 : -1;
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
        /* check the CaclAngle p1,p2,p3 */
        int cury = array[pcur]->y;
        int nexty = array[pnext]->y;
        int by = nexty - cury;

        if (VOS_SIGN(by) != nsign)
        {
            int ax = array[pcur]->x - array[pprev]->x;
            int bx = array[pnext]->x - array[pcur]->x;
            int ay = cury - array[pprev]->y;
            int convexity = ay * bx - ax * by; /* if >0 then convex CaclAngle */

            if (VOS_SIGN(convexity) == sign2 && (ax != 0 || ay != 0))
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

typedef int (*sklansky_func)(Point **points, int start, int end,
                             int *stack, int sign, int sign2);

#define cmp_pts(pt1, pt2) \
    (((pt1)->x < (pt2)->x) || ((pt1)->x <= (pt2)->x && (pt1)->y < (pt2)->y))

static void iSortPointsByPointers_32s(Point **array, size_t total)
{
    int isort_thresh = 7;
    Point *t;
    int sp = 0;
    struct
    {
        Point **lb;
        Point **ub;
    } stack[48];

    if (total <= 1)
        return;
    stack[0].lb = array;
    stack[0].ub = array + (total - 1);
    while (sp >= 0)
    {
        Point **left = stack[sp].lb;
        Point **right = stack[sp--].ub;
        for (;;)
        {
            int i, n = (int)(right - left) + 1, m;
            Point **ptr;
            Point **ptr2;
            if (n <= isort_thresh)
            {
            insert_sort:
                for (ptr = left + 1; ptr <= right; ptr++)
                {
                    for (ptr2 = ptr; ptr2 > left && cmp_pts(ptr2[0], ptr2[-1]); ptr2--)
                    {
                        //((t) = (ptr2[0]), (ptr2[0]) = (ptr2[-1]), (ptr2[-1]) = (t));
                        VOS_SWAP(ptr2[0], ptr2[-1], t);
                    }
                }
                break;
            }
            else
            {
                Point **left0;
                Point **left1;
                Point **right0;
                Point **right1;
                Point **pivot;
                Point **a;
                Point **b;
                Point **c;
                int swap_cnt = 0;
                left0 = left;
                right0 = right;
                pivot = left + (n / 2);
                if (n > 40)
                {
                    int d = n / 8;
                    a = left, b = left + d, c = left + 2 * d;
                    left = cmp_pts(*a, *b) ? (cmp_pts(*b, *c) ? b : (cmp_pts(*a, *c) ? c : a))
                                           : (cmp_pts(*c, *b) ? b : (cmp_pts(*a, *c) ? a : c));

                    a = pivot - d, b = pivot, c = pivot + d;
                    pivot = cmp_pts(*a, *b) ? (cmp_pts(*b, *c) ? b : (cmp_pts(*a, *c) ? c : a))
                                            : (cmp_pts(*c, *b) ? b : (cmp_pts(*a, *c) ? a : c));

                    a = right - 2 * d, b = right - d, c = right;
                    right = cmp_pts(*a, *b) ? (cmp_pts(*b, *c) ? b : (cmp_pts(*a, *c) ? c : a))
                                            : (cmp_pts(*c, *b) ? b : (cmp_pts(*a, *c) ? a : c));
                }
                a = left, b = pivot, c = right;
                pivot = cmp_pts(*a, *b) ? (cmp_pts(*b, *c) ? b : (cmp_pts(*a, *c) ? c : a))
                                        : (cmp_pts(*c, *b) ? b : (cmp_pts(*a, *c) ? a : c));
                if (pivot != left0)
                {
                    VOS_SWAP(*pivot, *left0, t);
                    pivot = left0;
                }
                left = left1 = left0 + 1;
                right = right1 = right0;
                for (;;)
                {
                    while (left <= right && !cmp_pts(*pivot, *left))
                    {
                        if (!cmp_pts(*left, *pivot))
                        {
                            if (left > left1)
                                VOS_SWAP(*left1, *left, t);
                            swap_cnt = 1;
                            left1++;
                        }
                        left++;
                    }
                    while (left <= right && !cmp_pts(*right, *pivot))
                    {
                        if (!cmp_pts(*pivot, *right))
                        {
                            if (right < right1)
                                VOS_SWAP(*right1, *right, t);
                            swap_cnt = 1;
                            right1--;
                        }
                        right--;
                    }
                    if (left > right)
                        break;
                    VOS_SWAP(*left, *right, t);
                    swap_cnt = 1;
                    left++;
                    right--;
                }
                if (0 == swap_cnt)
                {
                    left = left0, right = right0;
                    goto insert_sort;
                }
                n = VOS_MIN((int)(left1 - left0), (int)(left - left1));
                for (i = 0; i < n; i++)
                    VOS_SWAP(left0[i], left[i - n], t);

                n = VOS_MIN((int)(right0 - right1), (int)(right1 - right));
                for (i = 0; i < n; i++)
                    VOS_SWAP(left[i], right0[i - n + 1], t);
                n = (int)(left - left1);
                m = (int)(right1 - right);
                if (n > 1)
                {
                    if (m > 1)
                    {
                        if (n > m)
                        {
                            stack[++sp].lb = left0;
                            stack[sp].ub = left0 + n - 1;
                            left = right0 - m + 1, right = right0;
                        }
                        else
                        {
                            stack[++sp].lb = right0 - m + 1;
                            stack[sp].ub = right0;
                            left = left0, right = left0 + n - 1;
                        }
                    }
                    else
                        left = left0, right = left0 + n - 1;
                }
                else if (m > 1)
                    left = right0 - m + 1, right = right0;
                else
                    break;
            }
        }
    }
}

Seq *ConvexHull2(const VOID *array, void *hull_storage,
                 int orientation)
{
    union {
        Contour *c;
        Seq *s;
    } hull;
    Point **pointer = NULL;
    int *stack = NULL;

    VOS_FUNCNAME("ConvexHull2");

    hull.s = NULL;

    __BEGIN__;

    Mat *mat = NULL;
    SeqReader reader;
    SeqWriter writer;
    Contour contour_header;
    Seq *ptseq = NULL;
    Seq *hullseq = NULL;
    int *t_stack = NULL;
    int t_count;
    int i, miny_ind = 0, maxy_ind = 0, total;
    int stop_idx;
    sklansky_func sklansky = NULL;

    ptseq = (Seq *)array;
    if (NULL == hull_storage)
        hull_storage = ptseq->storage;

    VOS_CALL(hullseq = CreateSeq(
                 VOS_SEQ_KIND_CURVE | VOS_SEQ_ELTYPE(ptseq) |
                     VOS_SEQ_FLAG_CLOSED | VOS_SEQ_FLAG_CONVEX,
                 sizeof(Contour), sizeof(Point), (MemStorage *)hull_storage));

    total = ptseq->total;
    if (0 == total)
    {
        if (mat)
            VOS_ERROR(VOS_StsBadSize,"");
        EXIT;
    }

    StartAppendToSeq(hullseq, &writer);

    sklansky = (sklansky_func)iSklansky_32s;

    VOS_CALL(pointer = (Point **)SysAlloc(ptseq->total * sizeof(pointer[0])));
    VOS_CALL(stack = (int *)SysAlloc((ptseq->total + 2) * sizeof(stack[0])));

    StartReadSeq(ptseq, &reader);

    for (i = 0; i < total; i++)
    {
        pointer[i] = (Point *)reader.ptr;
        VOS_NEXT_SEQ_ELEM(ptseq->elem_size, reader);
    }

    iSortPointsByPointers_32s(pointer, total);
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
        Point pt = pointer[0][0];
        VOS_WRITE_SEQ_ELEM(pt, writer);
        goto finish_hull;
    }

    /*upper half */
    {
        int *tl_stack = stack;
        int tl_count = sklansky(pointer, 0, maxy_ind, tl_stack, -1, 1);
        int *tr_stack = tl_stack + tl_count;
        int tr_count = sklansky(pointer, ptseq->total - 1, maxy_ind, tr_stack, -1, -1);

        /* gather upper part of convex hull to output */
        if (VOS_COUNTER_CLOCKWISE == orientation)
        {
            VOS_SWAP(tl_stack, tr_stack, t_stack);
            VOS_SWAP(tl_count, tr_count, t_count);
        }

        for (i = 0; i < tl_count - 1; i++)
            VOS_WRITE_SEQ_ELEM(pointer[tl_stack[i]][0], writer);

        for (i = tr_count - 1; i > 0; i--)
            VOS_WRITE_SEQ_ELEM(pointer[tr_stack[i]][0], writer);

        stop_idx = tr_count > 2 ? tr_stack[1] : tl_count > 2 ? tl_stack[tl_count - 2] : -1;
    }

    /* lower half */
    {
        int *bl_stack = stack;
        int bl_count = sklansky(pointer, 0, miny_ind, bl_stack, 1, -1);
        int *br_stack = stack + bl_count;
        int br_count = sklansky(pointer, ptseq->total - 1, miny_ind, br_stack, 1, 1);

        if (VOS_COUNTER_CLOCKWISE != orientation)
        {
            VOS_SWAP(bl_stack, br_stack, t_stack);
            VOS_SWAP(bl_count, br_count, t_count);
        }

        if (stop_idx >= 0)
        {
            int check_idx = bl_count > 2 ? bl_stack[1] : bl_count + br_count > 2 ? br_stack[2 - bl_count] : -1;
            if (check_idx == stop_idx || (check_idx >= 0 &&
                                          pointer[check_idx]->x == pointer[stop_idx]->x &&
                                          pointer[check_idx]->y == pointer[stop_idx]->y))
            {
                bl_count = VOS_MIN(bl_count, 2);
                br_count = VOS_MIN(br_count, 2);
            }
        }

        for (i = 0; i < bl_count - 1; i++)
            VOS_WRITE_SEQ_ELEM(pointer[bl_stack[i]][0], writer);

        for (i = br_count - 1; i > 0; i--)
            VOS_WRITE_SEQ_ELEM(pointer[br_stack[i]][0], writer);
    }

finish_hull:
    VOS_CALL(EndWriteSeq(&writer));

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
        hull.c->rect = BoundingRect(ptseq,
                                    ptseq->header_size < (int)sizeof(Contour) ||
                                        &ptseq->flags == &contour_header.flags);
    }

    __END__;

    SYS_FREE(&pointer);
    SYS_FREE(&stack);

    return hull.s;
}

int CheckContourConvexity(const VOID *array)
{
    int flag = -1;

    VOS_FUNCNAME("CheckContourConvexity");

    __BEGIN__;

    int i;
    int orientation = 0;
    SeqReader reader;

    Seq *contour = (Seq *)array;

    if (0 == contour->total)
        EXIT;

    StartReadSeq(contour, &reader, 0);

    flag = 1;

    if (VOS_SEQ_ELTYPE(contour) == VOS_32SC2)
    {
        Point *prev_pt = (Point *)reader.prev_elem;
        Point *cur_pt = (Point *)reader.ptr;

        int dx0 = cur_pt->x - prev_pt->x;
        int dy0 = cur_pt->y - prev_pt->y;

        for (i = 0; i < contour->total; i++)
        {
            int dxdy0, dydx0;
            int dx, dy;

            /*int orient; */
            VOS_NEXT_SEQ_ELEM(sizeof(Point), reader);
            prev_pt = cur_pt;
            cur_pt = (Point *)reader.ptr;

            dx = cur_pt->x - prev_pt->x;
            dy = cur_pt->y - prev_pt->y;
            dxdy0 = dx * dy0;
            dydx0 = dy * dx0;

            orientation |= (dydx0 > dxdy0) ? 1 : ((dydx0 < dxdy0) ? 2 : 3);

            if (3 == orientation)
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
        VOS_ERROR(VOS_StsBadArg, "");
    }

    __END__;

    return flag;
}

/* End of file. */

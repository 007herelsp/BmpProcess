
#include "_cv.h"

/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define VOS_INIT_3X3_DELTAS(deltas, step, nch)             \
    ((deltas)[0] = (nch), (deltas)[1] = -(step) + (nch),   \
     (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch), \
     (deltas)[4] = -(nch), (deltas)[5] = (step) - (nch),   \
     (deltas)[6] = (step), (deltas)[7] = (step) + (nch))

static const Point iCodeDeltas[8] =
    {{1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}};

/****************************************************************************************\
*                         Raster->Chain Tree (Suzuki algorithms)                         *
\****************************************************************************************/

typedef struct _ContourInfo
{
    int flags;
    struct _ContourInfo *next;   /* next contour with the same mark value */
    struct _ContourInfo *parent; /* information about parent contour */
    Seq *contour;                /* corresponding contour (may be 0, if rejected) */
    Rect rect;                   /* bounding rectangle */
    Point origin;                /* origin point (where the contour was traced from) */
    int is_hole;                   /* hole flag */
} _ContourInfo;

/*
  Structure that is used for sequental retrieving contours from the image.
  It supports both hierarchical and plane variants of Suzuki algorithm.
*/
typedef struct _ContourScanner
{
    MemStorage *storage1;      /* contains fetched contours */
    MemStorage *storage2;      /* contains approximated contours
                                   (!=storage1 if approx_method2 != approx_method1) */
    MemStorage *cinfo_storage; /* contains _CvContourInfo nodes */
    Set *cinfo_set;            /* set of _CvContourInfo nodes */
    MemStoragePos initial_pos; /* starting storage pos */
    MemStoragePos backup_pos;  /* beginning of the latest approx. contour */
    MemStoragePos backup_pos2; /* ending of the latest approx. contour */
    char *img0;                  /* image origin */
    char *img;                   /* current image row */
    int img_step;                /* image step */
    Size img_size;             /* ROI size */
    Point offset;              /* ROI offset: coordinates, added to each contour point */
    Point pt;                  /* current scanner position */
    Point lnbd;                /* position of the last met contour */
    int nbd;                     /* current mark val */
    _ContourInfo *l_cinfo;     /* information about latest approx. contour */
    _ContourInfo cinfo_temp;   /* temporary var which is used in simple modes */
    _ContourInfo frame_info;   /* information about frame */
    Seq frame;                 /* frame itself */
    int approx_method1;          /* approx method when tracing */
    int approx_method2;          /* final approx method */
    int mode;                    /* contour scanning mode:
                                   0 - external only
                                   1 - all the contours w/o any hierarchy
                                   2 - connected components (i.e. two-level structure -
                                   external contours and holes) */
    int subst_flag;
    int seq_type1;    /* type of fetched contours */
    int header_size1; /* hdr size of fetched contours */
    int elem_size1;   /* elem size of fetched contours */
    int seq_type2;    /*                                       */
    int header_size2; /*        the same for approx. contours  */
    int elem_size2;   /*                                       */
    _ContourInfo *cinfo_table[126];
} _ContourScanner;

#define _VOS_FIND_CONTOURS_FLAGS_EXTERNAL_ONLY 1
#define _VOS_FIND_CONTOURS_FLAGS_HIERARCHIC 2

/*
   Initializes scanner structure.
   Prepare image for scanning ( clear borders and convert all pixels to 0-1.
*/
ContourScanner
StartFindContours(void *_img, MemStorage *storage,
                    int header_size, int mode,
                    int method, Point offset)
{
    int y;
    int step;
    Size size;
    uchar *img = 0;
    ContourScanner scanner = 0;
    Mat stub, *mat = (Mat *)_img;

    VOS_FUNCNAME("StartFindContours");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    VOS_CALL(mat = GetMat(mat, &stub));

    if (!VOS_IS_MASK_ARR(mat))
        VOS_ERROR(VOS_StsUnsupportedFormat, "[Start]FindContours support only 8uC1 images");

    size = GetSize(mat->width, mat->height);
    step = mat->step;
    img = (uchar *)(mat->data.ptr);

    if (method < 0 || method > VOS_LINK_RUNS)
        VOS_ERROR_FROM_STATUS(VOS_BADRANGE_ERR);

    if (header_size < (int)(sizeof(CvContour)))
        VOS_ERROR_FROM_STATUS(VOS_BADSIZE_ERR);

    scanner = (ContourScanner)SysAlloc(sizeof(*scanner));
    if (!scanner)
        VOS_ERROR_FROM_STATUS(VOS_OUTOFMEM_ERR);

    memset(scanner, 0, sizeof(*scanner));

    scanner->storage1 = scanner->storage2 = storage;
    scanner->img0 = (char *)img;
    scanner->img = (char *)(img + step);
    scanner->img_step = step;
    scanner->img_size.width = size.width - 1;   /* exclude rightest column */
    scanner->img_size.height = size.height - 1; /* exclude bottomost row */
    scanner->mode = mode;
    scanner->offset = offset;
    scanner->pt.x = scanner->pt.y = 1;
    scanner->lnbd.x = 0;
    scanner->lnbd.y = 1;
    scanner->nbd = 2;
    scanner->mode = (int)mode;
    scanner->frame_info.contour = &(scanner->frame);
    scanner->frame_info.is_hole = 1;
    scanner->frame_info.next = 0;
    scanner->frame_info.parent = 0;
    scanner->frame_info.rect = initRect(0, 0, size.width, size.height);
    scanner->l_cinfo = 0;
    scanner->subst_flag = 0;

    scanner->frame.flags = VOS_SEQ_FLAG_HOLE;

    scanner->approx_method2 = scanner->approx_method1 = method;

    scanner->seq_type2 = scanner->seq_type1 = VOS_SEQ_POLYGON;
    scanner->header_size2 = scanner->header_size1 = header_size;
    scanner->elem_size2 = scanner->elem_size1 = sizeof(Point);

    SaveMemStoragePos(storage, &(scanner->initial_pos));

    if (method > VOS_CHAIN_APPROX_SIMPLE)
    {
        VOS_ERROR_FROM_STATUS(VOS_BADRANGE_ERR);
        //scanner->storage1 = CreateChildMemStorage( scanner->storage2 );
    }

    if (mode > VOS_RETR_LIST)
    {
        VOS_ERROR_FROM_STATUS(VOS_BADRANGE_ERR);
    }

    /* make zero borders */
    memset(img, 0, size.width);
    memset(img + step * (size.height - 1), 0, size.width);

    for (y = 1, img += step; y < size.height - 1; y++, img += step)
    {
        img[0] = img[size.width - 1] = 0;
    }

    /* converts all pixels to 0 or 1 */
    Threshold(mat, mat, 0, 1, VOS_THRESH_BINARY);
    VOS_CHECK();

    __END__;

    if (GetErrStatus() < 0)
        SYS_FREE(&scanner);

    return scanner;
}

static void
iEndProcessContour(ContourScanner scanner)
{
    _ContourInfo *l_cinfo = scanner->l_cinfo;

    if (l_cinfo)
    {
        if (scanner->subst_flag)
        {
            MemStoragePos temp;

            SaveMemStoragePos(scanner->storage2, &temp);

            if (temp.top == scanner->backup_pos2.top &&
                temp.free_space == scanner->backup_pos2.free_space)
            {
                RestoreMemStoragePos(scanner->storage2, &scanner->backup_pos);
            }
            scanner->subst_flag = 0;
        }

        if (l_cinfo->contour)
        {
            InsertNodeIntoTree(l_cinfo->contour, l_cinfo->parent->contour,
                                 &(scanner->frame));
        }
        scanner->l_cinfo = 0;
    }
}

/*
    marks domain border with +/-<constant> and stores the contour into Seq.
        method:
            <0  - chain
            ==0 - direct
            >0  - simple approximation
*/
static CvStatus
iFetchContour(char *ptr,
                int step,
                Point pt,
                Seq *contour,
                int _method)
{
    const char nbd = 2;
    int deltas[16];
    SeqWriter writer;
    char *i0 = ptr, *i1, *i3, *i4 = 0;
    int prev_s = -1, s, s_end;
    int method = _method - 1;

    assert((unsigned)_method <= VOS_CHAIN_APPROX_SIMPLE);

    /* initialize local state */
    VOS_INIT_3X3_DELTAS(deltas, step, 1);
    memcpy(deltas + 8, deltas, 8 * sizeof(deltas[0]));

    /* initialize writer */
    StartAppendToSeq(contour, &writer);

    s_end = s = VOS_IS_SEQ_HOLE(contour) ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
        if (*i1 != 0)
            break;
    } while (s != s_end);

    if (s == s_end) /* single pixel domain */
    {
        *i0 = (char)(nbd | -128);
        if (method >= 0)
        {
            VOS_WRITE_SEQ_ELEM(pt, writer);
        }
    }
    else
    {
        i3 = i0;
        prev_s = s ^ 4;

        /* follow border */
        for (;;)
        {
            s_end = s;

            for (;;)
            {
                i4 = i3 + deltas[++s];
                if (*i4 != 0)
                    break;
            }
            s &= 7;

            /* check "right" bound */
            if ((unsigned)(s - 1) < (unsigned)s_end)
            {
                *i3 = (char)(nbd | -128);
            }
            else if (*i3 == 1)
            {
                *i3 = nbd;
            }

            if (method < 0)
            {
                char _s = (char)s;

                VOS_WRITE_SEQ_ELEM(_s, writer);
            }
            else
            {
                if (s != prev_s || method == 0)
                {
                    VOS_WRITE_SEQ_ELEM(pt, writer);
                    prev_s = s;
                }

                pt.x += iCodeDeltas[s].x;
                pt.y += iCodeDeltas[s].y;
            }

            if (i4 == i0 && i3 == i1)
                break;

            i3 = i4;
            s = (s + 4) & 7;
        } /* end of border following loop */
    }

    EndWriteSeq(&writer);

    if (_method != VOS_CHAIN_CODE)
        BoundingRect(contour, 1);

    assert(writer.seq->total == 0 && writer.seq->first == 0 ||
           writer.seq->total > writer.seq->first->count ||
           (writer.seq->first->prev == writer.seq->first &&
            writer.seq->first->next == writer.seq->first));

    return VOS_OK;
}

Seq *
FindNextContour(ContourScanner scanner)
{
    char *img0;
    char *img;
    int step;
    int width, height;
    int x, y;
    int prev;
    Point lnbd;
    Seq *contour = 0;
    int nbd;
    int mode;
    CvStatus result = (CvStatus)1;

    VOS_FUNCNAME("cvFindNextContour");

    __BEGIN__;

    if (!scanner)
        VOS_ERROR(VOS_StsNullPtr, "");
    iEndProcessContour(scanner);

    /* initialize local state */
    img0 = scanner->img0;
    img = scanner->img;
    step = scanner->img_step;
    x = scanner->pt.x;
    y = scanner->pt.y;
    width = scanner->img_size.width;
    height = scanner->img_size.height;
    mode = scanner->mode;
    lnbd = scanner->lnbd;
    nbd = scanner->nbd;

    prev = img[x - 1];

    for (; y < height; y++, img += step)
    {
        for (; x < width; x++)
        {
            int p = img[x];

            if (p != prev)
            {
                _ContourInfo *par_info = 0;
                _ContourInfo *l_cinfo = 0;
                Seq *seq = 0;
                int is_hole = 0;
                Point origin;

                if (!(prev == 0 && p == 1)) /* if not external contour */
                {
                    /* check hole */
                    if (p != 0 || prev < 1)
                        goto resume_scan;

                    if (prev & -2)
                    {
                        lnbd.x = x - 1;
                    }
                    is_hole = 1;
                }

                if (mode == 0 && (is_hole || img0[lnbd.y * step + lnbd.x] > 0))
                    goto resume_scan;

                origin.y = y;
                origin.x = x - is_hole;

                /* find contour parent */
                if (mode <= 1 || (!is_hole && mode == 2) || lnbd.x <= 0)
                {
                    par_info = &(scanner->frame_info);
                }
                else
                {
                    VOS_ERROR(VOS_StsNullPtr, "");
                }

                lnbd.x = x - is_hole;

                SaveMemStoragePos(scanner->storage2, &(scanner->backup_pos));

                seq = CreateSeq(scanner->seq_type1, scanner->header_size1,
                                  scanner->elem_size1, scanner->storage1);
                if (!seq)
                {
                    result = VOS_OUTOFMEM_ERR;
                    goto exit_func;
                }
                seq->flags |= is_hole ? VOS_SEQ_FLAG_HOLE : 0;

                /* initialize header */
                if (mode <= 1)
                {
                    l_cinfo = &(scanner->cinfo_temp);
                    result = iFetchContour(img + x - is_hole, step,
                                             InitPoint(origin.x + scanner->offset.x,
                                                     origin.y + scanner->offset.y),
                                             seq, scanner->approx_method1);
                    if (result < 0)
                        goto exit_func;
                }
                else
                {
                     VOS_ERROR(VOS_StsNullPtr, "");
                }

                l_cinfo->is_hole = is_hole;
                l_cinfo->contour = seq;
                l_cinfo->origin = origin;
                l_cinfo->parent = par_info;

                if (scanner->approx_method1 != scanner->approx_method2)
                {
                    VOS_ERROR(VOS_StsNotImplemented, "herelsp remove");
                }

                l_cinfo->contour->v_prev = l_cinfo->parent->contour;

                if (par_info->contour == 0)
                {
                    l_cinfo->contour = 0;
                    if (scanner->storage1 == scanner->storage2)
                    {
                        RestoreMemStoragePos(scanner->storage1, &(scanner->backup_pos));
                    }
                    else
                    {
                        ClearMemStorage(scanner->storage1);
                    }
                    p = img[x];
                    goto resume_scan;
                }

                SaveMemStoragePos(scanner->storage2, &(scanner->backup_pos2));
                scanner->l_cinfo = l_cinfo;
                scanner->pt.x = x + 1;
                scanner->pt.y = y;
                scanner->lnbd = lnbd;
                scanner->img = (char *)img;
                scanner->nbd = nbd;
                contour = l_cinfo->contour;

                result = VOS_OK;
                goto exit_func;
            resume_scan:
                prev = p;
                /* update lnbd */
                if (prev & -2)
                {
                    lnbd.x = x;
                }
            } /* end of prev != p */
        }     /* end of loop on x */

        lnbd.x = 0;
        lnbd.y = y + 1;
        x = 1;
        prev = 0;

    } /* end of loop on y */

exit_func:

    if (result != 0)
        contour = 0;
    if (result < 0)
        VOS_ERROR_FROM_STATUS(result);

    __END__;

    return contour;
}

/*
   The function add to tree the last retrieved/substituted contour,
   releases temp_storage, restores state of dst_storage (if needed), and
   returns pointer to root of the contour tree */
Seq *
EndFindContours(ContourScanner *_scanner)
{
    ContourScanner scanner;
    Seq *first = 0;

    VOS_FUNCNAME("cvFindNextContour");

    __BEGIN__;

    if (!_scanner)
        VOS_ERROR(VOS_StsNullPtr, "");
    scanner = *_scanner;

    if (scanner)
    {
        iEndProcessContour(scanner);

        if (scanner->storage1 != scanner->storage2)
            ReleaseMemStorage(&(scanner->storage1));

        if (scanner->cinfo_storage)
            ReleaseMemStorage(&(scanner->cinfo_storage));

        first = scanner->frame.v_next;
        SYS_FREE(_scanner);
    }

    __END__;

    return first;
}

int FindContours(void *img, MemStorage *storage,
                   Seq **firstContour, int cntHeaderSize,
                   int mode,
                   int method, Point offset)
{
    ContourScanner scanner = 0;
    Seq *contour = 0;
    int count = -1;

    VOS_FUNCNAME("FindContours");

    __BEGIN__;

    if (!firstContour)
        VOS_ERROR(VOS_StsNullPtr, "NULL double Seq pointer");

    VOS_CALL(scanner = StartFindContours(img, storage,
                                           cntHeaderSize, mode, method, offset));
    assert(scanner);

    do
    {
        count++;
        contour = FindNextContour(scanner);
    } while (contour != 0);

    *firstContour = EndFindContours(&scanner);

    __END__;

    return count;
}

/* End of file. */

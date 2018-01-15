#include "core.h"
#include "misc.h"

static const signed char iDepthToType[] =
{
    -1, -1, VOS_8U, VOS_8S, VOS_16U, VOS_16S, -1, -1,
    VOS_32F, VOS_32S, -1, -1, -1, -1, -1, -1, VOS_64F, -1};

#define iIplToDepth(depth) iDepthToType[(((depth)&255) >> 2) + ((depth) < 0)]

AutoBuffer *CreateAutoBuffer(int height, int width, int type)
{
    AutoBuffer *arr = NULL;

    VOS_FUNCNAME("CreateAutoBuffer");

    __BEGIN__;

    VOS_CALL(arr = CreateAutoBufferHeader(height, width, type));
    VOS_CALL(CreateData(arr));

    __END__;

    if (GetErrStatus() < 0)
        ReleaseAutoBuffer(&arr);

    return arr;
}

static void iCheckHuge(AutoBuffer *arr)
{
    if ((int64)arr->step * arr->rows > INT_MAX)
        arr->type &= ~VOS_BUFFER_CONT_FLAG;
}

AutoBuffer *CreateAutoBufferHeader(int rows, int cols, int type)
{
    AutoBuffer *arr = NULL;

    VOS_FUNCNAME("CreateAutoBufferHeader");

    __BEGIN__;

    int min_step;
    type = VOS_BUFFER_TYPE(type);

    if (rows <= 0 || cols <= 0)
        VOS_ERROR(VOS_StsBadSize, "");

    min_step = VOS_ELEM_SIZE(type) * cols;
    if (min_step <= 0)
        VOS_ERROR(VOS_StsUnsupportedFormat, "");

    VOS_CALL(arr = (AutoBuffer *)SysAlloc(sizeof(*arr)));

    arr->step = rows == 1 ? 0 : SysAlign(min_step, VOS_DEFAULT_BUFFER_ROW_ALIGN);
    arr->type = VOS_BUFFER_MAGIC_VAL | type |
            (arr->step == 0 || arr->step == min_step ? VOS_BUFFER_CONT_FLAG : 0);
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = NULL;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    iCheckHuge(arr);

    __END__;

    if (GetErrStatus() < 0)
        ReleaseAutoBuffer(&arr);

    return arr;
}

AutoBuffer *InitAutoBufferHeader(AutoBuffer *arr, int rows, int cols,
                                 int type, void *data, int step)
{
    VOS_FUNCNAME("InitAutoBufferHeader");

    __BEGIN__;

    int mask, pix_size, min_step;

    if (!arr)
        VOS_ERROR_FROM_CODE(VOS_StsNullPtr);

    if ((unsigned)VOS_BUFFER_DEPTH(type) > VOS_DEPTH_MAX)
        VOS_ERROR_FROM_CODE(VOS_BadNumChannels);

    if (rows <= 0 || cols <= 0)
        VOS_ERROR(VOS_StsBadSize, "");

    type = VOS_BUFFER_TYPE(type);
    arr->type = type | VOS_BUFFER_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar *)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    mask = (arr->rows <= 1) - 1;
    pix_size = VOS_ELEM_SIZE(type);
    min_step = arr->cols * pix_size & mask;

    if (VOS_AUTOSTEP != step && 0 != step)
    {
        if (step < min_step)
            VOS_ERROR_FROM_CODE(VOS_BadStep);
        arr->step = step & mask;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = VOS_BUFFER_MAGIC_VAL | type |
            (arr->step == min_step ? VOS_BUFFER_CONT_FLAG : 0);

    iCheckHuge(arr);

    __END__;

    return arr;
}

void ReleaseAutoBuffer(AutoBuffer **array)
{
    VOS_FUNCNAME("ReleaseAutoBuffer");

    __BEGIN__;

    if (!array)
        VOS_ERROR_FROM_CODE(VOS_HeaderIsNull);

    if (*array)
    {
        AutoBuffer *arr = *array;

        if (!VOS_IS_BUFFER_HDR(arr))
            VOS_ERROR_FROM_CODE(VOS_StsBadFlag);

        *array = NULL;

        DecRefData(arr);
        SYS_FREE(&arr);
    }

    __END__;
}

void CreateData(VOID *arr)
{
    VOS_FUNCNAME("CreateData");

    __BEGIN__;

    if (VOS_IS_BUFFER_HDR(arr))
    {
        size_t step, total_size;
        AutoBuffer *mat = (AutoBuffer *)arr;
        step = mat->step;

        if (NULL != mat->data.ptr)
            VOS_ERROR(VOS_StsError, "");

        if (0 == step)
            step = VOS_ELEM_SIZE(mat->type) * mat->cols;

        total_size = step * mat->rows + sizeof(int) + VOS_MALLOC_ALIGN;
        VOS_CALL(mat->refcount = (int *)SysAlloc((size_t)total_size));
        mat->data.ptr = (uchar *)SysAlignPtr(mat->refcount + 1, VOS_MALLOC_ALIGN);
        *mat->refcount = 1;
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        BmpImage *img = (BmpImage *)arr;

        if (NULL != img->imageData)
            VOS_ERROR(VOS_StsError, "");

        VOS_CALL(img->imageData = img->imageDataOrigin =
                (char *)SysAlloc((size_t)img->imageSize));
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "");
    }

    __END__;
}

void SetData(VOID *arr, void *data, int step)
{
    VOS_FUNCNAME("SetData");

    __BEGIN__;

    int pix_size, min_step;

    if (VOS_IS_BUFFER_HDR(arr))
        ReleaseData(arr);

    if (VOS_IS_BUFFER_HDR(arr))
    {
        AutoBuffer *mat = (AutoBuffer *)arr;

        int type = VOS_BUFFER_TYPE(mat->type);
        pix_size = VOS_ELEM_SIZE(type);
        min_step = mat->cols * pix_size & ((mat->rows <= 1) - 1);

        if (VOS_AUTOSTEP != step)
        {
            if (step < min_step && NULL != data)
                VOS_ERROR_FROM_CODE(VOS_BadStep);
            mat->step = step & ((mat->rows <= 1) - 1);
        }
        else
        {
            mat->step = min_step;
        }

        mat->data.ptr = (uchar *)data;
        mat->type = VOS_BUFFER_MAGIC_VAL | type |
                (mat->step == min_step ? VOS_BUFFER_CONT_FLAG : 0);
        iCheckHuge(mat);
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        BmpImage *img = (BmpImage *)arr;

        pix_size = ((img->depth & 255) >> 3) * img->nChannels;
        min_step = img->width * pix_size;

        if (VOS_AUTOSTEP != step && img->height > 1)
        {
            if (step < min_step && NULL != data)
                VOS_ERROR_FROM_CODE(VOS_BadStep);
            img->widthStep = step;
        }
        else
        {
            img->widthStep = min_step;
        }

        img->imageSize = img->widthStep * img->height;
        img->imageData = img->imageDataOrigin = (char *)data;

        if ((((int)(size_t)data | step) & 7) == 0 &&
                SysAlign(img->width * pix_size, 8) == step)
        {
            img->align = 8;
        }
        else
        {
            img->align = 4;
        }
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "");
    }

    __END__;
}

void ReleaseData(VOID *arr)
{
    VOS_FUNCNAME("ReleaseData");

    __BEGIN__;

    if (VOS_IS_BUFFER_HDR(arr))
    {
        AutoBuffer *mat = (AutoBuffer *)arr;
        DecRefData(mat);
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        BmpImage *img = (BmpImage *)arr;

        char *ptr = img->imageDataOrigin;
        img->imageData = img->imageDataOrigin = NULL;
        SYS_FREE(&ptr);
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "");
    }

    __END__;
}

Size GetSize(const VOID *arr)
{
    Size size = {0, 0};

    VOS_FUNCNAME("GetSize");

    __BEGIN__;

    if (VOS_IS_BUFFER_HDR(arr))
    {
        AutoBuffer *mat = (AutoBuffer *)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        BmpImage *img = (BmpImage *)arr;

        size.width = img->width;
        size.height = img->height;
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "");
    }

    __END__;

    return size;
}

void ScalarToRawData(const Scalar *scalar, void *data, int type)
{
    VOS_FUNCNAME("ScalarToRawData");

    type = VOS_BUFFER_TYPE(type);

    __BEGIN__;

    int cn = VOS_BUFFER_CN(type);
    int depth = type & VOS_BUFFER_DEPTH_MASK;

    assert(scalar && data);
    if ((unsigned)(cn - 1) >= 4)
        VOS_ERROR(VOS_StsOutOfRange, "");

    if (VOS_8UC1 != depth)
    {
        VOS_ERROR_FROM_CODE(VOS_BadDepth);
    }

    while (cn--)
    {
        int t = SysRound(scalar->val[cn]);
        ((uchar *)data)[cn] = VOS_CAST_8U(t);
    }
    __END__;
}

AutoBuffer *GetAutoBuffer(const VOID *array, AutoBuffer *mat,
                          int *pCOI)
{
    AutoBuffer *result = NULL;
    AutoBuffer *src = (AutoBuffer *)array;
    int coi = 0;

    VOS_FUNCNAME("GetAutoBuffer");

    __BEGIN__;

    if (!mat || !src)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (VOS_IS_BUFFER_HDR(src))
    {
        if (!src->data.ptr)
            VOS_ERROR(VOS_StsNullPtr, "");

        result = (AutoBuffer *)src;
    }
    else if (VOS_IS_IMAGE_HDR(src))
    {
        const BmpImage *img = (const BmpImage *)src;
        int depth, order;

        if (NULL == img->imageData)
            VOS_ERROR(VOS_StsNullPtr, "");

        depth = iIplToDepth(img->depth);
        if (depth < 0)
            VOS_ERROR_FROM_CODE(VOS_BadDepth);

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);
        {
            int type = VOS_MAKETYPE(depth, img->nChannels);

            if (order != SYS_DATA_ORDER_PIXEL)
                VOS_ERROR(VOS_StsBadFlag, "");

            VOS_CALL(InitAutoBufferHeader(mat, img->height, img->width, type,
                                          img->imageData, img->widthStep));
        }

        result = mat;
    }
    else
    {
        VOS_ERROR(VOS_StsBadFlag, "");
    }

    __END__;

    if (pCOI)
        *pCOI = coi;

    return result;
}

BmpImage *
CreateImageHeader(Size size, int depth, int channels)
{
    BmpImage *img = NULL;

    VOS_FUNCNAME("CreateImageHeader");

    __BEGIN__;

    VOS_CALL(img = (BmpImage *)SysAlloc(sizeof(*img)));
    VOS_CALL(InitImageHeader(img, size, depth, channels, SYS_ORIGIN_TL,
                             VOS_DEFAULT_IMAGE_ROW_ALIGN));

    __END__;

    if (GetErrStatus() < 0 && img)
        ReleaseImageHeader(&img);

    return img;
}

BmpImage *
CreateImage(Size size, int depth, int channels)
{
    BmpImage *img = NULL;

    VOS_FUNCNAME("CreateImage");

    __BEGIN__;

    VOS_CALL(img = CreateImageHeader(size, depth, channels));
    assert(img);
    VOS_CALL(CreateData(img));

    __END__;

    if (GetErrStatus() < 0)
        ReleaseImage(&img);

    return img;
}

BmpImage *
InitImageHeader(BmpImage *image, Size size, int depth,
                int channels, int origin, int align)
{
    BmpImage *result = NULL;

    VOS_FUNCNAME("InitImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_HeaderIsNull, "");

    VOS_MEMSET(image, 0, sizeof(*image));
    image->nSize = sizeof(*image);

    if (size.width < 0 || size.height < 0)
        VOS_ERROR(VOS_BadROISize, "");

    if (((depth != (int)SYS_DEPTH_8U) && ((depth != (int)SYS_DEPTH_32F))) ||
            (channels < 0))
        VOS_ERROR(VOS_BadDepth, "");
    if (origin != VOS_ORIGIN_BL && origin != VOS_ORIGIN_TL)
        VOS_ERROR(VOS_BadOrigin, "");

    if (align != 4 && align != 8)
        VOS_ERROR(VOS_BadAlign, "");

    image->width = size.width;
    image->height = size.height;

    image->nChannels = VOS_MAX(channels, 1);
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
                          (image->depth & ~SYS_DEPTH_SIGN) +
                          7) /
                         8) +
                        align - 1) &
            (~(align - 1));
    image->origin = origin;
    image->imageSize = image->widthStep * image->height;

    result = image;

    __END__;

    return result;
}

void ReleaseImageHeader(BmpImage **image)
{
    VOS_FUNCNAME("ReleaseImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        BmpImage *img = *image;
        *image = NULL;

        SYS_FREE(&img);
    }
    __END__;
}

void ReleaseImage(BmpImage **image)
{
    VOS_FUNCNAME("ReleaseImage");

    __BEGIN__

            if (!image)
            VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        BmpImage *img = *image;
        *image = NULL;

        ReleaseData(img);
        ReleaseImageHeader(&img);
    }

    __END__;
}

BmpImage *CloneImage(const BmpImage *src)
{
    BmpImage *dst = NULL;
    VOS_FUNCNAME("CloneImage");

    __BEGIN__;

    if (!VOS_IS_IMAGE_HDR(src))
        VOS_ERROR(VOS_StsBadArg, "");

    VOS_CALL(dst = (BmpImage *)SysAlloc(sizeof(*dst)));

    VOS_MEMCPY(dst, src, sizeof(*src));
    dst->imageData = dst->imageDataOrigin = NULL;

    if (src->imageData)
    {
        int size = src->imageSize;
        CreateData(dst);
        if (NULL != dst->imageData)
        {
            VOS_MEMCPY(dst->imageData, src->imageData, size);
        }
    }

    __END__;

    return dst;
}

/* End of file. */

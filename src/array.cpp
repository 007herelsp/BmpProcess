#include "_cxcore.h"

static const signed char iDepthToType[] =
    {
        -1, -1, VOS_8U, VOS_8S, VOS_16U, VOS_16S, -1, -1,
        VOS_32F, VOS_32S, -1, -1, -1, -1, -1, -1, VOS_64F, -1};

#define icvIplToCvDepth(depth) \
    iDepthToType[(((depth)&255) >> 2) + ((depth) < 0)]

/****************************************************************************************\
*                               Mat creation and basic operations                      *
\****************************************************************************************/

// Creates Mat and underlying data
 Mat *
CreateMat(int height, int width, int type)
{
    Mat *arr = 0;

    VOS_FUNCNAME("CreateMat");

    __BEGIN__;

    VOS_CALL(arr = CreateMatHeader(height, width, type));
    VOS_CALL(CreateData(arr));

    __END__;

    if (GetErrStatus() < 0)
        ReleaseMat(&arr);

    return arr;
}

static void iCheckHuge(Mat *arr)
{
    if ((int64)arr->step * arr->rows > INT_MAX)
        arr->type &= ~VOS_MAT_CONT_FLAG;
}

// Creates Mat header only
 Mat *
CreateMatHeader(int rows, int cols, int type)
{
    Mat *arr = 0;

    VOS_FUNCNAME("CreateMatHeader");

    __BEGIN__;

    int min_step;
    type = VOS_MAT_TYPE(type);

    if (rows <= 0 || cols <= 0)
        VOS_ERROR(VOS_StsBadSize, "Non-positive width or height");

    min_step = VOS_ELEM_SIZE(type) * cols;
    if (min_step <= 0)
        VOS_ERROR(VOS_StsUnsupportedFormat, "Invalid matrix type");

    VOS_CALL(arr = (Mat *)SysAlloc(sizeof(*arr)));

    arr->step = rows == 1 ? 0 : Align(min_step, VOS_DEFAULT_MAT_ROW_ALIGN);
    arr->type = VOS_MAT_MAGIC_VAL | type |
                (arr->step == 0 || arr->step == min_step ? VOS_MAT_CONT_FLAG : 0);
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    iCheckHuge(arr);

    __END__;

    if (GetErrStatus() < 0)
        ReleaseMat(&arr);

    return arr;
}

// Initializes Mat header, allocated by the user
 Mat *
InitMatHeader(Mat *arr, int rows, int cols,
                int type, void *data, int step)
{
    VOS_FUNCNAME("InitMatHeader");

    __BEGIN__;

    int mask, pix_size, min_step;

    if (!arr)
        VOS_ERROR_FROM_CODE(VOS_StsNullPtr);

    if ((unsigned)VOS_MAT_DEPTH(type) > VOS_DEPTH_MAX)
        VOS_ERROR_FROM_CODE(VOS_BadNumChannels);

    if (rows <= 0 || cols <= 0)
        VOS_ERROR(VOS_StsBadSize, "Non-positive cols or rows");

    type = VOS_MAT_TYPE(type);
    arr->type = type | VOS_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar *)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    mask = (arr->rows <= 1) - 1;
    pix_size = VOS_ELEM_SIZE(type);
    min_step = arr->cols * pix_size & mask;

    if (step != VOS_AUTOSTEP && step != 0)
    {
        if (step < min_step)
            VOS_ERROR_FROM_CODE(VOS_BadStep);
        arr->step = step & mask;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = VOS_MAT_MAGIC_VAL | type |
                (arr->step == min_step ? VOS_MAT_CONT_FLAG : 0);

    iCheckHuge(arr);

    __END__;

    return arr;
}

// Deallocates the Mat structure and underlying data
 void
ReleaseMat(Mat **array)
{
    VOS_FUNCNAME("ReleaseMat");

    __BEGIN__;

    if (!array)
        VOS_ERROR_FROM_CODE(VOS_HeaderIsNull);

    if (*array)
    {
        Mat *arr = *array;

        if (!VOS_IS_MAT_HDR(arr))
            VOS_ERROR_FROM_CODE(VOS_StsBadFlag);

        *array = 0;

        DecRefData(arr);
        SYS_FREE(&arr);
    }

    __END__;
}

/****************************************************************************************\
*                          Common for multiple array types operations                    *
\****************************************************************************************/

// Allocates underlying array data
 void
CreateData(CvArr *arr)
{
    VOS_FUNCNAME("CreateData");

    __BEGIN__;

    if (VOS_IS_MAT_HDR(arr))
    {
        size_t step, total_size;
        Mat *mat = (Mat *)arr;
        step = mat->step;

        if (mat->data.ptr != 0)
            VOS_ERROR(VOS_StsError, "Data is already allocated");

        if (step == 0)
            step = VOS_ELEM_SIZE(mat->type) * mat->cols;

        total_size = step * mat->rows + sizeof(int) + VOS_MALLOC_ALIGN;
        VOS_CALL(mat->refcount = (int *)SysAlloc((size_t)total_size));
        mat->data.ptr = (uchar *)AlignPtr(mat->refcount + 1, VOS_MALLOC_ALIGN);
        *mat->refcount = 1;
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        if (img->imageData != 0)
            VOS_ERROR(VOS_StsError, "Data is already allocated");

        VOS_CALL(img->imageData = img->imageDataOrigin =
                     (char *)SysAlloc((size_t)img->imageSize));
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "unrecognized or unsupported array type");
    }

    __END__;
}

// Assigns external data to array
 void
SetData(CvArr *arr, void *data, int step)
{
    VOS_FUNCNAME("SetData");

    __BEGIN__;

    int pix_size, min_step;

    if (VOS_IS_MAT_HDR(arr))
        ReleaseData(arr);

    if (VOS_IS_MAT_HDR(arr))
    {
        Mat *mat = (Mat *)arr;

        int type = VOS_MAT_TYPE(mat->type);
        pix_size = VOS_ELEM_SIZE(type);
        min_step = mat->cols * pix_size & ((mat->rows <= 1) - 1);

        if (step != VOS_AUTOSTEP)
        {
            if (step < min_step && data != 0)
                VOS_ERROR_FROM_CODE(VOS_BadStep);
            mat->step = step & ((mat->rows <= 1) - 1);
        }
        else
        {
            mat->step = min_step;
        }

        mat->data.ptr = (uchar *)data;
        mat->type = VOS_MAT_MAGIC_VAL | type |
                    (mat->step == min_step ? VOS_MAT_CONT_FLAG : 0);
        iCheckHuge(mat);
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        pix_size = ((img->depth & 255) >> 3) * img->nChannels;
        min_step = img->width * pix_size;

        if (step != VOS_AUTOSTEP && img->height > 1)
        {
            if (step < min_step && data != 0)
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
            Align(img->width * pix_size, 8) == step)
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
        VOS_ERROR(VOS_StsBadArg, "unrecognized or unsupported array type");
    }

    __END__;
}

// Deallocates array's data
 void
ReleaseData(CvArr *arr)
{
    VOS_FUNCNAME("ReleaseData");

    __BEGIN__;

    if (VOS_IS_MAT_HDR(arr))
    {
        Mat *mat = (Mat *)arr;
        DecRefData(mat);
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        char *ptr = img->imageDataOrigin;
        img->imageData = img->imageDataOrigin = 0;
        SYS_FREE(&ptr);
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "unrecognized or unsupported array type");
    }

    __END__;
}

 int
GetElemType(const CvArr *arr)
{
    int type = -1;

    VOS_FUNCNAME("GetElemType");

    __BEGIN__;

    if (VOS_IS_MAT_HDR(arr))
    {
        type = VOS_MAT_TYPE(((Mat *)arr)->type);
    }
    else if (VOS_IS_IMAGE(arr))
    {
        IplImage *img = (IplImage *)arr;
        type = VOS_MAKETYPE(icvIplToCvDepth(img->depth), img->nChannels);
    }
    else
        VOS_ERROR(VOS_StsBadArg, "unrecognized or unsupported array type");

    __END__;

    return type;
}

// Returns the size of Mat or IplImage
 Size
GetSize(const CvArr *arr)
{
    Size size = {0, 0};

    VOS_FUNCNAME("GetSize");

    __BEGIN__;

    if (VOS_IS_MAT_HDR(arr))
    {
        Mat *mat = (Mat *)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        size.width = img->width;
        size.height = img->height;
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "Array should be Mat or IplImage");
    }

    __END__;

    return size;
}

/****************************************************************************************\
*                      Operations on Scalar and accessing array elements               *
\****************************************************************************************/

// Converts Scalar to specified type
 void
ScalarToRawData(const Scalar *scalar, void *data, int type)
{
    VOS_FUNCNAME("ScalarToRawData");

    type = VOS_MAT_TYPE(type);

    __BEGIN__;

    int cn = VOS_MAT_CN(type);
    int depth = type & VOS_MAT_DEPTH_MASK;

    assert(scalar && data);
    if ((unsigned)(cn - 1) >= 4)
        VOS_ERROR(VOS_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4");

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

/****************************************************************************************\
*                             Conversion to Mat or IplImage                            *
\****************************************************************************************/

// convert array (Mat or IplImage) to Mat
 Mat *
GetMat(const CvArr *array, Mat *mat,
         int *pCOI, int allowND)
{
    Mat *result = 0;
    Mat *src = (Mat *)array;
    int coi = 0;

    VOS_FUNCNAME("GetMat");

    __BEGIN__;

    if (!mat || !src)
        VOS_ERROR(VOS_StsNullPtr, "NULL array pointer is passed");

    if (VOS_IS_MAT_HDR(src))
    {
        if (!src->data.ptr)
            VOS_ERROR(VOS_StsNullPtr, "The matrix has NULL data pointer");

        result = (Mat *)src;
    }
    else if (VOS_IS_IMAGE_HDR(src))
    {
        const IplImage *img = (const IplImage *)src;
        int depth, order;

        if (img->imageData == 0)
            VOS_ERROR(VOS_StsNullPtr, "The image has NULL data pointer");

        depth = icvIplToCvDepth(img->depth);
        if (depth < 0)
            VOS_ERROR_FROM_CODE(VOS_BadDepth);

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);
        {
            int type = VOS_MAKETYPE(depth, img->nChannels);

            if (order != IPL_DATA_ORDER_PIXEL)
                VOS_ERROR(VOS_StsBadFlag, "Pixel order should be used with coi == 0");

            VOS_CALL(InitMatHeader(mat, img->height, img->width, type,
                                     img->imageData, img->widthStep));
        }

        result = mat;
    }
    else
    {
        VOS_ERROR(VOS_StsBadFlag, "Unrecognized or unsupported array type");
    }

    __END__;

    if (pCOI)
        *pCOI = coi;

    return result;
}

/****************************************************************************************\
*                               IplImage-specific functions                              *
\****************************************************************************************/

// create IplImage header
 IplImage *
CreateImageHeader(Size size, int depth, int channels)
{
    IplImage *img = 0;

    VOS_FUNCNAME("CreateImageHeader");

    __BEGIN__;

    VOS_CALL(img = (IplImage *)SysAlloc(sizeof(*img)));
    VOS_CALL(InitImageHeader(img, size, depth, channels, IPL_ORIGIN_TL,
                               VOS_DEFAULT_IMAGE_ROW_ALIGN));

    __END__;

    if (GetErrStatus() < 0 && img)
        ReleaseImageHeader(&img);

    return img;
}

// create IplImage header and allocate underlying data
 IplImage *
CreateImage(Size size, int depth, int channels)
{
    IplImage *img = 0;

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

// initalize IplImage header, allocated by the user
 IplImage *
InitImageHeader(IplImage *image, Size size, int depth,
                  int channels, int origin, int align)
{
    IplImage *result = 0;

    VOS_FUNCNAME("InitImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_HeaderIsNull, "null pointer to header");

    memset(image, 0, sizeof(*image));
    image->nSize = sizeof(*image);

    if (size.width < 0 || size.height < 0)
        VOS_ERROR(VOS_BadROISize, "Bad input roi");

    if ((depth != (int)IPL_DEPTH_8U) ||
        (channels < 0))
        VOS_ERROR(VOS_BadDepth, "Unsupported format");
    if (origin != VOS_ORIGIN_BL && origin != VOS_ORIGIN_TL)
        VOS_ERROR(VOS_BadOrigin, "Bad input origin");

    if (align != 4 && align != 8)
        VOS_ERROR(VOS_BadAlign, "Bad input align");

    image->width = size.width;
    image->height = size.height;

    image->nChannels = MAX(channels, 1);
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
                              (image->depth & ~IPL_DEPTH_SIGN) +
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

 void
ReleaseImageHeader(IplImage **image)
{
    VOS_FUNCNAME("ReleaseImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = 0;

        SYS_FREE(&img);
    }
    __END__;
}

 void
ReleaseImage(IplImage **image)
{
    VOS_FUNCNAME("ReleaseImage");

    __BEGIN__

    if (!image)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = 0;

        ReleaseData(img);
        ReleaseImageHeader(&img);
    }

    __END__;
}

 IplImage *
CloneImage(const IplImage *src)
{
    IplImage *dst = 0;
    VOS_FUNCNAME("CloneImage");

    __BEGIN__;

    if (!VOS_IS_IMAGE_HDR(src))
        VOS_ERROR(VOS_StsBadArg, "Bad image header");

    VOS_CALL(dst = (IplImage *)SysAlloc(sizeof(*dst)));

    memcpy(dst, src, sizeof(*src));
    dst->imageData = dst->imageDataOrigin = 0;

    if (src->imageData)
    {
        int size = src->imageSize;
        CreateData(dst);
        memcpy(dst->imageData, src->imageData, size);
    }

    __END__;

    return dst;
}

/* End of file. */

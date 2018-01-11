#include "core.h"
#include "misc.h"

static const signed char iDepthToType[] =
    {
        -1, -1, VOS_8U, VOS_8S, VOS_16U, VOS_16S, -1, -1,
        VOS_32F, VOS_32S, -1, -1, -1, -1, -1, -1, VOS_64F, -1};

#define iIplToCvDepth(depth) \
    iDepthToType[(((depth)&255) >> 2) + ((depth) < 0)]

// Creates Mat and underlying data
Mat *CreateMat(int height, int width, int type)
{
    Mat *arr = NULL;

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
Mat *CreateMatHeader(int rows, int cols, int type)
{
    Mat *arr = NULL;

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
    arr->data.ptr = NULL;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    iCheckHuge(arr);

    __END__;

    if (GetErrStatus() < 0)
        ReleaseMat(&arr);

    return arr;
}

// Initializes Mat header, allocated by the user
Mat *InitMatHeader(Mat *arr, int rows, int cols,
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

    if ( VOS_AUTOSTEP!=step  &&  0!=step )
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
void ReleaseMat(Mat **array)
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

        *array = NULL;

        DecRefData(arr);
        SYS_FREE(&arr);
    }

    __END__;
}

/****************************************************************************************\
*                          Common for multiple array types operations                    *
\****************************************************************************************/

// Allocates underlying array data
void CreateData(VOID *arr)
{
    VOS_FUNCNAME("CreateData");

    __BEGIN__;

    if (VOS_IS_MAT_HDR(arr))
    {
        size_t step, total_size;
        Mat *mat = (Mat *)arr;
        step = mat->step;

        if ( NULL!=mat->data.ptr )
            VOS_ERROR(VOS_StsError, "Data is already allocated");

        if ( 0==step )
            step = VOS_ELEM_SIZE(mat->type) * mat->cols;

        total_size = step * mat->rows + sizeof(int) + VOS_MALLOC_ALIGN;
        VOS_CALL(mat->refcount = (int *)SysAlloc((size_t)total_size));
        mat->data.ptr = (uchar *)AlignPtr(mat->refcount + 1, VOS_MALLOC_ALIGN);
        *mat->refcount = 1;
    }
    else if (VOS_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        if ( NULL!=img->imageData )
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
void SetData(VOID *arr, void *data, int step)
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

        if ( VOS_AUTOSTEP!=step )
        {
            if (step < min_step &&  NULL!=data )
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

        if ( VOS_AUTOSTEP!=step  && img->height > 1)
        {
            if (step < min_step &&  NULL!=data )
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
void ReleaseData(VOID *arr)
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
        img->imageData = img->imageDataOrigin = NULL;
        SYS_FREE(&ptr);
    }
    else
    {
        VOS_ERROR(VOS_StsBadArg, "unrecognized or unsupported array type");
    }

    __END__;
}

// Returns the size of Mat or IplImage
Size GetSize(const VOID *arr)
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

void ScalarToRawData(const Scalar *scalar, void *data, int type)
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

// convert array (Mat or IplImage) to Mat
Mat *GetMat(const VOID *array, Mat *mat,
            int *pCOI)
{
    Mat *result = NULL;
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

        if ( NULL==img->imageData )
            VOS_ERROR(VOS_StsNullPtr, "The image has NULL data pointer");

        depth = iIplToCvDepth(img->depth);
        if (depth < 0)
            VOS_ERROR_FROM_CODE(VOS_BadDepth);

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);
        {
            int type = VOS_MAKETYPE(depth, img->nChannels);

            if (order != SYS_DATA_ORDER_PIXEL)
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

// create IplImage header
IplImage *
CreateImageHeader(Size size, int depth, int channels)
{
    IplImage *img = NULL;

    VOS_FUNCNAME("CreateImageHeader");

    __BEGIN__;

    VOS_CALL(img = (IplImage *)SysAlloc(sizeof(*img)));
    VOS_CALL(InitImageHeader(img, size, depth, channels, SYS_ORIGIN_TL,
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
    IplImage *img = NULL;

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
    IplImage *result = NULL;

    VOS_FUNCNAME("InitImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_HeaderIsNull, "null pointer to header");

    VOS_MEMSET(image, 0, sizeof(*image));
    image->nSize = sizeof(*image);

    if (size.width < 0 || size.height < 0)
        VOS_ERROR(VOS_BadROISize, "Bad input roi");

    if (((depth != (int)SYS_DEPTH_8U) && ((depth != (int)SYS_DEPTH_32F))) ||
        (channels < 0))
        VOS_ERROR(VOS_BadDepth, "Unsupported format");
    if (origin != VOS_ORIGIN_BL && origin != VOS_ORIGIN_TL)
        VOS_ERROR(VOS_BadOrigin, "Bad input origin");

    if (align != 4 && align != 8)
        VOS_ERROR(VOS_BadAlign, "Bad input align");

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

void ReleaseImageHeader(IplImage **image)
{
    VOS_FUNCNAME("ReleaseImageHeader");

    __BEGIN__;

    if (!image)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = NULL;

        SYS_FREE(&img);
    }
    __END__;
}

void ReleaseImage(IplImage **image)
{
    VOS_FUNCNAME("ReleaseImage");

    __BEGIN__

    if (!image)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = NULL;

        ReleaseData(img);
        ReleaseImageHeader(&img);
    }

    __END__;
}

IplImage *
CloneImage(const IplImage *src)
{
    IplImage *dst = NULL;
    VOS_FUNCNAME("CloneImage");

    __BEGIN__;

    if (!VOS_IS_IMAGE_HDR(src))
        VOS_ERROR(VOS_StsBadArg, "Bad image header");

    VOS_CALL(dst = (IplImage *)SysAlloc(sizeof(*dst)));

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

/************************************************* 
Function:      通过直方图变换进行图像增强，将图像灰度的域值拉伸到0-255 
src1:               单通道灰度图像                   
dst1:              同样大小的单通道灰度图像  
*************************************************/

int ImageStretchByHistogram(IplImage *src1, IplImage *dst1)

{
    assert(src1->width == dst1->width);
    double p[256], p1[256], num[256];

    memset(p, 0, sizeof(p));
    memset(p1, 0, sizeof(p1));
    memset(num, 0, sizeof(num));
    int height = src1->height;
    int width = src1->width;
    long wMulh = height * width;

    //statistics
    for (int x = 0; x < src1->width; x++)
    {
        for (int y = 0; y < src1->height; y++)
        {
            uchar v = ((uchar *)(src1->imageData + src1->widthStep * y))[x];
            num[v]++;
        }
    }
    //calculate probability
    for (int i = 0; i < 256; i++)
    {
        p[i] = num[i] / wMulh;
    }

    //p1[i]=sum(p[j]);  j<=i;
    for (int i = 0; i < 256; i++)
    {
        for (int k = 0; k <= i; k++)
            p1[i] += p[k];
    }

    // histogram transformation
    for (int x = 0; x < src1->width; x++)
    {
        for (int y = 0; y < src1->height; y++)
        {
            uchar v = ((uchar *)(src1->imageData + src1->widthStep * y))[x];
            ((uchar *)(dst1->imageData + dst1->widthStep * y))[x] = p1[v] * 255 + 0.5;
        }
    }
    return 0;
}

/* End of file. */

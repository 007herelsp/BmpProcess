#include "core.precomp.hpp"

static struct
{
    Cv_iplCreateImageHeader createHeader;
    Cv_iplAllocateImageData allocateData;
    Cv_iplDeallocate deallocate;
    Cv_iplCloneImage cloneImage;
} CvIPL;

/****************************************************************************************\
*                               CvMat creation and basic operations                      *
\****************************************************************************************/

// Creates CvMat and underlying data
CV_IMPL CvMat *
cvCreateMat(int height, int width, int type)
{
    CvMat *arr = cvCreateMatHeader(height, width, type);
    cvCreateData(arr);

    return arr;
}

static void icvCheckHuge(CvMat *arr)
{
    if ((int64)arr->step * arr->rows > INT_MAX)
        arr->type &= ~CV_MAT_CONT_FLAG;
}

// Creates CvMat header only
CV_IMPL CvMat *
cvCreateMatHeader(int rows, int cols, int type)
{
    type = CV_MAT_TYPE(type);

    if (rows < 0 || cols <= 0)
        CV_Error(CV_StsBadSize, "Non-positive width or height");

    int min_step = CV_ELEM_SIZE(type) * cols;
    if (min_step <= 0)
        CV_Error(CV_StsUnsupportedFormat, "Invalid matrix type");

    CvMat *arr = (CvMat *)cvAlloc(sizeof(*arr));

    arr->step = min_step;
    arr->type = CV_MAT_MAGIC_VAL | type | CV_MAT_CONT_FLAG;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    icvCheckHuge(arr);
    return arr;
}

// Initializes CvMat header, allocated by the user
CV_IMPL CvMat *
cvInitMatHeader(CvMat *arr, int rows, int cols,
                int type, void *data, int step)
{
    if (!arr)
        CV_Error(CV_StsNullPtr, "");

    if ((unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX)
        CV_Error(CV_BadNumChannels, "");

    if (rows < 0 || cols <= 0)
        CV_Error(CV_StsBadSize, "Non-positive cols or rows");

    type = CV_MAT_TYPE(type);
    arr->type = type | CV_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar *)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    int pix_size = CV_ELEM_SIZE(type);
    int min_step = arr->cols * pix_size;

    if (step != CV_AUTOSTEP && step != 0)
    {
        if (step < min_step)
            CV_Error(CV_BadStep, "");
        arr->step = step;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->rows == 1 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

    icvCheckHuge(arr);
    return arr;
}

// Deallocates the CvMat structure and underlying data
CV_IMPL void
cvReleaseMat(CvMat **array)
{
    if (!array)
        CV_Error(CV_HeaderIsNull, "");

    if (*array)
    {
        CvMat *arr = *array;

        if (!CV_IS_MAT_HDR_Z(arr))
            CV_Error(CV_StsBadFlag, "");

        *array = 0;

        cvDecRefData(arr);
        cvFree(&arr);
    }
}


/****************************************************************************************\
*                          Common for multiple array types operations                    *
\****************************************************************************************/

// Allocates underlying array data
CV_IMPL void
cvCreateData(CvArr *arr)
{
    if (CV_IS_MAT_HDR_Z(arr))
    {
        size_t step, total_size;
        CvMat *mat = (CvMat *)arr;
        step = mat->step;

        if (mat->rows == 0 || mat->cols == 0)
            return;

        if (mat->data.ptr != 0)
            CV_Error(CV_StsError, "Data is already allocated");

        if (step == 0)
            step = CV_ELEM_SIZE(mat->type) * mat->cols;

        int64 _total_size = (int64)step * mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
        total_size = (size_t)_total_size;
        if (_total_size != (int64)total_size)
            CV_Error(CV_StsNoMem, "Too big buffer is allocated");
        mat->refcount = (int *)cvAlloc((size_t)total_size);
        mat->data.ptr = (uchar *)cvAlignPtr(mat->refcount + 1, CV_MALLOC_ALIGN);
        *mat->refcount = 1;
    }
    else if (CV_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        if (img->imageData != 0)
            CV_Error(CV_StsError, "Data is already allocated");

        if (!CvIPL.allocateData)
        {
            img->imageData = img->imageDataOrigin =
                (char *)cvAlloc((size_t)img->imageSize);
        }
        else
        {
            int depth = img->depth;
            int width = img->width;

            if (img->depth == IPL_DEPTH_32F || img->depth == IPL_DEPTH_64F)
            {
                img->width *= img->depth == IPL_DEPTH_32F ? sizeof(float) : sizeof(double);
                img->depth = IPL_DEPTH_8U;
            }

            CvIPL.allocateData(img, 0, 0);

            img->width = width;
            img->depth = depth;
        }
    }
    else
        CV_Error(CV_StsBadArg, "unrecognized or unsupported array type");
}

// Assigns external data to array
CV_IMPL void
cvSetData(CvArr *arr, void *data, int step)
{
    int pix_size, min_step;

    if (CV_IS_MAT_HDR(arr))
        cvReleaseData(arr);

    if (CV_IS_MAT_HDR(arr))
    {
        CvMat *mat = (CvMat *)arr;

        int type = CV_MAT_TYPE(mat->type);
        pix_size = CV_ELEM_SIZE(type);
        min_step = mat->cols * pix_size;

        if (step != CV_AUTOSTEP && step != 0)
        {
            if (step < min_step && data != 0)
                CV_Error(CV_BadStep, "");
            mat->step = step;
        }
        else
            mat->step = min_step;

        mat->data.ptr = (uchar *)data;
        mat->type = CV_MAT_MAGIC_VAL | type |
                    (mat->rows == 1 || mat->step == min_step ? CV_MAT_CONT_FLAG : 0);
        icvCheckHuge(mat);
    }
    else if (CV_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        pix_size = ((img->depth & 255) >> 3) * img->nChannels;
        min_step = img->width * pix_size;

        if (step != CV_AUTOSTEP && img->height > 1)
        {
            if (step < min_step && data != 0)
                CV_Error(CV_BadStep, "");
            img->widthStep = step;
        }
        else
        {
            img->widthStep = min_step;
        }

        img->imageSize = img->widthStep * img->height;
        img->imageData = img->imageDataOrigin = (char *)data;

        if ((((int)(size_t)data | step) & 7) == 0 &&
            cvAlign(img->width * pix_size, 8) == step)
            img->align = 8;
        else
            img->align = 4;
    }
    else
        CV_Error(CV_StsBadArg, "unrecognized or unsupported array type");
}

// Deallocates array's data
CV_IMPL void
cvReleaseData(CvArr *arr)
{
    if (CV_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

        if (!CvIPL.deallocate)
        {
            char *ptr = img->imageDataOrigin;
            img->imageData = img->imageDataOrigin = 0;
            cvFree(&ptr);
        }
        else
        {
            CvIPL.deallocate(img, IPL_IMAGE_DATA);
        }
    }
    else
    {
        CV_Error(CV_StsBadArg, "unrecognized or unsupported array type");
    }
}

// Returns the size of CvMat or IplImage
CV_IMPL CvSize
cvGetSize(const CvArr *arr)
{
    CvSize size = {0, 0};

    if (CV_IS_MAT_HDR_Z(arr))
    {
        CvMat *mat = (CvMat *)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if (CV_IS_IMAGE_HDR(arr))
    {
        IplImage *img = (IplImage *)arr;

            size.width = img->width;
            size.height = img->height;
    }
    else
        CV_Error(CV_StsBadArg, "Array should be CvMat or IplImage");

    return size;
}

/****************************************************************************************\
*                             Conversion to CvMat or IplImage                            *
\****************************************************************************************/

// convert array (CvMat or IplImage) to CvMat
CV_IMPL CvMat *
cvGetMat(const CvArr *array, CvMat *mat,
         int *pCOI, int allowND)
{
    CvMat *result = 0;
    CvMat *src = (CvMat *)array;
    int coi = 0;

    if (!mat || !src)
        CV_Error(CV_StsNullPtr, "NULL array pointer is passed");

    if (CV_IS_MAT_HDR(src))
    {
        if (!src->data.ptr)
            CV_Error(CV_StsNullPtr, "The matrix has NULL data pointer");

        result = (CvMat *)src;
    }
    else if (CV_IS_IMAGE_HDR(src))
    {
        const IplImage *img = (const IplImage *)src;
        int depth, order;

        if (img->imageData == 0)
            CV_Error(CV_StsNullPtr, "The image has NULL data pointer");

        depth = IPL2CV_DEPTH(img->depth);
        if (depth < 0)
            CV_Error(CV_BadDepth, "");

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);

            int type = CV_MAKETYPE(depth, img->nChannels);

            if (order != IPL_DATA_ORDER_PIXEL)
                CV_Error(CV_StsBadFlag, "Pixel order should be used with coi == 0");

            cvInitMatHeader(mat, img->height, img->width, type,
                            img->imageData, img->widthStep);

        result = mat;
    }
    
    else
        CV_Error(CV_StsBadFlag, "Unrecognized or unsupported array type");

    if (pCOI)
        *pCOI = coi;

    return result;
}



static void
icvGetColorModel(int nchannels, const char **colorModel, const char **channelSeq)
{
    static const char *tab[][2] =
        {
            {"GRAY", "GRAY"},
            {"", ""},
            {"RGB", "BGR"},
            {"RGB", "BGRA"}};

    nchannels--;
    *colorModel = *channelSeq = "";

    if ((unsigned)nchannels <= 3)
    {
        *colorModel = tab[nchannels][0];
        *channelSeq = tab[nchannels][1];
    }
}

// create IplImage header
CV_IMPL IplImage *
cvCreateImageHeader(CvSize size, int depth, int channels)
{
    IplImage *img = 0;

    if (!CvIPL.createHeader)
    {
        img = (IplImage *)cvAlloc(sizeof(*img));
        cvInitImageHeader(img, size, depth, channels, IPL_ORIGIN_TL,
                          CV_DEFAULT_IMAGE_ROW_ALIGN);
    }
    else
    {
        const char *colorModel, *channelSeq;

        icvGetColorModel(channels, &colorModel, &channelSeq);

        img = CvIPL.createHeader(channels, 0, depth, (char *)colorModel, (char *)channelSeq,
                                 IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
                                 CV_DEFAULT_IMAGE_ROW_ALIGN,
                                 size.width, size.height,  0, 0, 0);
    }

    return img;
}

// create IplImage header and allocate underlying data
CV_IMPL IplImage *
cvCreateImage(CvSize size, int depth, int channels)
{
    IplImage *img = cvCreateImageHeader(size, depth, channels);
    assert(img);
    cvCreateData(img);

    return img;
}

// initialize IplImage header, allocated by the user
CV_IMPL IplImage *
cvInitImageHeader(IplImage *image, CvSize size, int depth,
                  int channels, int origin, int align)
{
    const char *colorModel, *channelSeq;

    if (!image)
        CV_Error(CV_HeaderIsNull, "null pointer to header");

    memset(image, 0, sizeof(*image));
    image->nSize = sizeof(*image);

    icvGetColorModel(channels, &colorModel, &channelSeq);
    strncpy(image->colorModel, colorModel, 4);
    strncpy(image->channelSeq, channelSeq, 4);

    if (size.width < 0 || size.height < 0)
        CV_Error(CV_BadROISize, "Bad input roi");

    if ((depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
         depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
         depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
         depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
        channels < 0)
        CV_Error(CV_BadDepth, "Unsupported format");
    if (origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL)
        CV_Error(CV_BadOrigin, "Bad input origin");

    if (align != 4 && align != 8)
        CV_Error(CV_BadAlign, "Bad input align");

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

    return image;
}

CV_IMPL void
cvReleaseImageHeader(IplImage **image)
{
    if (!image)
        CV_Error(CV_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = 0;

        if (!CvIPL.deallocate)
        {
            cvFree(&img);
        }
        else
        {
            CvIPL.deallocate(img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI);
        }
    }
}

CV_IMPL void
cvReleaseImage(IplImage **image)
{
    if (!image)
        CV_Error(CV_StsNullPtr, "");

    if (*image)
    {
        IplImage *img = *image;
        *image = 0;

        cvReleaseData(img);
        cvReleaseImageHeader(&img);
    }
}

CV_IMPL IplImage *
cvCloneImage(const IplImage *src)
{
    IplImage *dst = 0;

    if (!CV_IS_IMAGE_HDR(src))
        CV_Error(CV_StsBadArg, "Bad image header");

    if (!CvIPL.cloneImage)
    {
        dst = (IplImage *)cvAlloc(sizeof(*dst));

        memcpy(dst, src, sizeof(*src));
        dst->imageData = dst->imageDataOrigin = 0;
        if (src->imageData)
        {
            int size = src->imageSize;
            cvCreateData(dst);
            memcpy(dst->imageData, src->imageData, size);
        }
    }
    else
        dst = CvIPL.cloneImage(src);

    return dst;
}

/****************************************************************************************\
*                            Additional operations on CvTermCriteria                     *
\****************************************************************************************/

namespace cv
{

template <>
void Ptr<CvMat>::delete_obj()
{
    cvReleaseMat(&obj);
}

template <>
void Ptr<IplImage>::delete_obj()
{
    cvReleaseImage(&obj);
}

template <>
void Ptr<CvMemStorage>::delete_obj()
{
    cvReleaseMemStorage(&obj);
}
}

/* End of file. */

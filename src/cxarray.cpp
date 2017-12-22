
/* ////////////////////////////////////////////////////////////////////
//
//  CvMat, CvMatND, CvSparceMat and IplImage support functions
//  (creation, deletion, copying, retrieving and setting elements etc.)
//
// */

#include "_cxcore.h"

static struct
{
    Cv_iplCreateImageHeader  createHeader;
    Cv_iplAllocateImageData  allocateData;
    Cv_iplDeallocate  deallocate;
    Cv_iplCloneImage  cloneImage;
}
CvIPL;


/****************************************************************************************\
*                               CvMat creation and basic operations                      *
\****************************************************************************************/

// Creates CvMat and underlying data
CV_IMPL CvMat*
cvCreateMat( int height, int width, int type )
{
    CvMat* arr = 0;

    CV_FUNCNAME( "cvCreateMat" );

    __BEGIN__;

    CV_CALL( arr = cvCreateMatHeader( height, width, type ));
    CV_CALL( cvCreateData( arr ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}


static void icvCheckHuge( CvMat* arr )
{
    if( (int64)arr->step*arr->rows > INT_MAX )
        arr->type &= ~CV_MAT_CONT_FLAG;
}

// Creates CvMat header only
CV_IMPL CvMat*
cvCreateMatHeader( int rows, int cols, int type )
{
    CvMat* arr = 0;

    CV_FUNCNAME( "cvCreateMatHeader" );

    __BEGIN__;

    int min_step;
    type = CV_MAT_TYPE(type);

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive width or height" );

    min_step = CV_ELEM_SIZE(type)*cols;
    if( min_step <= 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "Invalid matrix type" );

    CV_CALL( arr = (CvMat*)cvAlloc( sizeof(*arr)));

    arr->step = rows == 1 ? 0 : cvAlign(min_step, CV_DEFAULT_MAT_ROW_ALIGN);
    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == 0 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    icvCheckHuge( arr );

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}


// Initializes CvMat header, allocated by the user
CV_IMPL CvMat*
cvInitMatHeader( CvMat* arr, int rows, int cols,
                 int type, void* data, int step )
{
    CV_FUNCNAME( "cvInitMatHeader" );

    __BEGIN__;

    int mask, pix_size, min_step;

    if( !arr )
        CV_ERROR_FROM_CODE( CV_StsNullPtr );

    if( (unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX )
        CV_ERROR_FROM_CODE( CV_BadNumChannels );

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive cols or rows" );

    type = CV_MAT_TYPE( type );
    arr->type = type | CV_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar*)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    mask = (arr->rows <= 1) - 1;
    pix_size = CV_ELEM_SIZE(type);
    min_step = arr->cols*pix_size & mask;

    if( step != CV_AUTOSTEP && step != 0 )
    {
        if( step < min_step )
            CV_ERROR_FROM_CODE( CV_BadStep );
        arr->step = step & mask;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

    icvCheckHuge( arr );

    __END__;

    return arr;
}


// Deallocates the CvMat structure and underlying data
CV_IMPL void
cvReleaseMat( CvMat** array )
{
    CV_FUNCNAME( "cvReleaseMat" );

    __BEGIN__;

    if( !array )
        CV_ERROR_FROM_CODE( CV_HeaderIsNull );

    if( *array )
    {
        CvMat* arr = *array;

        if( !CV_IS_MAT_HDR(arr) && !CV_IS_MATND_HDR(arr) )
            CV_ERROR_FROM_CODE( CV_StsBadFlag );

        *array = 0;

        cvDecRefData( arr );
        cvFree( &arr );
    }

    __END__;
}


// Creates a copy of matrix
CV_IMPL CvMat*
cvCloneMat( const CvMat* src )
{
    CvMat* dst = 0;
    CV_FUNCNAME( "cvCloneMat" );

    __BEGIN__;

    if( !CV_IS_MAT_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad CvMat header" );

    CV_CALL( dst = cvCreateMatHeader( src->rows, src->cols, src->type ));

    if( src->data.ptr )
    {
        CV_CALL( cvCreateData( dst ));
        CV_CALL( cvCopy( src, dst ));
    }

    __END__;

    return dst;
}



// returns zero value if iteration is finished, non-zero otherwise
CV_IMPL  int  cvNextNArraySlice( CvNArrayIterator* iterator )
{
    assert( iterator != 0 );
    int i, dims, size = 0;

    for( dims = iterator->dims; dims > 0; dims-- )
    {
        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] += iterator->hdr[i]->dim[dims-1].step;

        if( --iterator->stack[dims-1] > 0 )
            break;

        size = iterator->hdr[0]->dim[dims-1].size;

        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] -= (size_t)size*iterator->hdr[i]->dim[dims-1].step;

        iterator->stack[dims-1] = size;
    }

    return dims > 0;
}



/****************************************************************************************\
*                          Common for multiple array types operations                    *
\****************************************************************************************/

// Allocates underlying array data
CV_IMPL void
cvCreateData( CvArr* arr )
{
    CV_FUNCNAME( "cvCreateData" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        size_t step, total_size;
        CvMat* mat = (CvMat*)arr;
        step = mat->step;

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( step == 0 )
            step = CV_ELEM_SIZE(mat->type)*mat->cols;

        total_size = step*mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
        CV_CALL( mat->refcount = (int*)cvAlloc( (size_t)total_size ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else if( CV_IS_IMAGE_HDR(arr))
    {
        IplImage* img = (IplImage*)arr;

        if( img->imageData != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( !CvIPL.allocateData )
        {
            CV_CALL( img->imageData = img->imageDataOrigin =
                        (char*)cvAlloc( (size_t)img->imageSize ));
        }
        else
        {
            int depth = img->depth;
            int width = img->width;

            if( img->depth == IPL_DEPTH_32F || img->nChannels == 64 )
            {
                img->width *= img->depth == IPL_DEPTH_32F ? sizeof(float) : sizeof(double);
                img->depth = IPL_DEPTH_8U;
            }

            CvIPL.allocateData( img, 0, 0 );

            img->width = width;
            img->depth = depth;
        }
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        size_t total_size = CV_ELEM_SIZE(mat->type);

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( CV_IS_MAT_CONT( mat->type ))
        {
            total_size = (size_t)mat->dim[0].size*(mat->dim[0].step != 0 ?
                         mat->dim[0].step : total_size);
        }
        else
        {
            for( i = mat->dims - 1; i >= 0; i-- )
            {
                size_t size = (size_t)mat->dim[i].step*mat->dim[i].size;

                if( total_size < size )
                    total_size = size;
            }
        }

        CV_CALL( mat->refcount = (int*)cvAlloc( total_size +
                                        sizeof(int) + CV_MALLOC_ALIGN ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}


// Assigns external data to array
CV_IMPL void
cvSetData( CvArr* arr, void* data, int step )
{
    CV_FUNCNAME( "cvSetData" );

    __BEGIN__;

    int pix_size, min_step;

    if( CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr) )
        cvReleaseData( arr );

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;

        int type = CV_MAT_TYPE(mat->type);
        pix_size = CV_ELEM_SIZE(type);
        min_step = mat->cols*pix_size & ((mat->rows <= 1) - 1);

        if( step != CV_AUTOSTEP )
        {
            if( step < min_step && data != 0 )
                CV_ERROR_FROM_CODE( CV_BadStep );
            mat->step = step & ((mat->rows <= 1) - 1);
        }
        else
        {
            mat->step = min_step;
        }

        mat->data.ptr = (uchar*)data;
        mat->type = CV_MAT_MAGIC_VAL | type |
                    (mat->step==min_step ? CV_MAT_CONT_FLAG : 0);
        icvCheckHuge( mat );
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        pix_size = ((img->depth & 255) >> 3)*img->nChannels;
        min_step = img->width*pix_size;

        if( step != CV_AUTOSTEP && img->height > 1 )
        {
            if( step < min_step && data != 0 )
                CV_ERROR_FROM_CODE( CV_BadStep );
            img->widthStep = step;
        }
        else
        {
            img->widthStep = min_step;
        }

        img->imageSize = img->widthStep * img->height;
        img->imageData = img->imageDataOrigin = (char*)data;

        if( (((int)(size_t)data | step) & 7) == 0 &&
            cvAlign(img->width * pix_size, 8) == step )
        {
            img->align = 8;
        }
        else
        {
            img->align = 4;
        }
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        int64 cur_step;

        if( step != CV_AUTOSTEP )
            CV_ERROR( CV_BadStep,
            "For multidimensional array only CV_AUTOSTEP is allowed here" );

        mat->data.ptr = (uchar*)data;
        cur_step = CV_ELEM_SIZE(mat->type);

        for( i = mat->dims - 1; i >= 0; i-- )
        {
            if( cur_step > INT_MAX )
                CV_ERROR( CV_StsOutOfRange, "The array is too big" );
            mat->dim[i].step = (int)cur_step;
            cur_step *= mat->dim[i].size;
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}


// Deallocates array's data
CV_IMPL void
cvReleaseData( CvArr* arr )
{
    CV_FUNCNAME( "cvReleaseData" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ) || CV_IS_MATND_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        cvDecRefData( mat );
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( !CvIPL.deallocate )
        {
            char* ptr = img->imageDataOrigin;
            img->imageData = img->imageDataOrigin = 0;
            cvFree( &ptr );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_DATA );
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}



CV_IMPL int
cvGetElemType( const CvArr* arr )
{
    int type = -1;

    CV_FUNCNAME( "cvGetElemType" );

    __BEGIN__;

    if( CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr) || CV_IS_SPARSE_MAT_HDR(arr))
    {
        type = CV_MAT_TYPE( ((CvMat*)arr)->type );
    }
    else if( CV_IS_IMAGE(arr))
    {
        IplImage* img = (IplImage*)arr;
        type = CV_MAKETYPE( icvIplToCvDepth(img->depth), img->nChannels );
    }
    else
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );

    __END__;

    return type;
}


// Returns the size of CvMat or IplImage
CV_IMPL CvSize
cvGetSize( const CvArr* arr )
{
    CvSize size = { 0, 0 };

    CV_FUNCNAME( "cvGetSize" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat *mat = (CvMat*)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

            size.width = img->width;
            size.height = img->height;
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "Array should be CvMat or IplImage" );
    }

    __END__;

    return size;
}


/****************************************************************************************\
*                      Operations on CvScalar and accessing array elements               *
\****************************************************************************************/

// Converts CvScalar to specified type
CV_IMPL void
cvScalarToRawData( const CvScalar* scalar, void* data, int type, int extend_to_12 )
{
    CV_FUNCNAME( "cvScalarToRawData" );

    type = CV_MAT_TYPE(type);

    __BEGIN__;

    int cn = CV_MAT_CN( type );
    int depth = type & CV_MAT_DEPTH_MASK;

    assert( scalar && data );
    if( (unsigned)(cn - 1) >= 4 )
        CV_ERROR( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    switch( depth )
    {
    case CV_8UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((uchar*)data)[cn] = CV_CAST_8U(t);
        }
        break;
    case CV_8SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((char*)data)[cn] = CV_CAST_8S(t);
        }
        break;
    case CV_16UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((ushort*)data)[cn] = CV_CAST_16U(t);
        }
        break;
    case CV_16SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((short*)data)[cn] = CV_CAST_16S(t);
        }
        break;
    case CV_32SC1:
        while( cn-- )
            ((int*)data)[cn] = cvRound( scalar->val[cn] );
        break;
    case CV_32FC1:
        while( cn-- )
            ((float*)data)[cn] = (float)(scalar->val[cn]);
        break;
    case CV_64FC1:
        while( cn-- )
            ((double*)data)[cn] = (double)(scalar->val[cn]);
        break;
    default:
        assert(0);
        CV_ERROR_FROM_CODE( CV_BadDepth );
    }

    if( extend_to_12 )
    {
        int pix_size = CV_ELEM_SIZE(type);
        int offset = CV_ELEM_SIZE1(depth)*12;

        do
        {
            offset -= pix_size;
            CV_MEMCPY_AUTO( (char*)data + offset, data, pix_size );
        }
        while( offset > pix_size );
    }

    __END__;
}



/****************************************************************************************\
*                             Conversion to CvMat or IplImage                            *
\****************************************************************************************/

// convert array (CvMat or IplImage) to CvMat
CV_IMPL CvMat*
cvGetMat( const CvArr* array, CvMat* mat,
          int* pCOI, int allowND )
{
    CvMat* result = 0;
    CvMat* src = (CvMat*)array;
    int coi = 0;

    CV_FUNCNAME( "cvGetMat" );

    __BEGIN__;

    if( !mat || !src )
        CV_ERROR( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MAT_HDR(src))
    {
        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "The matrix has NULL data pointer" );

        result = (CvMat*)src;
    }
    else if( CV_IS_IMAGE_HDR(src) )
    {
        const IplImage* img = (const IplImage*)src;
        int depth, order;

        if( img->imageData == 0 )
            CV_ERROR( CV_StsNullPtr, "The image has NULL data pointer" );

        depth = icvIplToCvDepth( img->depth );
        if( depth < 0 )
            CV_ERROR_FROM_CODE( CV_BadDepth );

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);
        {
            int type = CV_MAKETYPE( depth, img->nChannels );

            if( order != IPL_DATA_ORDER_PIXEL )
                CV_ERROR( CV_StsBadFlag, "Pixel order should be used with coi == 0" );

            CV_CALL( cvInitMatHeader( mat, img->height, img->width, type,
                                      img->imageData, img->widthStep ));
        }

        result = mat;
    }
    else if( allowND && CV_IS_MATND_HDR(src) )
    {
        CvMatND* matnd = (CvMatND*)src;
        int i;
        int size1 = matnd->dim[0].size, size2 = 1;

        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "Input array has NULL data pointer" );

        if( !CV_IS_MAT_CONT( matnd->type ))
            CV_ERROR( CV_StsBadArg, "Only continuous nD arrays are supported here" );

        if( matnd->dims > 2 )
            for( i = 1; i < matnd->dims; i++ )
                size2 *= matnd->dim[i].size;
        else
            size2 = matnd->dims == 1 ? 1 : matnd->dim[1].size;

        mat->refcount = 0;
        mat->hdr_refcount = 0;
        mat->data.ptr = matnd->data.ptr;
        mat->rows = size1;
        mat->cols = size2;
        mat->type = CV_MAT_TYPE(matnd->type) | CV_MAT_MAGIC_VAL | CV_MAT_CONT_FLAG;
        mat->step = size2*CV_ELEM_SIZE(matnd->type);
        mat->step &= size1 > 1 ? -1 : 0;

        icvCheckHuge( mat );
        result = mat;
    }
    else
    {
        CV_ERROR( CV_StsBadFlag, "Unrecognized or unsupported array type" );
    }

    __END__;

    if( pCOI )
        *pCOI = coi;

    return result;
}


CV_IMPL CvMat*
cvReshape( const CvArr* array, CvMat* header,
           int new_cn, int new_rows )
{
    CvMat* result = 0;
    CV_FUNCNAME( "cvReshape" );

    __BEGIN__;

    CvMat *mat = (CvMat*)array;
    int total_width, new_width;

    if( !header )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !CV_IS_MAT( mat ))
    {
        int coi = 0;
        CV_CALL( mat = cvGetMat( mat, header, &coi, 1 ));
        if( coi )
            CV_ERROR( CV_BadCOI, "COI is not supported" );
    }

    if( new_cn == 0 )
        new_cn = CV_MAT_CN(mat->type);
    else if( (unsigned)(new_cn - 1) > 3 )
        CV_ERROR( CV_BadNumChannels, "" );

    if( mat != header )
    {
        *header = *mat;
        header->refcount = 0;
        header->hdr_refcount = 0;
    }

    total_width = mat->cols * CV_MAT_CN( mat->type );

    if( (new_cn > total_width || total_width % new_cn != 0) && new_rows == 0 )
        new_rows = mat->rows * total_width / new_cn;

    if( new_rows == 0 || new_rows == mat->rows )
    {
        header->rows = mat->rows;
        header->step = mat->step;
    }
    else
    {
        int total_size = total_width * mat->rows;
        if( !CV_IS_MAT_CONT( mat->type ))
            CV_ERROR( CV_BadStep,
            "The matrix is not continuous, thus its number of rows can not be changed" );

        if( (unsigned)new_rows > (unsigned)total_size )
            CV_ERROR( CV_StsOutOfRange, "Bad new number of rows" );

        total_width = total_size / new_rows;

        if( total_width * new_rows != total_size )
            CV_ERROR( CV_StsBadArg, "The total number of matrix elements "
                                    "is not divisible by the new number of rows" );

        header->rows = new_rows;
        header->step = total_width * CV_ELEM_SIZE1(mat->type);
    }

    new_width = total_width / new_cn;

    if( new_width * new_cn != total_width )
        CV_ERROR( CV_BadNumChannels,
        "The total width is not divisible by the new number of channels" );

    header->cols = new_width;
    header->type = CV_MAKETYPE( mat->type & ~CV_MAT_CN_MASK, new_cn );

    result = header;

    __END__;

    return  result;
}


// convert array (CvMat or IplImage) to IplImage
CV_IMPL IplImage*
cvGetImage( const CvArr* array, IplImage* img )
{
    IplImage* result = 0;
    const IplImage* src = (const IplImage*)array;

    CV_FUNCNAME( "cvGetImage" );

    __BEGIN__;

    int depth;

    if( !img )
        CV_ERROR_FROM_CODE( CV_StsNullPtr );

    if( !CV_IS_IMAGE_HDR(src) )
    {
        const CvMat* mat = (const CvMat*)src;

        if( !CV_IS_MAT_HDR(mat))
            CV_ERROR_FROM_CODE( CV_StsBadFlag );

        if( mat->data.ptr == 0 )
            CV_ERROR_FROM_CODE( CV_StsNullPtr );

        depth = cvCvToIplDepth(mat->type);

        cvInitImageHeader( img, cvSize(mat->cols, mat->rows),
                           depth, CV_MAT_CN(mat->type) );
        cvSetData( img, mat->data.ptr, mat->step );

        result = img;
    }
    else
    {
        result = (IplImage*)src;
    }

    __END__;

    return result;
}


/****************************************************************************************\
*                               IplImage-specific functions                              *
\****************************************************************************************/
static  void
icvGetColorModel( int nchannels, char** colorModel, char** channelSeq )
{
    static char* tab[][2] =
    {
        {"GRAY", "GRAY"},
        {"",""},
        {"RGB","BGR"},
        {"RGB","BGRA"}
    };

    nchannels--;
    *colorModel = *channelSeq = "";

    if( (unsigned)nchannels <= 3 )
    {
        *colorModel = tab[nchannels][0];
        *channelSeq = tab[nchannels][1];
    }
}


// create IplImage header
CV_IMPL IplImage *
cvCreateImageHeader( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImageHeader" );

    __BEGIN__;

    if( !CvIPL.createHeader )
    {
        CV_CALL( img = (IplImage *)cvAlloc( sizeof( *img )));
        CV_CALL( cvInitImageHeader( img, size, depth, channels, IPL_ORIGIN_TL,
                                    CV_DEFAULT_IMAGE_ROW_ALIGN ));
    }
    else
    {
        char *colorModel;
        char *channelSeq;

        icvGetColorModel( channels, &colorModel, &channelSeq );

        img = CvIPL.createHeader( channels, 0, depth, colorModel, channelSeq,
                                  IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
                                  CV_DEFAULT_IMAGE_ROW_ALIGN,
                                  size.width, size.height, 0, 0, 0 );
    }

    __END__;

    if( cvGetErrStatus() < 0 && img )
        cvReleaseImageHeader( &img );

    return img;
}


// create IplImage header and allocate underlying data
CV_IMPL IplImage *
cvCreateImage( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImage" );

    __BEGIN__;

    CV_CALL( img = cvCreateImageHeader( size, depth, channels ));
    assert( img );
    CV_CALL( cvCreateData( img ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseImage( &img );

    return img;
}


// initalize IplImage header, allocated by the user
CV_IMPL IplImage*
cvInitImageHeader( IplImage * image, CvSize size, int depth,
                   int channels, int origin, int align )
{
    IplImage* result = 0;

    CV_FUNCNAME( "cvInitImageHeader" );

    __BEGIN__;

    char *colorModel, *channelSeq;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "null pointer to header" );

    memset( image, 0, sizeof( *image ));
    image->nSize = sizeof( *image );

    CV_CALL( icvGetColorModel( channels, &colorModel, &channelSeq ));
    strncpy( image->colorModel, colorModel, 4 );
    strncpy( image->channelSeq, channelSeq, 4 );

    if( size.width < 0 || size.height < 0 )
        CV_ERROR( CV_BadROISize, "Bad input roi" );

    if( (depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
         depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
         depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
         depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
         channels < 0 )
        CV_ERROR( CV_BadDepth, "Unsupported format" );
    if( origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL )
        CV_ERROR( CV_BadOrigin, "Bad input origin" );

    if( align != 4 && align != 8 )
        CV_ERROR( CV_BadAlign, "Bad input align" );

    image->width = size.width;
    image->height = size.height;


    image->nChannels = MAX( channels, 1 );
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
         (image->depth & ~IPL_DEPTH_SIGN) + 7)/8)+ align - 1) & (~(align - 1));
    image->origin = origin;
    image->imageSize = image->widthStep * image->height;

    result = image;

    __END__;

    return result;
}


CV_IMPL void
cvReleaseImageHeader( IplImage** image )
{
    CV_FUNCNAME( "cvReleaseImageHeader" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;

        if( !CvIPL.deallocate )
        {
            cvFree( &img );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI );
        }
    }
    __END__;
}


CV_IMPL void
cvReleaseImage( IplImage ** image )
{
    CV_FUNCNAME( "cvReleaseImage" );

    __BEGIN__

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;

        cvReleaseData( img );
        cvReleaseImageHeader( &img );
    }

    __END__;
}


CV_IMPL IplImage*
cvCloneImage( const IplImage* src )
{
    IplImage* dst = 0;
    CV_FUNCNAME( "cvCloneImage" );

    __BEGIN__;

    if( !CV_IS_IMAGE_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad image header" );

    if( !CvIPL.cloneImage )
    {
        CV_CALL( dst = (IplImage*)cvAlloc( sizeof(*dst)));

        memcpy( dst, src, sizeof(*src));
        dst->imageData = dst->imageDataOrigin = 0;



        if( src->imageData )
        {
            int size = src->imageSize;
            cvCreateData( dst );
            memcpy( dst->imageData, src->imageData, size );
        }
    }
    else
    {
        dst = CvIPL.cloneImage( src );
    }

    __END__;

    return dst;
}

/* End of file. */

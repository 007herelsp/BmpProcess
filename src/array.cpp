#include "core.precomp.hpp"


static struct
{
    Cv_iplCreateImageHeader  createHeader;
    Cv_iplAllocateImageData  allocateData;
    Cv_iplDeallocate  deallocate;
    Cv_iplCreateROI  createROI;
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
    CvMat* arr = cvCreateMatHeader( height, width, type );
    cvCreateData( arr );

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
    type = CV_MAT_TYPE(type);

    if( rows < 0 || cols <= 0 )
        CV_Error( CV_StsBadSize, "Non-positive width or height" );

    int min_step = CV_ELEM_SIZE(type)*cols;
    if( min_step <= 0 )
        CV_Error( CV_StsUnsupportedFormat, "Invalid matrix type" );

    CvMat* arr = (CvMat*)cvAlloc( sizeof(*arr));

    arr->step = min_step;
    arr->type = CV_MAT_MAGIC_VAL | type | CV_MAT_CONT_FLAG;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    icvCheckHuge( arr );
    return arr;
}


// Initializes CvMat header, allocated by the user
CV_IMPL CvMat*
cvInitMatHeader( CvMat* arr, int rows, int cols,
                 int type, void* data, int step )
{
    if( !arr )
        CV_Error( CV_StsNullPtr, "" );

    if( (unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX )
        CV_Error( CV_BadNumChannels, "" );

    if( rows < 0 || cols <= 0 )
        CV_Error( CV_StsBadSize, "Non-positive cols or rows" );

    type = CV_MAT_TYPE( type );
    arr->type = type | CV_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar*)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    int pix_size = CV_ELEM_SIZE(type);
    int min_step = arr->cols*pix_size;

    if( step != CV_AUTOSTEP && step != 0 )
    {
        if( step < min_step )
            CV_Error( CV_BadStep, "" );
        arr->step = step;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = CV_MAT_MAGIC_VAL | type |
        (arr->rows == 1 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

    icvCheckHuge( arr );
    return arr;
}


// Deallocates the CvMat structure and underlying data
CV_IMPL void
cvReleaseMat( CvMat** array )
{
    if( !array )
        CV_Error( CV_HeaderIsNull, "" );

    if( *array )
    {
        CvMat* arr = *array;

        if( !CV_IS_MAT_HDR_Z(arr) && !CV_IS_MATND_HDR(arr) )
            CV_Error( CV_StsBadFlag, "" );

        *array = 0;

        cvDecRefData( arr );
        cvFree( &arr );
    }
}


// Creates a copy of matrix
CV_IMPL CvMat*
cvCloneMat( const CvMat* src )
{
    if( !CV_IS_MAT_HDR( src ))
        CV_Error( CV_StsBadArg, "Bad CvMat header" );

    CvMat* dst = cvCreateMatHeader( src->rows, src->cols, src->type );

    if( src->data.ptr )
    {
        cvCreateData( dst );
        cvCopy( src, dst );
    }

    return dst;
}


/****************************************************************************************\
*                               CvMatND creation and basic operations                    *
\****************************************************************************************/

CV_IMPL CvMatND*
cvInitMatNDHeader( CvMatND* mat, int dims, const int* sizes,
                   int type, void* data )
{
    type = CV_MAT_TYPE(type);
    int64 step = CV_ELEM_SIZE(type);

    if( !mat )
        CV_Error( CV_StsNullPtr, "NULL matrix header pointer" );

    if( step == 0 )
        CV_Error( CV_StsUnsupportedFormat, "invalid array data type" );

    if( !sizes )
        CV_Error( CV_StsNullPtr, "NULL <sizes> pointer" );

    if( dims <= 0 || dims > CV_MAX_DIM )
        CV_Error( CV_StsOutOfRange,
        "non-positive or too large number of dimensions" );

    for( int i = dims - 1; i >= 0; i-- )
    {
        if( sizes[i] < 0 )
            CV_Error( CV_StsBadSize, "one of dimesion sizes is non-positive" );
        mat->dim[i].size = sizes[i];
        if( step > INT_MAX )
            CV_Error( CV_StsOutOfRange, "The array is too big" );
        mat->dim[i].step = (int)step;
        step *= sizes[i];
    }

    mat->type = CV_MATND_MAGIC_VAL | (step <= INT_MAX ? CV_MAT_CONT_FLAG : 0) | type;
    mat->dims = dims;
    mat->data.ptr = (uchar*)data;
    mat->refcount = 0;
    mat->hdr_refcount = 0;
    return mat;
}


// Creates CvMatND and underlying data
CV_IMPL CvMatND*
cvCreateMatND( int dims, const int* sizes, int type )
{
    CvMatND* arr = cvCreateMatNDHeader( dims, sizes, type );
    cvCreateData( arr );

    return arr;
}


// Creates CvMatND header only
CV_IMPL CvMatND*
cvCreateMatNDHeader( int dims, const int* sizes, int type )
{
    if( dims <= 0 || dims > CV_MAX_DIM )
        CV_Error( CV_StsOutOfRange,
        "non-positive or too large number of dimensions" );

    CvMatND* arr = (CvMatND*)cvAlloc( sizeof(*arr) );

    cvInitMatNDHeader( arr, dims, sizes, type, 0 );
    arr->hdr_refcount = 1;
    return arr;
}


// Creates a copy of nD array
CV_IMPL CvMatND*
cvCloneMatND( const CvMatND* src )
{
    if( !CV_IS_MATND_HDR( src ))
        CV_Error( CV_StsBadArg, "Bad CvMatND header" );

    CV_Assert( src->dims <= CV_MAX_DIM );
    int sizes[CV_MAX_DIM];

    for( int i = 0; i < src->dims; i++ )
        sizes[i] = src->dim[i].size;

    CvMatND* dst = cvCreateMatNDHeader( src->dims, sizes, src->type );

    if( src->data.ptr )
    {
        cvCreateData( dst );
        cv::Mat _src(src), _dst(dst);
        uchar* data0 = dst->data.ptr;
        _src.copyTo(_dst);
        CV_Assert(_dst.data == data0);
        //cvCopy( src, dst );
    }

    return dst;
}



/****************************************************************************************\
*                          Common for multiple array types operations                    *
\****************************************************************************************/

// Allocates underlying array data
CV_IMPL void
cvCreateData( CvArr* arr )
{
    if( CV_IS_MAT_HDR_Z( arr ))
    {
        size_t step, total_size;
        CvMat* mat = (CvMat*)arr;
        step = mat->step;

        if( mat->rows == 0 || mat->cols == 0 )
            return;

        if( mat->data.ptr != 0 )
            CV_Error( CV_StsError, "Data is already allocated" );

        if( step == 0 )
            step = CV_ELEM_SIZE(mat->type)*mat->cols;

        int64 _total_size = (int64)step*mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
        total_size = (size_t)_total_size;
        if(_total_size != (int64)total_size)
            CV_Error(CV_StsNoMem, "Too big buffer is allocated" );
        mat->refcount = (int*)cvAlloc( (size_t)total_size );
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else if( CV_IS_IMAGE_HDR(arr))
    {
        IplImage* img = (IplImage*)arr;

        if( img->imageData != 0 )
            CV_Error( CV_StsError, "Data is already allocated" );

        if( !CvIPL.allocateData )
        {
            img->imageData = img->imageDataOrigin =
                        (char*)cvAlloc( (size_t)img->imageSize );
        }
        else
        {
            int depth = img->depth;
            int width = img->width;

            if( img->depth == IPL_DEPTH_32F || img->depth == IPL_DEPTH_64F )
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

        if( mat->dim[0].size == 0 )
            return;

        if( mat->data.ptr != 0 )
            CV_Error( CV_StsError, "Data is already allocated" );

        if( CV_IS_MAT_CONT( mat->type ))
        {
            total_size = (size_t)mat->dim[0].size*(mat->dim[0].step != 0 ?
                         (size_t)mat->dim[0].step : total_size);
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

        mat->refcount = (int*)cvAlloc( total_size +
                                        sizeof(int) + CV_MALLOC_ALIGN );
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else
        CV_Error( CV_StsBadArg, "unrecognized or unsupported array type" );
}


// Assigns external data to array
CV_IMPL void
cvSetData( CvArr* arr, void* data, int step )
{
    int pix_size, min_step;

    if( CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr) )
        cvReleaseData( arr );

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;

        int type = CV_MAT_TYPE(mat->type);
        pix_size = CV_ELEM_SIZE(type);
        min_step = mat->cols*pix_size;

        if( step != CV_AUTOSTEP && step != 0 )
        {
            if( step < min_step && data != 0 )
                CV_Error( CV_BadStep, "" );
            mat->step = step;
        }
        else
            mat->step = min_step;

        mat->data.ptr = (uchar*)data;
        mat->type = CV_MAT_MAGIC_VAL | type |
                    (mat->rows == 1 || mat->step == min_step ? CV_MAT_CONT_FLAG : 0);
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
                CV_Error( CV_BadStep, "" );
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
            img->align = 8;
        else
            img->align = 4;
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        int64 cur_step;

        if( step != CV_AUTOSTEP )
            CV_Error( CV_BadStep,
            "For multidimensional array only CV_AUTOSTEP is allowed here" );

        mat->data.ptr = (uchar*)data;
        cur_step = CV_ELEM_SIZE(mat->type);

        for( i = mat->dims - 1; i >= 0; i-- )
        {
            if( cur_step > INT_MAX )
                CV_Error( CV_StsOutOfRange, "The array is too big" );
            mat->dim[i].step = (int)cur_step;
            cur_step *= mat->dim[i].size;
        }
    }
    else
        CV_Error( CV_StsBadArg, "unrecognized or unsupported array type" );
}


// Deallocates array's data
CV_IMPL void
cvReleaseData( CvArr* arr )
{
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
        CV_Error( CV_StsBadArg, "unrecognized or unsupported array type" );
}





// Returns the size of CvMat or IplImage
CV_IMPL CvSize
cvGetSize( const CvArr* arr )
{
    CvSize size = { 0, 0 };

    if( CV_IS_MAT_HDR_Z( arr ))
    {
        CvMat *mat = (CvMat*)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( img->roi )
        {
            size.width = img->roi->width;
            size.height = img->roi->height;
        }
        else
        {
            size.width = img->width;
            size.height = img->height;
        }
    }
    else
        CV_Error( CV_StsBadArg, "Array should be CvMat or IplImage" );

    return size;
}


// Selects sub-array (no data is copied)
CV_IMPL  CvMat*
cvGetSubRect( const CvArr* arr, CvMat* submat, CvRect rect )
{
    CvMat* res = 0;
    CvMat stub, *mat = (CvMat*)arr;

    if( !CV_IS_MAT( mat ))
        mat = cvGetMat( mat, &stub );

    if( !submat )
        CV_Error( CV_StsNullPtr, "" );

    if( (rect.x|rect.y|rect.width|rect.height) < 0 )
        CV_Error( CV_StsBadSize, "" );

    if( rect.x + rect.width > mat->cols ||
        rect.y + rect.height > mat->rows )
        CV_Error( CV_StsBadSize, "" );

    {
    submat->data.ptr = mat->data.ptr + (size_t)rect.y*mat->step +
                       rect.x*CV_ELEM_SIZE(mat->type);
    submat->step = mat->step;
    submat->type = (mat->type & (rect.width < mat->cols ? ~CV_MAT_CONT_FLAG : -1)) |
                   (rect.height <= 1 ? CV_MAT_CONT_FLAG : 0);
    submat->rows = rect.height;
    submat->cols = rect.width;
    submat->refcount = 0;
    res = submat;
    }

    return res;
}



// Selects array diagonal
CV_IMPL  CvMat*
cvGetDiag( const CvArr* arr, CvMat* submat, int diag )
{
    CvMat* res = 0;
    CvMat stub, *mat = (CvMat*)arr;
    int len, pix_size;

    if( !CV_IS_MAT( mat ))
        mat = cvGetMat( mat, &stub );

    if( !submat )
        CV_Error( CV_StsNullPtr, "" );

    pix_size = CV_ELEM_SIZE(mat->type);

    if( diag >= 0 )
    {
        len = mat->cols - diag;

        if( len <= 0 )
            CV_Error( CV_StsOutOfRange, "" );

        len = CV_IMIN( len, mat->rows );
        submat->data.ptr = mat->data.ptr + diag*pix_size;
    }
    else
    {
        len = mat->rows + diag;

        if( len <= 0 )
            CV_Error( CV_StsOutOfRange, "" );

        len = CV_IMIN( len, mat->cols );
        submat->data.ptr = mat->data.ptr - diag*mat->step;
    }

    submat->rows = len;
    submat->cols = 1;
    submat->step = mat->step + (submat->rows > 1 ? pix_size : 0);
    submat->type = mat->type;
    if( submat->rows > 1 )
        submat->type &= ~CV_MAT_CONT_FLAG;
    else
        submat->type |= CV_MAT_CONT_FLAG;
    submat->refcount = 0;
    submat->hdr_refcount = 0;
    res = submat;

    return res;
}


/****************************************************************************************\
*                      Operations on CvScalar and accessing array elements               *
\****************************************************************************************/

// Converts CvScalar to specified type
CV_IMPL void
cvScalarToRawData( const CvScalar* scalar, void* data, int type, int extend_to_12 )
{
    type = CV_MAT_TYPE(type);
    int cn = CV_MAT_CN( type );
    int depth = type & CV_MAT_DEPTH_MASK;

    assert( scalar && data );
    if( (unsigned)(cn - 1) >= 4 )
        CV_Error( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

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
        CV_Error( CV_BadDepth, "" );
    }

    if( extend_to_12 )
    {
        int pix_size = CV_ELEM_SIZE(type);
        int offset = CV_ELEM_SIZE1(depth)*12;

        do
        {
            offset -= pix_size;
            memcpy((char*)data + offset, data, pix_size);
        }
        while( offset > pix_size );
    }
}


// Converts data of specified type to CvScalar
CV_IMPL void
cvRawDataToScalar( const void* data, int flags, CvScalar* scalar )
{
    int cn = CV_MAT_CN( flags );

    assert( scalar && data );

    if( (unsigned)(cn - 1) >= 4 )
        CV_Error( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    memset( scalar->val, 0, sizeof(scalar->val));

    switch( CV_MAT_DEPTH( flags ))
    {
    case CV_8U:
        while( cn-- )
            scalar->val[cn] = CV_8TO32F(((uchar*)data)[cn]);
        break;
    case CV_8S:
        while( cn-- )
            scalar->val[cn] = CV_8TO32F(((char*)data)[cn]);
        break;
    case CV_16U:
        while( cn-- )
            scalar->val[cn] = ((ushort*)data)[cn];
        break;
    case CV_16S:
        while( cn-- )
            scalar->val[cn] = ((short*)data)[cn];
        break;
    case CV_32S:
        while( cn-- )
            scalar->val[cn] = ((int*)data)[cn];
        break;
    case CV_32F:
        while( cn-- )
            scalar->val[cn] = ((float*)data)[cn];
        break;
    case CV_64F:
        while( cn-- )
            scalar->val[cn] = ((double*)data)[cn];
        break;
    default:
        assert(0);
        CV_Error( CV_BadDepth, "" );
    }
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

    if( !mat || !src )
        CV_Error( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MAT_HDR(src))
    {
        if( !src->data.ptr )
            CV_Error( CV_StsNullPtr, "The matrix has NULL data pointer" );

        result = (CvMat*)src;
    }
    else if( CV_IS_IMAGE_HDR(src) )
    {
        const IplImage* img = (const IplImage*)src;
        int depth, order;

        if( img->imageData == 0 )
            CV_Error( CV_StsNullPtr, "The image has NULL data pointer" );

        depth = IPL2CV_DEPTH( img->depth );
        if( depth < 0 )
            CV_Error( CV_BadDepth, "" );

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);

        if( img->roi )
        {
            if( order == IPL_DATA_ORDER_PLANE )
            {
                int type = depth;

                if( img->roi->coi == 0 )
                    CV_Error( CV_StsBadFlag,
                    "Images with planar data layout should be used with COI selected" );

                cvInitMatHeader( mat, img->roi->height,
                                img->roi->width, type,
                                img->imageData + (img->roi->coi-1)*img->imageSize +
                                img->roi->yOffset*img->widthStep +
                                img->roi->xOffset*CV_ELEM_SIZE(type),
                                img->widthStep );
            }
            else /* pixel order */
            {
                int type = CV_MAKETYPE( depth, img->nChannels );
                coi = img->roi->coi;

                if( img->nChannels > CV_CN_MAX )
                    CV_Error( CV_BadNumChannels,
                        "The image is interleaved and has over CV_CN_MAX channels" );

                cvInitMatHeader( mat, img->roi->height, img->roi->width,
                                 type, img->imageData +
                                 img->roi->yOffset*img->widthStep +
                                 img->roi->xOffset*CV_ELEM_SIZE(type),
                                 img->widthStep );
            }
        }
        else
        {
            int type = CV_MAKETYPE( depth, img->nChannels );

            if( order != IPL_DATA_ORDER_PIXEL )
                CV_Error( CV_StsBadFlag, "Pixel order should be used with coi == 0" );

            cvInitMatHeader( mat, img->height, img->width, type,
                             img->imageData, img->widthStep );
        }

        result = mat;
    }
    else if( allowND && CV_IS_MATND_HDR(src) )
    {
        CvMatND* matnd = (CvMatND*)src;
        int i;
        int size1 = matnd->dim[0].size, size2 = 1;

        if( !src->data.ptr )
            CV_Error( CV_StsNullPtr, "Input array has NULL data pointer" );

        if( !CV_IS_MAT_CONT( matnd->type ))
            CV_Error( CV_StsBadArg, "Only continuous nD arrays are supported here" );

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
        CV_Error( CV_StsBadFlag, "Unrecognized or unsupported array type" );

    if( pCOI )
        *pCOI = coi;

    return result;
}






// convert array (CvMat or IplImage) to IplImage
CV_IMPL IplImage*
cvGetImage( const CvArr* array, IplImage* img )
{
    IplImage* result = 0;
    const IplImage* src = (const IplImage*)array;
    int depth;

    if( !img )
        CV_Error( CV_StsNullPtr, "" );

    if( !CV_IS_IMAGE_HDR(src) )
    {
        const CvMat* mat = (const CvMat*)src;

        if( !CV_IS_MAT_HDR(mat))
            CV_Error( CV_StsBadFlag, "" );

        if( mat->data.ptr == 0 )
            CV_Error( CV_StsNullPtr, "" );

        depth = cvIplDepth(mat->type);

        cvInitImageHeader( img, cvSize(mat->cols, mat->rows),
                           depth, CV_MAT_CN(mat->type) );
        cvSetData( img, mat->data.ptr, mat->step );

        result = img;
    }
    else
    {
        result = (IplImage*)src;
    }

    return result;
}


/****************************************************************************************\
*                               IplImage-specific functions                              *
\****************************************************************************************/

static IplROI* icvCreateROI( int coi, int xOffset, int yOffset, int width, int height )
{
    IplROI *roi = 0;
    if( !CvIPL.createROI )
    {
        roi = (IplROI*)cvAlloc( sizeof(*roi));

        roi->coi = coi;
        roi->xOffset = xOffset;
        roi->yOffset = yOffset;
        roi->width = width;
        roi->height = height;
    }
    else
    {
        roi = CvIPL.createROI( coi, xOffset, yOffset, width, height );
    }

    return roi;
}

static  void
icvGetColorModel( int nchannels, const char** colorModel, const char** channelSeq )
{
    static const char* tab[][2] =
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

    if( !CvIPL.createHeader )
    {
        img = (IplImage *)cvAlloc( sizeof( *img ));
        cvInitImageHeader( img, size, depth, channels, IPL_ORIGIN_TL,
                                    CV_DEFAULT_IMAGE_ROW_ALIGN );
    }
    else
    {
        const char *colorModel, *channelSeq;

        icvGetColorModel( channels, &colorModel, &channelSeq );

        img = CvIPL.createHeader( channels, 0, depth, (char*)colorModel, (char*)channelSeq,
                                  IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
                                  CV_DEFAULT_IMAGE_ROW_ALIGN,
                                  size.width, size.height, 0, 0, 0, 0 );
    }

    return img;
}


// create IplImage header and allocate underlying data
CV_IMPL IplImage *
cvCreateImage( CvSize size, int depth, int channels )
{
    IplImage *img = cvCreateImageHeader( size, depth, channels );
    assert( img );
    cvCreateData( img );

    return img;
}


// initialize IplImage header, allocated by the user
CV_IMPL IplImage*
cvInitImageHeader( IplImage * image, CvSize size, int depth,
                   int channels, int origin, int align )
{
    const char *colorModel, *channelSeq;

    if( !image )
        CV_Error( CV_HeaderIsNull, "null pointer to header" );

    memset( image, 0, sizeof( *image ));
    image->nSize = sizeof( *image );

    icvGetColorModel( channels, &colorModel, &channelSeq );
    strncpy( image->colorModel, colorModel, 4 );
    strncpy( image->channelSeq, channelSeq, 4 );

    if( size.width < 0 || size.height < 0 )
        CV_Error( CV_BadROISize, "Bad input roi" );

    if( (depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
         depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
         depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
         depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
         channels < 0 )
        CV_Error( CV_BadDepth, "Unsupported format" );
    if( origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL )
        CV_Error( CV_BadOrigin, "Bad input origin" );

    if( align != 4 && align != 8 )
        CV_Error( CV_BadAlign, "Bad input align" );

    image->width = size.width;
    image->height = size.height;

    if( image->roi )
    {
        image->roi->coi = 0;
        image->roi->xOffset = image->roi->yOffset = 0;
        image->roi->width = size.width;
        image->roi->height = size.height;
    }

    image->nChannels = MAX( channels, 1 );
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
         (image->depth & ~IPL_DEPTH_SIGN) + 7)/8)+ align - 1) & (~(align - 1));
    image->origin = origin;
    image->imageSize = image->widthStep * image->height;

    return image;
}


CV_IMPL void
cvReleaseImageHeader( IplImage** image )
{
    if( !image )
        CV_Error( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;

        if( !CvIPL.deallocate )
        {
            cvFree( &img->roi );
            cvFree( &img );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI );
        }
    }
}


CV_IMPL void
cvReleaseImage( IplImage ** image )
{
    if( !image )
        CV_Error( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;

        cvReleaseData( img );
        cvReleaseImageHeader( &img );
    }
}


CV_IMPL void
cvSetImageROI( IplImage* image, CvRect rect )
{
    if( !image )
        CV_Error( CV_HeaderIsNull, "" );

    // allow zero ROI width or height
    CV_Assert( rect.width >= 0 && rect.height >= 0 &&
               rect.x < image->width && rect.y < image->height &&
               rect.x + rect.width >= (int)(rect.width > 0) &&
               rect.y + rect.height >= (int)(rect.height > 0) );

    rect.width += rect.x;
    rect.height += rect.y;

    rect.x = std::max(rect.x, 0);
    rect.y = std::max(rect.y, 0);
    rect.width = std::min(rect.width, image->width);
    rect.height = std::min(rect.height, image->height);

    rect.width -= rect.x;
    rect.height -= rect.y;

    if( image->roi )
    {
        image->roi->xOffset = rect.x;
        image->roi->yOffset = rect.y;
        image->roi->width = rect.width;
        image->roi->height = rect.height;
    }
    else
        image->roi = icvCreateROI( 0, rect.x, rect.y, rect.width, rect.height );
}


CV_IMPL void
cvResetImageROI( IplImage* image )
{
    if( !image )
        CV_Error( CV_HeaderIsNull, "" );

    if( image->roi )
    {
        if( !CvIPL.deallocate )
        {
            cvFree( &image->roi );
        }
        else
        {
            CvIPL.deallocate( image, IPL_IMAGE_ROI );
            image->roi = 0;
        }
    }
}


CV_IMPL CvRect
cvGetImageROI( const IplImage* img )
{
    CvRect rect = { 0, 0, 0, 0 };
    if( !img )
        CV_Error( CV_StsNullPtr, "Null pointer to image" );

    if( img->roi )
        rect = cvRect( img->roi->xOffset, img->roi->yOffset,
                       img->roi->width, img->roi->height );
    else
        rect = cvRect( 0, 0, img->width, img->height );

    return rect;
}


CV_IMPL void
cvSetImageCOI( IplImage* image, int coi )
{
    if( !image )
        CV_Error( CV_HeaderIsNull, "" );

    if( (unsigned)coi > (unsigned)(image->nChannels) )
        CV_Error( CV_BadCOI, "" );

    if( image->roi || coi != 0 )
    {
        if( image->roi )
        {
            image->roi->coi = coi;
        }
        else
        {
            image->roi = icvCreateROI( coi, 0, 0, image->width, image->height );
        }
    }
}


CV_IMPL int
cvGetImageCOI( const IplImage* image )
{
    if( !image )
        CV_Error( CV_HeaderIsNull, "" );

    return image->roi ? image->roi->coi : 0;
}


CV_IMPL IplImage*
cvCloneImage( const IplImage* src )
{
    IplImage* dst = 0;

    if( !CV_IS_IMAGE_HDR( src ))
        CV_Error( CV_StsBadArg, "Bad image header" );

    if( !CvIPL.cloneImage )
    {
        dst = (IplImage*)cvAlloc( sizeof(*dst));

        memcpy( dst, src, sizeof(*src));
        dst->imageData = dst->imageDataOrigin = 0;
        dst->roi = 0;

        if( src->roi )
        {
            dst->roi = icvCreateROI( src->roi->coi, src->roi->xOffset,
                          src->roi->yOffset, src->roi->width, src->roi->height );
        }

        if( src->imageData )
        {
            int size = src->imageSize;
            cvCreateData( dst );
            memcpy( dst->imageData, src->imageData, size );
        }
    }
    else
        dst = CvIPL.cloneImage( src );

    return dst;
}


/****************************************************************************************\
*                            Additional operations on CvTermCriteria                     *
\****************************************************************************************/

CV_IMPL CvTermCriteria
cvCheckTermCriteria( CvTermCriteria criteria, double default_eps,
                     int default_max_iters )
{
    CvTermCriteria crit;

    crit.type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
    crit.max_iter = default_max_iters;
    crit.epsilon = (float)default_eps;

    if( (criteria.type & ~(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER)) != 0 )
        CV_Error( CV_StsBadArg,
                  "Unknown type of term criteria" );

    if( (criteria.type & CV_TERMCRIT_ITER) != 0 )
    {
        if( criteria.max_iter <= 0 )
            CV_Error( CV_StsBadArg,
                  "Iterations flag is set and maximum number of iterations is <= 0" );
        crit.max_iter = criteria.max_iter;
    }

    if( (criteria.type & CV_TERMCRIT_EPS) != 0 )
    {
        if( criteria.epsilon < 0 )
            CV_Error( CV_StsBadArg, "Accuracy flag is set and epsilon is < 0" );

        crit.epsilon = criteria.epsilon;
    }

    if( (criteria.type & (CV_TERMCRIT_EPS | CV_TERMCRIT_ITER)) == 0 )
        CV_Error( CV_StsBadArg,
                  "Neither accuracy nor maximum iterations "
                  "number flags are set in criteria type" );

    crit.epsilon = (float)MAX( 0, crit.epsilon );
    crit.max_iter = MAX( 1, crit.max_iter );

    return crit;
}

namespace cv
{

template<> void Ptr<CvMat>::delete_obj()
{ cvReleaseMat(&obj); }

template<> void Ptr<IplImage>::delete_obj()
{ cvReleaseImage(&obj); }

template<> void Ptr<CvMatND>::delete_obj()
{ cvReleaseMatND(&obj); }


template<> void Ptr<CvMemStorage>::delete_obj()
{ cvReleaseMemStorage(&obj); }


}

/* End of file. */

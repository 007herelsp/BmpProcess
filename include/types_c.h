#ifndef __OPENCV_CORE_TYPES_H__
#define __OPENCV_CORE_TYPES_H__

#ifndef SKIP_INCLUDES

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#if !defined _MSC_VER && !defined __BORLANDC__
#include <stdint.h>
#endif

#if (defined _M_X64 && defined _MSC_VER && _MSC_VER >= 1400) || (__GNUC__ >= 4 && defined __x86_64__)
#if defined WIN32
#include <intrin.h>
#endif
#endif

#include <math.h>

#endif // SKIP_INCLUDES

#if defined WIN32 || defined _WIN32
#define CV_CDECL __cdecl
#define CV_STDCALL __stdcall
#else
#define CV_CDECL
#define CV_STDCALL
#endif

#ifndef CV_EXTERN_C
#ifdef __cplusplus
#define CV_EXTERN_C extern "C"
#define CV_DEFAULT(val) = val
#else
#define CV_EXTERN_C
#define CV_DEFAULT(val)
#endif
#endif

#ifndef CV_EXTERN_C_FUNCPTR
#ifdef __cplusplus
#define CV_EXTERN_C_FUNCPTR(x) \
    extern "C" {               \
    typedef x;                 \
    }
#else
#define CV_EXTERN_C_FUNCPTR(x) typedef x
#endif
#endif

#ifndef CV_INLINE
#if defined __cplusplus
#define CV_INLINE inline
#elif defined _MSC_VER
#define CV_INLINE __inline
#else
#define CV_INLINE static
#endif
#endif /* CV_INLINE */

#if (defined WIN32 || defined _WIN32 || defined WINCE) && defined CVAPI_EXPORTS
#define CV_EXPORTS __declspec(dllexport)
#else
#define CV_EXPORTS
#endif

#ifndef CVAPI
#define CVAPI(rettype) CV_EXTERN_C CV_EXPORTS rettype CV_CDECL
#endif

#if defined _MSC_VER
typedef __int64 int64;
typedef unsigned __int64 uint64;
#define CV_BIG_INT(n) n##I64
#define CV_BIG_UINT(n) n##UI64
#else
typedef int64_t int64;
typedef uint64_t uint64;
#define CV_BIG_INT(n) n##LL
#define CV_BIG_UINT(n) n##ULL
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;

typedef signed char schar;

/* special informative macros for wrapper generators */
#define CV_EXPORTS_W CV_EXPORTS

/* CvArr* is used to pass arbitrary
 * array-like data structures
 * into functions where the particular
 * array type is recognized at runtime:
 */
typedef void CvArr;

typedef union Cv32suf {
    int i;
    unsigned u;
    float f;
} Cv32suf;

typedef int CVStatus;

enum
{
    CV_StsOk = 0,                       /* everithing is ok                */
    CV_StsBackTrace = -1,               /* pseudo error for back trace     */
    CV_StsError = -2,                   /* unknown /unspecified error      */
    CV_StsInternal = -3,                /* internal error (bad state)      */
    CV_StsNoMem = -4,                   /* insufficient memory             */
    CV_StsBadArg = -5,                  /* function arg/param is bad       */
    CV_StsBadFunc = -6,                 /* unsupported function            */
    CV_StsNoConv = -7,                  /* iter. didn't converge           */
    CV_StsAutoTrace = -8,               /* tracing                         */
    CV_HeaderIsNull = -9,               /* image header is NULL            */
    CV_BadImageSize = -10,              /* image size is invalid           */
    CV_BadOffset = -11,                 /* offset is invalid               */
    CV_BadDataPtr = -12,                /**/
    CV_BadStep = -13,                   /**/
    CV_BadModelOrChSeq = -14,           /**/
    CV_BadNumChannels = -15,            /**/
    CV_BadNumChannel1U = -16,           /**/
    CV_BadDepth = -17,                  /**/
    CV_BadAlphaChannel = -18,           /**/
    CV_BadOrder = -19,                  /**/
    CV_BadOrigin = -20,                 /**/
    CV_BadAlign = -21,                  /**/
    CV_BadCallBack = -22,               /**/
    CV_BadTileSize = -23,               /**/
    CV_BadCOI = -24,                    /**/
    CV_BadROISize = -25,                /**/
    CV_MaskIsTiled = -26,               /**/
    CV_StsNullPtr = -27,                /* null pointer */
    CV_StsVecLengthErr = -28,           /* incorrect vector length */
    CV_StsFilterStructContentErr = -29, /* incorr. filter structure content */
    CV_StsKernelStructContentErr = -30, /* incorr. transform kernel content */
    CV_StsFilterOffsetErr = -31,        /* incorrect filter offset value */
    CV_StsBadSize = -201,               /* the input/output structure size is incorrect  */
    CV_StsDivByZero = -202,             /* division by zero */
    CV_StsInplaceNotSupported = -203,   /* in-place operation is not supported */
    CV_StsObjectNotFound = -204,        /* request can't be completed */
    CV_StsUnmatchedFormats = -205,      /* formats of input/output arrays differ */
    CV_StsBadFlag = -206,               /* flag is wrong or not supported */
    CV_StsBadPoint = -207,              /* bad CvPoint */
    CV_StsBadMask = -208,               /* bad format of mask (neither 8uC1 nor 8sC1)*/
    CV_StsUnmatchedSizes = -209,        /* sizes of input/output structures do not match */
    CV_StsUnsupportedFormat = -210,     /* the data format/type is not supported by the function*/
    CV_StsOutOfRange = -211,            /* some of parameters are out of range */
    CV_StsParseError = -212,            /* invalid syntax/structure of the parsed file */
    CV_StsNotImplemented = -213,        /* the requested function/feature is not implemented */
    CV_StsBadMemBlock = -214,           /* an allocated block has been corrupted */
    CV_StsAssert = -215,                /* assertion failed */
    CV_GpuNotSupported = -216,
    CV_GpuApiCallError = -217,
    CV_OpenGlNotSupported = -218,
    CV_OpenGlApiCallError = -219,
    CV_OpenCLDoubleNotSupported = -220,
    CV_OpenCLInitError = -221,
    CV_OpenCLNoAMDBlasFft = -222
};

/****************************************************************************************\
*                             Common macros and inline functions                         *
\****************************************************************************************/

#define CV_PI 3.1415926535897932384626433832795

#define CV_SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

/* min & max without jumps */
#define CV_IMIN(a, b) ((a) ^ (((a) ^ (b)) & (((a) < (b)) - 1)))

#define CV_IMAX(a, b) ((a) ^ (((a) ^ (b)) & (((a) > (b)) - 1)))

/* absolute value without jumps */
#define CV_CMP(a, b) (((a) > (b)) - ((a) < (b)))
#define CV_SIGN(a) CV_CMP((a), 0)

CV_INLINE int cvRound(double value)
{
    return (int)round(value);
}

CV_INLINE int cvFloor(double value)
{
	return (int)floor(value);
}

    /*************** Random number generation *******************/

#define CV_RNG_COEFF 4164903690U

    /****************************************************************************************\
*                                  Image type (IplImage)                                 *
\****************************************************************************************/

#ifndef HAVE_IPL

/*
 * The following definitions (until #endif)
 * is an extract from IPL headers.
 * Copyright (c) 1995 Intel Corporation.
 */
#define IPL_DEPTH_SIGN 0x80000000

#define IPL_DEPTH_1U 1
#define IPL_DEPTH_8U 8
#define IPL_DEPTH_16U 16
#define IPL_DEPTH_32F 32

#define IPL_DEPTH_8S (IPL_DEPTH_SIGN | 8)
#define IPL_DEPTH_16S (IPL_DEPTH_SIGN | 16)
#define IPL_DEPTH_32S (IPL_DEPTH_SIGN | 32)

#define IPL_DATA_ORDER_PIXEL 0
#define IPL_DATA_ORDER_PLANE 1

#define IPL_ORIGIN_TL 0
#define IPL_ORIGIN_BL 1

#define IPL_ALIGN_4BYTES 4
#define IPL_ALIGN_8BYTES 8
#define IPL_ALIGN_16BYTES 16
#define IPL_ALIGN_32BYTES 32

#define IPL_ALIGN_DWORD IPL_ALIGN_4BYTES
#define IPL_ALIGN_QWORD IPL_ALIGN_8BYTES

#define IPL_BORDER_CONSTANT 0
#define IPL_BORDER_REPLICATE 1
#define IPL_BORDER_REFLECT 2
#define IPL_BORDER_WRAP 3

typedef struct _IplImage
{
    int nSize;                     /* sizeof(IplImage) */
    int ID;                        /* version (=0)*/
    int nChannels;                 /* Most of OpenCV functions support 1,2,3 or 4 channels */
    int alphaChannel;              /* Ignored by OpenCV */
    int depth;                     /* Pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                               IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported.  */
    char colorModel[4];            /* Ignored by OpenCV */
    char channelSeq[4];            /* ditto */
    int dataOrder;                 /* 0 - interleaved color channels, 1 - separate color channels.
                               cvCreateImage can only create interleaved images */
    int origin;                    /* 0 - top-left origin,
                               1 - bottom-left origin (Windows bitmaps style).  */
    int align;                     /* Alignment of image rows (4 or 8).
                               OpenCV ignores it and uses widthStep instead.    */
    int width;                     /* Image width in pixels.                           */
    int height;                    /* Image height in pixels.                          */
    void *imageId;                 /* "           " */
    struct _IplTileInfo *tileInfo; /* "           " */
    int imageSize;                 /* Image data size in bytes
                               (==image->height*image->widthStep
                               in case of interleaved data)*/
    char *imageData;               /* Pointer to aligned image data.         */
    int widthStep;                 /* Size of aligned image row in bytes.    */
    int BorderMode[4];             /* Ignored by OpenCV.                     */
    int BorderConst[4];            /* Ditto.                                 */
    char *imageDataOrigin;         /* Pointer to very origin of image data
                               (not necessarily aligned) -
                               needed for correct deallocation */
} IplImage;

typedef struct _IplTileInfo IplTileInfo;

typedef struct _IplConvKernel
{
    int nCols;
    int nRows;
    int anchorX;
    int anchorY;
    int *values;
    int nShiftR;
} IplConvKernel;


#define IPL_IMAGE_HEADER 1
#define IPL_IMAGE_DATA 2
#define IPL_IMAGE_ROI 4

#endif /*HAVE_IPL*/

/* extra border mode */
#define IPL_BORDER_REFLECT_101 4
#define IPL_BORDER_TRANSPARENT 5

#define IPL_IMAGE_MAGIC_VAL ((int)sizeof(IplImage))
#define CV_TYPE_NAME_IMAGE "opencv-image"

#define CV_IS_IMAGE_HDR(img) \
    ((img) != NULL && ((const IplImage *)(img))->nSize == sizeof(IplImage))

#define CV_IS_IMAGE(img) \
    (CV_IS_IMAGE_HDR(img) && ((IplImage *)img)->imageData != NULL)

/* for storing double-precision
   floating point data in IplImage's */
#define IPL_DEPTH_64F 64

/* get reference to pixel at (col,row),
   for multi-channel images (col) should be multiplied by number of channels */
#define CV_IMAGE_ELEM(image, elemtype, row, col) \
    (((elemtype *)((image)->imageData + (image)->widthStep * (row)))[(col)])

/****************************************************************************************\
*                                  Matrix type (CvMat)                                   *
\****************************************************************************************/

#define CV_CN_MAX 512
#define CV_CN_SHIFT 3
#define CV_DEPTH_MAX (1 << CV_CN_SHIFT)

#define CV_8U 0
#define CV_8S 1
#define CV_16U 2
#define CV_16S 3
#define CV_32S 4
#define CV_32F 5
#define CV_64F 6
#define CV_USRTYPE1 7

#define CV_MAT_DEPTH_MASK (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags) ((flags)&CV_MAT_DEPTH_MASK)

#define CV_MAKETYPE(depth, cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))
#define CV_MAKE_TYPE CV_MAKETYPE

#define CV_8UC1 CV_MAKETYPE(CV_8U, 1)
#define CV_8UC2 CV_MAKETYPE(CV_8U, 2)
#define CV_8UC3 CV_MAKETYPE(CV_8U, 3)
#define CV_8UC4 CV_MAKETYPE(CV_8U, 4)
#define CV_8UC(n) CV_MAKETYPE(CV_8U, (n))

#define CV_8SC1 CV_MAKETYPE(CV_8S, 1)
#define CV_8SC2 CV_MAKETYPE(CV_8S, 2)
#define CV_8SC3 CV_MAKETYPE(CV_8S, 3)
#define CV_8SC4 CV_MAKETYPE(CV_8S, 4)
#define CV_8SC(n) CV_MAKETYPE(CV_8S, (n))

#define CV_16UC1 CV_MAKETYPE(CV_16U, 1)
#define CV_16UC2 CV_MAKETYPE(CV_16U, 2)
#define CV_16UC3 CV_MAKETYPE(CV_16U, 3)
#define CV_16UC4 CV_MAKETYPE(CV_16U, 4)
#define CV_16UC(n) CV_MAKETYPE(CV_16U, (n))

#define CV_16SC1 CV_MAKETYPE(CV_16S, 1)
#define CV_16SC2 CV_MAKETYPE(CV_16S, 2)
#define CV_16SC3 CV_MAKETYPE(CV_16S, 3)
#define CV_16SC4 CV_MAKETYPE(CV_16S, 4)
#define CV_16SC(n) CV_MAKETYPE(CV_16S, (n))

#define CV_32SC1 CV_MAKETYPE(CV_32S, 1)
#define CV_32SC2 CV_MAKETYPE(CV_32S, 2)

#define CV_32FC1 CV_MAKETYPE(CV_32F, 1)
#define CV_32FC2 CV_MAKETYPE(CV_32F, 2)

#define CV_MAT_CN_MASK ((CV_CN_MAX - 1) << CV_CN_SHIFT)
#define CV_MAT_CN(flags) ((((flags)&CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#define CV_MAT_TYPE_MASK (CV_DEPTH_MAX * CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags) ((flags)&CV_MAT_TYPE_MASK)
#define CV_MAT_CONT_FLAG_SHIFT 14
#define CV_MAT_CONT_FLAG (1 << CV_MAT_CONT_FLAG_SHIFT)
#define CV_IS_MAT_CONT(flags) ((flags)&CV_MAT_CONT_FLAG)
#define CV_IS_CONT_MAT CV_IS_MAT_CONT
#define CV_SUBMAT_FLAG_SHIFT 15
#define CV_SUBMAT_FLAG (1 << CV_SUBMAT_FLAG_SHIFT)
#define CV_IS_SUBMAT(flags) ((flags)&CV_MAT_SUBMAT_FLAG)

#define CV_MAGIC_MASK 0xFFFF0000
#define CV_MAT_MAGIC_VAL 0x42420000

typedef struct CvMat
{
    int type;
    int step;

    /* for internal use only */
    int *refcount;
    int hdr_refcount;

    union {
        uchar *ptr;
        short *s;
        int *i;
        float *fl;
        double *db;
    } data;

#ifdef __cplusplus
    union {
        int rows;
        int height;
    };

    union {
        int cols;
        int width;
    };
#else
    int rows;
    int cols;
#endif

} CvMat;

#define CV_IS_MAT_HDR(mat)                                                 \
    ((mat) != NULL &&                                                      \
     (((const CvMat *)(mat))->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL && \
     ((const CvMat *)(mat))->cols > 0 && ((const CvMat *)(mat))->rows > 0)

#define CV_IS_MAT_HDR_Z(mat)                                               \
    ((mat) != NULL &&                                                      \
     (((const CvMat *)(mat))->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL && \
     ((const CvMat *)(mat))->cols >= 0 && ((const CvMat *)(mat))->rows >= 0)

#define CV_IS_MAT(mat) \
    (CV_IS_MAT_HDR(mat) && ((const CvMat *)(mat))->data.ptr != NULL)

#define CV_IS_MASK_ARR(mat) \
    (((mat)->type & (CV_MAT_TYPE_MASK & ~CV_8SC1)) == 0)


/* Size of each channel item,
   0x124489 = 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define CV_ELEM_SIZE1(type) \
    ((((sizeof(size_t) << 28) | 0x8442211) >> CV_MAT_DEPTH(type) * 4) & 15)

/* 0x3a50 = 11 10 10 01 01 00 00 ~ array of log2(sizeof(arr_type_elem)) */
#define CV_ELEM_SIZE(type) \
    (CV_MAT_CN(type) << ((((sizeof(size_t) / 4 + 1) * 16384 | 0x3a50) >> CV_MAT_DEPTH(type) * 2) & 3))

#define IPL2CV_DEPTH(depth)                                                       \
    ((((CV_8U) + (CV_16U << 4) + (CV_32F << 8) + (CV_64F << 16) + (CV_8S << 20) + \
       (CV_16S << 24) + (CV_32S << 28)) >>                                        \
      ((((depth)&0xF0) >> 2) +                                                    \
       (((depth)&IPL_DEPTH_SIGN) ? 20 : 0))) &                                    \
     15)

/* Inline constructor. No data is allocated internally!!!
 * (Use together with cvCreateData, or use cvCreateMat instead to
 * get a matrix with allocated data):
 */
CV_INLINE CvMat cvMat(int rows, int cols, int type, void *data CV_DEFAULT(NULL))
{
    CvMat m;

    assert((unsigned)CV_MAT_DEPTH(type) <= CV_64F);
    type = CV_MAT_TYPE(type);
    m.type = CV_MAT_MAGIC_VAL | CV_MAT_CONT_FLAG | type;
    m.cols = cols;
    m.rows = rows;
    m.step = m.cols * CV_ELEM_SIZE(type);
    m.data.ptr = (uchar *)data;
    m.refcount = NULL;
    m.hdr_refcount = 0;

    return m;
}

CV_INLINE int cvIplDepth(int type)
{
    int depth = CV_MAT_DEPTH(type);
    return CV_ELEM_SIZE1(depth) * 8 | (depth == CV_8S || depth == CV_16S ||
                                               depth == CV_32S
                                           ? IPL_DEPTH_SIGN
                                           : 0);
}

#define CV_MAX_DIM 32


/****************************************************************************************\
*                      Other supplementary data type definitions                         *
\****************************************************************************************/

/*************************************** CvRect *****************************************/

typedef struct CvRect
{
    int x;
    int y;
    int width;
    int height;
} CvRect;

CV_INLINE CvRect cvRect(int x, int y, int width, int height)
{
    CvRect r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}


/******************************* CvPoint and variants ***********************************/

typedef struct CvPoint
{
    int x;
    int y;
} CvPoint;

CV_INLINE CvPoint cvPoint(int x, int y)
{
    CvPoint p;

    p.x = x;
    p.y = y;

    return p;
}

typedef struct CvPoint2D32f
{
    float x;
    float y;
} CvPoint2D32f;

CV_INLINE CvPoint2D32f cvPoint2D32f(double x, double y)
{
    CvPoint2D32f p;

    p.x = (float)x;
    p.y = (float)y;

    return p;
}

CV_INLINE CvPoint2D32f cvPointTo32f(CvPoint point)
{
    return cvPoint2D32f((float)point.x, (float)point.y);
}

CV_INLINE CvPoint cvPointFrom32f(CvPoint2D32f point)
{
    CvPoint ipt;
    ipt.x = cvRound(point.x);
    ipt.y = cvRound(point.y);

    return ipt;
}

/******************************** CvSize's & CvBox **************************************/

typedef struct CvSize
{
    int width;
    int height;
} CvSize;

CV_INLINE CvSize cvSize(int width, int height)
{
    CvSize s;

    s.width = width;
    s.height = height;

    return s;
}

typedef struct CvSize2D32f
{
    float width;
    float height;
} CvSize2D32f;

CV_INLINE CvSize2D32f cvSize2D32f(double width, double height)
{
    CvSize2D32f s;

    s.width = (float)width;
    s.height = (float)height;

    return s;
}

typedef struct CvBox2D
{
    CvPoint2D32f center; /* Center of the box.                          */
    CvSize2D32f size;    /* Box width and length.                       */
    float angle;         /* Angle between the horizontal axis           */
                         /* and the first side (i.e. length) in degrees */
} CvBox2D;


/************************************* CvSlice ******************************************/

typedef struct CvSlice
{
    int start_index, end_index;
} CvSlice;

CV_INLINE CvSlice cvSlice(int start, int end)
{
    CvSlice slice;
    slice.start_index = start;
    slice.end_index = end;

    return slice;
}

#define CV_WHOLE_SEQ_END_INDEX 0x3fffffff
#define CV_WHOLE_SEQ cvSlice(0, CV_WHOLE_SEQ_END_INDEX)

/************************************* CvScalar *****************************************/

typedef struct CvScalar
{
    double val[4];
} CvScalar;

CV_INLINE CvScalar cvScalar(double val0, double val1 CV_DEFAULT(0),
                            double val2 CV_DEFAULT(0), double val3 CV_DEFAULT(0))
{
    CvScalar scalar;
    scalar.val[0] = val0;
    scalar.val[1] = val1;
    scalar.val[2] = val2;
    scalar.val[3] = val3;
    return scalar;
}


CV_INLINE CvScalar cvScalarAll(double val0123)
{
    CvScalar scalar;
    scalar.val[0] = val0123;
    scalar.val[1] = val0123;
    scalar.val[2] = val0123;
    scalar.val[3] = val0123;
    return scalar;
}

/****************************************************************************************\
*                                   Dynamic Data structures                              *
\****************************************************************************************/

/******************************** Memory storage ****************************************/

typedef struct CvMemBlock
{
    struct CvMemBlock *prev;
    struct CvMemBlock *next;
} CvMemBlock;

#define CV_STORAGE_MAGIC_VAL 0x42890000

typedef struct CvMemStorage
{
    int signature;
    CvMemBlock *bottom;          /* First allocated block.                   */
    CvMemBlock *top;             /* Current memory block - top of the stack. */
    struct CvMemStorage *parent; /* We get new blocks from parent as needed. */
    int block_size;              /* Block size.                              */
    int free_space;              /* Remaining free space in current block.   */
} CvMemStorage;

#define CV_IS_STORAGE(storage) \
    ((storage) != NULL &&      \
     (((CvMemStorage *)(storage))->signature & CV_MAGIC_MASK) == CV_STORAGE_MAGIC_VAL)

typedef struct CvMemStoragePos
{
    CvMemBlock *top;
    int free_space;
} CvMemStoragePos;

/*********************************** Sequence *******************************************/

typedef struct CvSeqBlock
{
    struct CvSeqBlock *prev; /* Previous sequence block.                   */
    struct CvSeqBlock *next; /* Next sequence block.                       */
    int start_index;         /* Index of the first element in the block +  */
                             /* sequence->first->start_index.              */
    int count;               /* Number of elements in the block.           */
    schar *data;             /* Pointer to the first element of the block. */
} CvSeqBlock;

#define CV_TREE_NODE_FIELDS(node_type)                       \
    int flags;                /* Miscellaneous flags.     */ \
    int header_size;          /* Size of sequence header. */ \
    struct node_type *h_prev; /* Previous sequence.       */ \
    struct node_type *h_next; /* Next sequence.           */ \
    struct node_type *v_prev; /* 2nd previous sequence.   */ \
    struct node_type *v_next  /* 2nd next sequence.       */

/*
   Read/Write sequence.
   Elements can be dynamically inserted to or deleted from the sequence.
*/
#define CV_SEQUENCE_FIELDS()                                            \
    CV_TREE_NODE_FIELDS(CvSeq);                                         \
    int total;               /* Total number of elements.            */ \
    int elem_size;           /* Size of sequence element in bytes.   */ \
    schar *block_max;        /* Maximal bound of the last block.     */ \
    schar *ptr;              /* Current write pointer.               */ \
    int delta_elems;         /* Grow seq this many at a time.        */ \
    CvMemStorage *storage;   /* Where the seq is stored.             */ \
    CvSeqBlock *free_blocks; /* Free blocks list.                    */ \
    CvSeqBlock *first;       /* Pointer to the first sequence block. */

typedef struct CvSeq
{
    CV_SEQUENCE_FIELDS()
} CvSeq;

#define CV_TYPE_NAME_SEQ "opencv-sequence"
#define CV_TYPE_NAME_SEQ_TREE "opencv-sequence-tree"

/*************************************** Set ********************************************/
/*
  Set.
  Order is not preserved. There can be gaps between sequence elements.
  After the element has been inserted it stays in the same place all the time.
  The MSB(most-significant or sign bit) of the first field (flags) is 0 iff the element exists.
*/
#define CV_SET_ELEM_FIELDS(elem_type) \
    int flags;                        \
    struct elem_type *next_free;

typedef struct CvSetElem
{
    CV_SET_ELEM_FIELDS(CvSetElem)
} CvSetElem;

#define CV_SET_FIELDS()    \
    CV_SEQUENCE_FIELDS()   \
    CvSetElem *free_elems; \
    int active_count;

typedef struct CvSet
{
    CV_SET_FIELDS()
} CvSet;

#define CV_SET_ELEM_IDX_MASK ((1 << 26) - 1)
#define CV_SET_ELEM_FREE_FLAG (1 << (sizeof(int) * 8 - 1))

/* Checks whether the element pointed by ptr belongs to a set or not */
#define CV_IS_SET_ELEM(ptr) (((CvSetElem *)(ptr))->flags >= 0)



/*********************************** Chain/Countour *************************************/

typedef struct CvChain
{
    CV_SEQUENCE_FIELDS()
    CvPoint origin;
} CvChain;

#define CV_CONTOUR_FIELDS() \
    CV_SEQUENCE_FIELDS()    \
    CvRect rect;            \
    int color;              \
    int reserved[3];

typedef struct CvContour
{
    CV_CONTOUR_FIELDS()
} CvContour;


/****************************************************************************************\
*                                    Sequence types                                      *
\****************************************************************************************/

#define CV_SEQ_MAGIC_VAL 0x42990000

#define CV_IS_SEQ(seq) \
    ((seq) != NULL && (((CvSeq *)(seq))->flags & CV_MAGIC_MASK) == CV_SEQ_MAGIC_VAL)

#define CV_SET_MAGIC_VAL 0x42980000
#define CV_IS_SET(set) \
    ((set) != NULL && (((CvSeq *)(set))->flags & CV_MAGIC_MASK) == CV_SET_MAGIC_VAL)

#define CV_SEQ_ELTYPE_BITS 12
#define CV_SEQ_ELTYPE_MASK ((1 << CV_SEQ_ELTYPE_BITS) - 1)

#define CV_SEQ_ELTYPE_POINT CV_32SC2 /* (x,y) */
#define CV_SEQ_ELTYPE_CODE CV_8UC1   /* freeman code: 0..7 */
#define CV_SEQ_ELTYPE_GENERIC 0
#define CV_SEQ_ELTYPE_PTR CV_USRTYPE1
#define CV_SEQ_ELTYPE_PPOINT CV_SEQ_ELTYPE_PTR /* &(x,y) */
#define CV_SEQ_ELTYPE_INDEX CV_32SC1           /* #(x,y) */
#define CV_SEQ_ELTYPE_GRAPH_EDGE 0             /* &next_o, &next_d, &vtx_o, &vtx_d */
#define CV_SEQ_ELTYPE_GRAPH_VERTEX 0           /* first_edge, &(x,y) */
#define CV_SEQ_ELTYPE_TRIAN_ATR 0              /* vertex of the binary tree   */
#define CV_SEQ_ELTYPE_CONNECTED_COMP 0         /* connected component  */

#define CV_SEQ_KIND_BITS 2
#define CV_SEQ_KIND_MASK (((1 << CV_SEQ_KIND_BITS) - 1) << CV_SEQ_ELTYPE_BITS)

/* types of sequences */
#define CV_SEQ_KIND_GENERIC (0 << CV_SEQ_ELTYPE_BITS)
#define CV_SEQ_KIND_CURVE (1 << CV_SEQ_ELTYPE_BITS)
#define CV_SEQ_KIND_BIN_TREE (2 << CV_SEQ_ELTYPE_BITS)

/* types of sparse sequences (sets) */
#define CV_SEQ_KIND_GRAPH (1 << CV_SEQ_ELTYPE_BITS)
#define CV_SEQ_KIND_SUBDIV2D (2 << CV_SEQ_ELTYPE_BITS)

#define CV_SEQ_FLAG_SHIFT (CV_SEQ_KIND_BITS + CV_SEQ_ELTYPE_BITS)

/* flags for curves */
#define CV_SEQ_FLAG_CLOSED (1 << CV_SEQ_FLAG_SHIFT)
#define CV_SEQ_FLAG_SIMPLE (0 << CV_SEQ_FLAG_SHIFT)
#define CV_SEQ_FLAG_CONVEX (0 << CV_SEQ_FLAG_SHIFT)
#define CV_SEQ_FLAG_HOLE (2 << CV_SEQ_FLAG_SHIFT)

/* point sets */
#define CV_SEQ_POINT_SET (CV_SEQ_KIND_GENERIC | CV_SEQ_ELTYPE_POINT)
#define CV_SEQ_POLYLINE (CV_SEQ_KIND_CURVE | CV_SEQ_ELTYPE_POINT)
#define CV_SEQ_POLYGON (CV_SEQ_FLAG_CLOSED | CV_SEQ_POLYLINE)

/* chain-coded curves */
#define CV_SEQ_CHAIN (CV_SEQ_KIND_CURVE | CV_SEQ_ELTYPE_CODE)
#define CV_SEQ_CHAIN_CONTOUR (CV_SEQ_FLAG_CLOSED | CV_SEQ_CHAIN)

/* binary tree for the contour */

/* sequence of the connected components */

/* sequence of the integer numbers */
#define CV_SEQ_INDEX (CV_SEQ_KIND_GENERIC | CV_SEQ_ELTYPE_INDEX)

#define CV_SEQ_ELTYPE(seq) ((seq)->flags & CV_SEQ_ELTYPE_MASK)
#define CV_SEQ_KIND(seq) ((seq)->flags & CV_SEQ_KIND_MASK)

/* flag checking */

#define CV_IS_SEQ_CLOSED(seq) (((seq)->flags & CV_SEQ_FLAG_CLOSED) != 0)
#define CV_IS_SEQ_HOLE(seq) (((seq)->flags & CV_SEQ_FLAG_HOLE) != 0)
#define CV_IS_SEQ_SIMPLE(seq) 1

/* type checking macros */
#define CV_IS_SEQ_POINT_SET(seq) \
    ((CV_SEQ_ELTYPE(seq) == CV_32SC2 || CV_SEQ_ELTYPE(seq) == CV_32FC2))

#define CV_IS_SEQ_POLYLINE(seq) \
    (CV_SEQ_KIND(seq) == CV_SEQ_KIND_CURVE && CV_IS_SEQ_POINT_SET(seq))

#define CV_IS_SEQ_POLYGON(seq) \
    (CV_IS_SEQ_POLYLINE(seq) && CV_IS_SEQ_CLOSED(seq))

#define CV_IS_SEQ_CHAIN(seq) \
    (CV_SEQ_KIND(seq) == CV_SEQ_KIND_CURVE && (seq)->elem_size == 1)


/****************************************************************************************/
/*                            Sequence writer & reader                                  */
/****************************************************************************************/

#define CV_SEQ_WRITER_FIELDS()                                \
    int header_size;                                          \
    CvSeq *seq;        /* the sequence written */             \
    CvSeqBlock *block; /* current block */                    \
    schar *ptr;        /* pointer to free space */            \
    schar *block_min;  /* pointer to the beginning of block*/ \
    schar *block_max;  /* pointer to the end of block */

typedef struct CvSeqWriter
{
    CV_SEQ_WRITER_FIELDS()
} CvSeqWriter;

#define CV_SEQ_READER_FIELDS()                                 \
    int header_size;                                           \
    CvSeq *seq;        /* sequence, beign read */              \
    CvSeqBlock *block; /* current block */                     \
    schar *ptr;        /* pointer to element be read next */   \
    schar *block_min;  /* pointer to the beginning of block */ \
    schar *block_max;  /* pointer to the end of block */       \
    int delta_index;   /* = seq->first->start_index   */       \
    schar *prev_elem;  /* pointer to previous element */

typedef struct CvSeqReader
{
    CV_SEQ_READER_FIELDS()
} CvSeqReader;

/****************************************************************************************/
/*                                Operations on sequences                               */
/****************************************************************************************/

#define CV_SEQ_ELEM(seq, elem_type, index) \
/* assert gives some guarantee that <seq> parameter is valid */                                \
    \
(assert(sizeof((seq)->first[0]) == sizeof(CvSeqBlock) &&                     \
        (seq)->elem_size == sizeof(elem_type)),                              \
        (elem_type *)((seq)->first && (unsigned)index <                      \
                                          (unsigned)((seq)->first->count)    \
                          ? (seq)->first->data + (index) * sizeof(elem_type) \
                          : cvGetSeqElem((CvSeq *)(seq), (index))))
#define CV_GET_SEQ_ELEM(elem_type, seq, index) CV_SEQ_ELEM((seq), elem_type, (index))

/* Add element to sequence: */
#define CV_WRITE_SEQ_ELEM_VAR(elem_ptr, writer)                  \
    \
{                                                           \
        if ((writer).ptr >= (writer).block_max)                  \
        {                                                        \
            cvCreateSeqBlock(&writer);                           \
        }                                                        \
        memcpy((writer).ptr, elem_ptr, (writer).seq->elem_size); \
        (writer).ptr += (writer).seq->elem_size;                 \
    \
}

#define CV_WRITE_SEQ_ELEM(elem, writer)                            \
    \
{                                                             \
        assert((writer).seq->elem_size == sizeof(elem));           \
        if ((writer).ptr >= (writer).block_max)                    \
        {                                                          \
            cvCreateSeqBlock(&writer);                             \
        }                                                          \
        assert((writer).ptr <= (writer).block_max - sizeof(elem)); \
        memcpy((writer).ptr, &(elem), sizeof(elem));               \
        (writer).ptr += sizeof(elem);                              \
    \
}

/* Move reader position forward: */
#define CV_NEXT_SEQ_ELEM(elem_size, reader)                      \
    \
{                                                           \
        if (((reader).ptr += (elem_size)) >= (reader).block_max) \
        {                                                        \
            cvChangeSeqBlock(&(reader), 1);                      \
        }                                                        \
    \
}

/* Move reader position backward: */
#define CV_PREV_SEQ_ELEM(elem_size, reader)                     \
    \
{                                                          \
        if (((reader).ptr -= (elem_size)) < (reader).block_min) \
        {                                                       \
            cvChangeSeqBlock(&(reader), -1);                    \
        }                                                       \
    \
}

/* Read element and move read position forward: */
#define CV_READ_SEQ_ELEM(elem, reader)                   \
    \
{                                                   \
        assert((reader).seq->elem_size == sizeof(elem)); \
        memcpy(&(elem), (reader).ptr, sizeof((elem)));   \
        CV_NEXT_SEQ_ELEM(sizeof(elem), reader)           \
    \
}


	
#ifdef __cplusplus
	extern "C" {
#endif
	
	
	/* Image smooth methods */
	enum
	{
		CV_BLUR_NO_SCALE =0,
		CV_BLUR  =1,
		CV_GAUSSIAN  =2,
		CV_MEDIAN =3,
		CV_BILATERAL =4
	};
	
	/* Filters used in pyramid decomposition */
	enum
	{
		CV_GAUSSIAN_5x5 = 7
	};
	
	/* Special filters */
	enum
	{
		CV_SCHARR =-1,
		CV_MAX_SOBEL_KSIZE =7
	};
	
	/* Constants for color conversion */
	enum
	{
	  
	
		CV_BGR2GRAY    =6,
		CV_RGB2GRAY    =7,
		CV_GRAY2BGR    =8,
		CV_GRAY2RGB    =CV_GRAY2BGR,
	 
	
		CV_COLORCVT_MAX  = CV_GRAY2RGB+1
	};
	
	
	/* Sub-pixel interpolation methods */
	enum
	{
		CV_INTER_NN 	   =0,
		CV_INTER_LINEAR    =1,
		CV_INTER_CUBIC	   =2,
		CV_INTER_AREA	   =3,
		CV_INTER_LANCZOS4  =4
	};
	
	/* ... and other image warping flags */
	enum
	{
		CV_WARP_FILL_OUTLIERS =8,
		CV_WARP_INVERSE_MAP  =16
	};
	
	/* Shapes of a structuring element for morphological operations */
	enum
	{
		CV_SHAPE_RECT	   =0,
		CV_SHAPE_CROSS	   =1,
		CV_SHAPE_ELLIPSE   =2,
		CV_SHAPE_CUSTOM    =100
	};
	
	/* Morphological operations */
	enum
	{
		CV_MOP_ERODE		=0,
		CV_MOP_DILATE		=1,
		CV_MOP_OPEN 		=2,
		CV_MOP_CLOSE		=3,
		CV_MOP_GRADIENT 	=4,
		CV_MOP_TOPHAT		=5,
		CV_MOP_BLACKHAT 	=6
	};
	
	
	
	/* Contour retrieval modes */
	enum
	{
		CV_RETR_EXTERNAL=0,
		CV_RETR_LIST=1,
		CV_RETR_CCOMP=2,
		CV_RETR_TREE=3,
		CV_RETR_FLOODFILL=4
	};
	
	/* Contour approximation methods */
	enum
	{
		CV_CHAIN_CODE=0,
		CV_CHAIN_APPROX_NONE=1,
		CV_CHAIN_APPROX_SIMPLE=2,
		CV_CHAIN_APPROX_TC89_L1=3,
		CV_CHAIN_APPROX_TC89_KCOS=4,
		CV_LINK_RUNS=5
	};
	
	/*
	Internal structure that is used for sequental retrieving contours from the image.
	It supports both hierarchical and plane variants of Suzuki algorithm.
	*/
	typedef struct _CvContourScanner* CvContourScanner;
	
	
	
	/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define  CV_INIT_3X3_DELTAS( deltas, step, nch )            \
		((deltas)[0] =	(nch),	(deltas)[1] = -(step) + (nch),	\
		 (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch),	\
		 (deltas)[4] = -(nch),	(deltas)[5] =  (step) - (nch),	\
		 (deltas)[6] =	(step), (deltas)[7] =  (step) + (nch))
	
	
	
	
	
	/* Contour approximation algorithms */
	enum
	{
		CV_POLY_APPROX_DP = 0
	};
	
	
	
	/* Shape orientation */
	enum
	{
		CV_CLOCKWISE		 =1,
		CV_COUNTER_CLOCKWISE =2
	};
	
	
	
	
	/* Threshold types */
	enum
	{
		CV_THRESH_BINARY	  =0,  /* value = value > threshold ? max_value : 0 	  */
		CV_THRESH_BINARY_INV  =1,  /* value = value > threshold ? 0 : max_value 	  */
		CV_THRESH_TRUNC 	  =2,  /* value = value > threshold ? threshold : value   */
		CV_THRESH_TOZERO	  =3,  /* value = value > threshold ? value : 0 		  */
		CV_THRESH_TOZERO_INV  =4,  /* value = value > threshold ? 0 : value 		  */
		CV_THRESH_MASK		  =7,
		CV_THRESH_OTSU		  =8  /* use Otsu algorithm to choose the optimal threshold value;
									 combine the flag with one of the above CV_THRESH_* values */
	};
	
	/* Adaptive threshold methods */
	enum
	{
		CV_ADAPTIVE_THRESH_MEAN_C  =0,
		CV_ADAPTIVE_THRESH_GAUSSIAN_C  =1
	};
	
	
	/* Canny edge detector flags */
	enum
	{
		CV_CANNY_L2_GRADIENT  =(1 << 31)
	};
	
	
#ifdef __cplusplus
	}
#endif


#endif /*__OPENCV_CORE_TYPES_H__*/




/* End of file. */

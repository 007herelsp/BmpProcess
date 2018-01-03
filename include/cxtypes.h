#ifndef _CXCORE_TYPES_H_
#define _CXCORE_TYPES_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>


#ifndef VOS_EXTERN_C
#ifdef __cplusplus
#define VOS_EXTERN_C extern "C"
#define VOS_DEFAULT(val) = val
#else
#define VOS_EXTERN_C
#define VOS_DEFAULT(val)
#endif
#endif

#ifndef VOS_INLINE
#if defined __cplusplus
#define VOS_INLINE inline
#elif (defined WIN32 || defined WIN64) && !defined __GNUC__
#define VOS_INLINE __inline
#else
#define VOS_INLINE static
#endif
#endif /* VOS_INLINE */



#if defined _MSC_VER
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif


typedef unsigned char uchar;
typedef unsigned short ushort;


/* CvArr* is used to pass arbitrary array-like data structures
   into the functions where the particular
   array type is recognized at runtime */
typedef void VOID;

typedef union Cv32suf
{
    int i;
    unsigned u;
    float f;
}
Cv32suf;


/****************************************************************************************\
*                             Common macros and inline functions                         *
\****************************************************************************************/

#define VOS_PI   3.1415926535897932384626433832795
#define VOS_LOG2 0.69314718055994530941723212145818

#define VOS_SWAP(a,b,t) ((t) = (a), (a) = (b), (b) = (t))
#define VOS_MIN(a,b)  ((a) > (b) ? (b) : (a))
#define VOS_MAX(a,b)  ((a) < (b) ? (b) : (a))
#define  VOS_CMP(a,b)    (((a) > (b)) - ((a) < (b)))
#define  VOS_SIGN(a)     VOS_CMP((a),0)

VOS_INLINE  int  SysRound( double value )
{
    return (int)round(value);
}


VOS_INLINE  int  SysFloor( double value )
{
    return (int)floor(value);
}


VOS_INLINE  int  SysCeil( double value )
{
    return ceil(value);
}

#define SYS_INVSQRT(value) ((float)(1./sqrt(value)))
#define SYS_SQRT(value)  ((float)sqrt(value))



/****************************************************************************************\
*                                  Image type (IplImage)                                 *
\****************************************************************************************/

#ifndef HAVE_IPL

#define IPL_DEPTH_SIGN 0x80000000

#define IPL_DEPTH_1U     1
#define IPL_DEPTH_8U     8
#define IPL_DEPTH_16U   16
#define IPL_DEPTH_32F   32

#define IPL_DEPTH_8S  (IPL_DEPTH_SIGN| 8)
#define IPL_DEPTH_16S (IPL_DEPTH_SIGN|16)
#define IPL_DEPTH_32S (IPL_DEPTH_SIGN|32)

#define IPL_DATA_ORDER_PIXEL  0
#define IPL_DATA_ORDER_PLANE  1

#define IPL_ORIGIN_TL 0
#define IPL_ORIGIN_BL 1

#define IPL_ALIGN_4BYTES   4
#define IPL_ALIGN_8BYTES   8
#define IPL_ALIGN_16BYTES 16
#define IPL_ALIGN_32BYTES 32

#define IPL_ALIGN_DWORD   IPL_ALIGN_4BYTES
#define IPL_ALIGN_QWORD   IPL_ALIGN_8BYTES

#define IPL_BORDER_CONSTANT   0
#define IPL_BORDER_REPLICATE  1
#define IPL_BORDER_REFLECT    2
#define IPL_BORDER_WRAP       3

typedef struct _IplImage
{
    int  nSize;         /* sizeof(IplImage) */
    int  ID;            /* version (=0)*/
    int  nChannels;     /* Most of   functions support 1,2,3 or 4 channels */
    int  alphaChannel;  /* ignored by   */
    int  depth;         /* pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                           IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported */
    int  dataOrder;     /* 0 - interleaved color channels, 1 - separate color channels.
                           CreateImage can only create interleaved images */
    int  origin;        /* 0 - top-left origin,
                           1 - bottom-left origin (Windows bitmaps style) */
    int  align;         /* Alignment of image rows (4 or 8).
                             ignores it and uses widthStep instead */
    int  width;         /* image width in pixels */
    int  height;        /* image height in pixels */
    void  *imageId;     /* ditto */
    int  imageSize;     /* image data size in bytes
                           (==image->height*image->widthStep
                           in case of interleaved data)*/
    char *imageData;  /* pointer to aligned image data */
    int  widthStep;   /* size of aligned image row in bytes */
    char *imageDataOrigin; /* pointer to very origin of image data
                              (not necessarily aligned) -
                              needed for correct deallocation */
}
IplImage;



typedef struct _IplConvKernel
{
    int  nCols;
    int  nRows;
    int  anchorX;
    int  anchorY;
    int *values;
    int  nShiftR;
}
IplConvKernel;


#define IPL_IMAGE_HEADER 1
#define IPL_IMAGE_DATA   2
#define IPL_IMAGE_ROI    4

#endif/*HAVE_IPL*/

/* extra border mode */
#define IPL_BORDER_REFLECT_101    4

#define IPL_IMAGE_MAGIC_VAL  ((int)sizeof(IplImage))
#define VOS_TYPE_NAME_IMAGE " -image"

#define VOS_IS_IMAGE_HDR(img) \
    ((img) != NULL && ((const IplImage*)(img))->nSize == sizeof(IplImage))

#define VOS_IS_IMAGE(img) \
    (VOS_IS_IMAGE_HDR(img) && ((IplImage*)img)->imageData != NULL)

/* for storing double-precision
   floating point data in IplImage's */
#define IPL_DEPTH_64F  64

/* get reference to pixel at (col,row),
   for multi-channel images (col) should be multiplied by number of channels */
#define VOS_IMAGE_ELEM( image, elemtype, row, col )       \
    (((elemtype*)((image)->imageData + (image)->widthStep*(row)))[(col)])

/****************************************************************************************\
*                                  Matrix type (Mat)                                   *
\****************************************************************************************/

#define VOS_CN_MAX     64
#define VOS_CN_SHIFT   3
#define VOS_DEPTH_MAX  (1 << VOS_CN_SHIFT)

#define VOS_8U   0
#define VOS_8S   1
#define VOS_16U  2
#define VOS_16S  3
#define VOS_32S  4
#define VOS_32F  5
#define VOS_64F  6
#define VOS_USRTYPE1 7

#define VOS_MAKETYPE(depth,cn) ((depth) + (((cn)-1) << VOS_CN_SHIFT))
#define VOS_MAKE_TYPE VOS_MAKETYPE

#define VOS_8UC1 VOS_MAKETYPE(VOS_8U,1)
#define VOS_8UC2 VOS_MAKETYPE(VOS_8U,2)
#define VOS_8UC3 VOS_MAKETYPE(VOS_8U,3)
#define VOS_8UC4 VOS_MAKETYPE(VOS_8U,4)
#define VOS_8UC(n) VOS_MAKETYPE(VOS_8U,(n))

#define VOS_8SC1 VOS_MAKETYPE(VOS_8S,1)
#define VOS_8SC2 VOS_MAKETYPE(VOS_8S,2)
#define VOS_8SC3 VOS_MAKETYPE(VOS_8S,3)
#define VOS_8SC4 VOS_MAKETYPE(VOS_8S,4)
#define VOS_8SC(n) VOS_MAKETYPE(VOS_8S,(n))

#define VOS_16UC1 VOS_MAKETYPE(VOS_16U,1)
#define VOS_16UC2 VOS_MAKETYPE(VOS_16U,2)
#define VOS_16UC3 VOS_MAKETYPE(VOS_16U,3)
#define VOS_16UC4 VOS_MAKETYPE(VOS_16U,4)
#define VOS_16UC(n) VOS_MAKETYPE(VOS_16U,(n))

#define VOS_16SC1 VOS_MAKETYPE(VOS_16S,1)
#define VOS_16SC2 VOS_MAKETYPE(VOS_16S,2)
#define VOS_16SC3 VOS_MAKETYPE(VOS_16S,3)

#define VOS_32SC1 VOS_MAKETYPE(VOS_32S,1)
#define VOS_32SC2 VOS_MAKETYPE(VOS_32S,2)
#define VOS_32SC3 VOS_MAKETYPE(VOS_32S,3)

#define VOS_32FC1 VOS_MAKETYPE(VOS_32F,1)
#define VOS_32FC2 VOS_MAKETYPE(VOS_32F,2)
#define VOS_32FC3 VOS_MAKETYPE(VOS_32F,3)
#define VOS_32FC(n) VOS_MAKETYPE(VOS_32F,(n))

#define VOS_64FC1 VOS_MAKETYPE(VOS_64F,1)
#define VOS_64FC2 VOS_MAKETYPE(VOS_64F,2)
#define VOS_64FC3 VOS_MAKETYPE(VOS_64F,3)
#define VOS_64FC4 VOS_MAKETYPE(VOS_64F,4)
#define VOS_64FC(n) VOS_MAKETYPE(VOS_64F,(n))

#define VOS_AUTO_STEP  0x7fffffff

#define VOS_MAT_CN_MASK          ((VOS_CN_MAX - 1) << VOS_CN_SHIFT)
#define VOS_MAT_CN(flags)        ((((flags) & VOS_MAT_CN_MASK) >> VOS_CN_SHIFT) + 1)
#define VOS_MAT_DEPTH_MASK       (VOS_DEPTH_MAX - 1)
#define VOS_MAT_DEPTH(flags)     ((flags) & VOS_MAT_DEPTH_MASK)
#define VOS_MAT_TYPE_MASK        (VOS_DEPTH_MAX*VOS_CN_MAX - 1)
#define VOS_MAT_TYPE(flags)      ((flags) & VOS_MAT_TYPE_MASK)
#define VOS_MAT_CONT_FLAG_SHIFT  14
#define VOS_MAT_CONT_FLAG        (1 << VOS_MAT_CONT_FLAG_SHIFT)
#define VOS_IS_MAT_CONT(flags)   ((flags) & VOS_MAT_CONT_FLAG)
#define VOS_IS_CONT_MAT          VOS_IS_MAT_CONT
#define VOS_MAT_TEMP_FLAG_SHIFT  15
#define VOS_MAT_TEMP_FLAG        (1 << VOS_MAT_TEMP_FLAG_SHIFT)
#define VOS_IS_TEMP_MAT(flags)   ((flags) & VOS_MAT_TEMP_FLAG)

#define VOS_MAGIC_MASK       0xFFFF0000
#define VOS_MAT_MAGIC_VAL    0x42420000
#define VOS_TYPE_NAME_MAT    " -matrix"

typedef struct Mat
{
    int type;
    int step;

    /* for internal use only */
    int* refcount;
    int hdr_refcount;

    union
    {
        uchar* ptr;
        short* s;
        int* i;
        float* fl;
        double* db;
    } data;

#ifdef __cplusplus
    union
    {
        int rows;
        int height;
    };

    union
    {
        int cols;
        int width;
    };
#else
    int rows;
    int cols;
#endif

}
Mat;


#define VOS_IS_MAT_HDR(mat) \
    ((mat) != NULL && \
    (((const Mat*)(mat))->type & VOS_MAGIC_MASK) == VOS_MAT_MAGIC_VAL && \
    ((const Mat*)(mat))->cols > 0 && ((const Mat*)(mat))->rows > 0)

#define VOS_IS_MAT(mat) \
    (VOS_IS_MAT_HDR(mat) && ((const Mat*)(mat))->data.ptr != NULL)

#define VOS_IS_MASK_ARR(mat) \
    (((mat)->type & (VOS_MAT_TYPE_MASK & ~VOS_8SC1)) == 0)

#define VOS_ARE_TYPES_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & VOS_MAT_TYPE_MASK) == 0)

#define VOS_ARE_CNS_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & VOS_MAT_CN_MASK) == 0)

#define VOS_ARE_DEPTHS_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & VOS_MAT_DEPTH_MASK) == 0)

#define VOS_ARE_SIZES_EQ(mat1, mat2) \
    ((mat1)->height == (mat2)->height && (mat1)->width == (mat2)->width)


/* size of each channel item,
   0x124489 = 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define VOS_ELEM_SIZE1(type) \
    ((((sizeof(size_t)<<28)|0x8442211) >> VOS_MAT_DEPTH(type)*4) & 15)

/* 0x3a50 = 11 10 10 01 01 00 00 ~ array of log2(sizeof(arr_type_elem)) */
#define VOS_ELEM_SIZE(type) \
    (VOS_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> VOS_MAT_DEPTH(type)*2) & 3))

/* inline constructor. No data is allocated internally!!!
   (use together with CreateData, or use CreateMat instead to
   get a matrix with allocated data) */
VOS_INLINE Mat InitMat( int rows, int cols, int type, void* data VOS_DEFAULT(NULL))
{
    Mat m;

    assert( (unsigned)VOS_MAT_DEPTH(type) <= VOS_64F );
    type = VOS_MAT_TYPE(type);
    m.type = VOS_MAT_MAGIC_VAL | VOS_MAT_CONT_FLAG | type;
    m.cols = cols;
    m.rows = rows;
    m.step = rows > 1 ? m.cols*VOS_ELEM_SIZE(type) : 0;
    m.data.ptr = (uchar*)data;
    m.refcount = NULL;
    m.hdr_refcount = 0;

    return m;
}


VOS_INLINE int CvToIplDepth( int type )
{
    int depth = VOS_MAT_DEPTH(type);
    return VOS_ELEM_SIZE1(depth)*8 | (depth == VOS_8S || depth == VOS_16S ||
                                      depth == VOS_32S ? IPL_DEPTH_SIGN : 0);
}



struct Set;

/*************************************** Rect *****************************************/

typedef struct Rect
{
    int x;
    int y;
    int width;
    int height;
}
Rect;

VOS_INLINE  Rect  initRect( int x, int y, int width, int height )
{
    Rect r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}


/******************************* Point and variants ***********************************/

typedef struct Point
{
    int x;
    int y;
}
Point;


VOS_INLINE  Point  InitPoint( int x, int y )
{
    Point p;

    p.x = x;
    p.y = y;

    return p;
}


typedef struct Point2D32f
{
    float x;
    float y;
}
Point2D32f;


VOS_INLINE  Point2D32f  initPoint2D32f( double x, double y )
{
    Point2D32f p;

    p.x = (float)x;
    p.y = (float)y;

    return p;
}


VOS_INLINE  Point2D32f  PointTo32f( Point point )
{
    return initPoint2D32f( (float)point.x, (float)point.y );
}


VOS_INLINE  Point  PointFrom32f( Point2D32f point )
{
    Point ipt;
    ipt.x = SysRound(point.x);
    ipt.y = SysRound(point.y);

    return ipt;
}




/******************************** Size's & CvBox **************************************/

typedef struct Size
{
    int width;
    int height;
}
Size;

VOS_INLINE  Size  GetSize( int width, int height )
{
    Size s;

    s.width = width;
    s.height = height;

    return s;
}

typedef struct Size2D32f
{
    float width;
    float height;
}
Size2D32f;


typedef struct Box2D
{
    Point2D32f center;  /* center of the box */
    Size2D32f  size;    /* box width and length */
    float angle;          /* angle between the horizontal axis
                             and the first side (i.e. length) in degrees */
}
Box2D;


/************************************* Slice ******************************************/

typedef struct Slice
{
    int  start_index, end_index;
}
Slice;

VOS_INLINE  Slice  GetSlice( int start, int end )
{
    Slice slice;
    slice.start_index = start;
    slice.end_index = end;

    return slice;
}

#define VOS_WHOLE_SEQ_END_INDEX 0x3fffffff
#define VOS_WHOLE_SEQ  GetSlice(0, VOS_WHOLE_SEQ_END_INDEX)


/************************************* Scalar *****************************************/

typedef struct Scalar
{
    double val[4];
}
Scalar;

VOS_INLINE  Scalar  GetScalar( double val0, double val1 VOS_DEFAULT(0),
                              double val2 VOS_DEFAULT(0), double val3 VOS_DEFAULT(0))
{
    Scalar scalar;
    scalar.val[0] = val0; scalar.val[1] = val1;
    scalar.val[2] = val2; scalar.val[3] = val3;
    return scalar;
}


VOS_INLINE  Scalar  RealScalar( double val0 )
{
    Scalar scalar;
    scalar.val[0] = val0;
    scalar.val[1] = scalar.val[2] = scalar.val[3] = 0;
    return scalar;
}

VOS_INLINE  Scalar  ScalarAll( double val0123 )
{
    Scalar scalar;
    scalar.val[0] = val0123;
    scalar.val[1] = val0123;
    scalar.val[2] = val0123;
    scalar.val[3] = val0123;
    return scalar;
}

/******************************** Memory storage ****************************************/

typedef struct MemBlock
{
    struct MemBlock*  prev;
    struct MemBlock*  next;
}
MemBlock;

#define VOS_STORAGE_MAGIC_VAL    0x42890000

typedef struct MemStorage
{
    int signature;
    MemBlock* bottom;/* first allocated block */
    MemBlock* top;   /* current memory block - top of the stack */
    struct  MemStorage* parent; /* borrows new blocks from */
    int block_size;  /* block size */
    int free_space;  /* free space in the current block */
}
MemStorage;

#define VOS_IS_STORAGE(storage)  \
    ((storage) != NULL &&       \
    (((MemStorage*)(storage))->signature & VOS_MAGIC_MASK) == VOS_STORAGE_MAGIC_VAL)


typedef struct MemStoragePos
{
    MemBlock* top;
    int free_space;
}
MemStoragePos;


/*********************************** Sequence *******************************************/

typedef struct SeqBlock
{
    struct SeqBlock*  prev; /* previous sequence block */
    struct SeqBlock*  next; /* next sequence block */
    int    start_index;       /* index of the first element in the block +
                                 sequence->first->start_index */
    int    count;             /* number of elements in the block */
    char*  data;              /* pointer to the first element of the block */
}
SeqBlock;


#define VOS_TREE_NODE_FIELDS(node_type)                          \
    int       flags;         /* micsellaneous flags */          \
    int       header_size;   /* size of sequence header */      \
    struct    node_type* h_prev; /* previous sequence */        \
    struct    node_type* h_next; /* next sequence */            \
    struct    node_type* v_prev; /* 2nd previous sequence */    \
    struct    node_type* v_next  /* 2nd next sequence */

/*
   Read/Write sequence.
   Elements can be dynamically inserted to or deleted from the sequence.
*/
#define VOS_SEQUENCE_FIELDS()                                            \
    VOS_TREE_NODE_FIELDS(Seq);                                         \
    int       total;          /* total number of elements */            \
    int       elem_size;      /* size of sequence element in bytes */   \
    char*     block_max;      /* maximal bound of the last block */     \
    char*     ptr;            /* current write pointer */               \
    int       delta_elems;    /* how many elements allocated when the seq grows */  \
    MemStorage* storage;    /* where the seq is stored */             \
    SeqBlock* free_blocks;  /* free blocks list */                    \
    SeqBlock* first; /* pointer to the first sequence block */

typedef struct Seq
{
    VOS_SEQUENCE_FIELDS()
}
Seq;

#define VOS_TYPE_NAME_SEQ             " -sequence"
#define VOS_TYPE_NAME_SEQ_TREE        " -sequence-tree"

/*************************************** Set ********************************************/
/*
  Set.
  Order is not preserved. There can be gaps between sequence elements.
  After the element has been inserted it stays in the same place all the time.
  The MSB(most-significant or sign bit) of the first field (flags) is 0 iff the element exists.
*/
#define VOS_SET_ELEM_FIELDS(elem_type)   \
    int  flags;                         \
    struct elem_type* next_free;

typedef struct SetElem
{
    VOS_SET_ELEM_FIELDS(SetElem)
}
SetElem;

#define VOS_SET_FIELDS()      \
    VOS_SEQUENCE_FIELDS()     \
    SetElem* free_elems;   \
    int active_count;

typedef struct Set
{
    VOS_SET_FIELDS()
}
Set;


#define VOS_SET_ELEM_IDX_MASK   ((1 << 26) - 1)
#define VOS_SET_ELEM_FREE_FLAG  (1 << (sizeof(int)*8-1))

/* Checks whether the element pointed by ptr belongs to a set or not */
#define VOS_IS_SET_ELEM( ptr )  (((SetElem*)(ptr))->flags >= 0)

/*********************************** Chain/Countour *************************************/

#define VOS_CONTOUR_FIELDS()  \
    VOS_SEQUENCE_FIELDS()     \
    Rect rect;             \
    int color;               \
    int reserved[3];

typedef struct CvContour
{
    VOS_CONTOUR_FIELDS()
}
CvContour;

/****************************************************************************************\
*                                    Sequence types                                      *
\****************************************************************************************/

#define VOS_SEQ_MAGIC_VAL             0x42990000

#define VOS_IS_SEQ(seq) \
    ((seq) != NULL && (((Seq*)(seq))->flags & VOS_MAGIC_MASK) == VOS_SEQ_MAGIC_VAL)

#define VOS_SET_MAGIC_VAL             0x42980000
#define VOS_IS_SET(set) \
    ((set) != NULL && (((Seq*)(set))->flags & VOS_MAGIC_MASK) == VOS_SET_MAGIC_VAL)

#define VOS_SEQ_ELTYPE_BITS           9
#define VOS_SEQ_ELTYPE_MASK           ((1 << VOS_SEQ_ELTYPE_BITS) - 1)

#define VOS_SEQ_ELTYPE_POINT          VOS_32SC2  /* (x,y) */
#define VOS_SEQ_ELTYPE_CODE           VOS_8UC1   /* freeman code: 0..7 */
#define VOS_SEQ_ELTYPE_GENERIC        0
#define VOS_SEQ_ELTYPE_PTR            VOS_USRTYPE1
#define VOS_SEQ_ELTYPE_PPOINT         VOS_SEQ_ELTYPE_PTR  /* &(x,y) */
#define VOS_SEQ_ELTYPE_INDEX          VOS_32SC1  /* #(x,y) */
#define VOS_SEQ_ELTYPE_GRAPH_EDGE     0  /* &next_o, &next_d, &vtx_o, &vtx_d */
#define VOS_SEQ_ELTYPE_GRAPH_VERTEX   0  /* first_edge, &(x,y) */
#define VOS_SEQ_ELTYPE_TRIAN_ATR      0  /* vertex of the binary tree   */
#define VOS_SEQ_ELTYPE_CONNECTED_COMP 0  /* connected component  */
#define VOS_SEQ_ELTYPE_POINT3D        VOS_32FC3  /* (x,y,z)  */

#define VOS_SEQ_KIND_BITS        3
#define VOS_SEQ_KIND_MASK        (((1 << VOS_SEQ_KIND_BITS) - 1)<<VOS_SEQ_ELTYPE_BITS)

/* types of sequences */
#define VOS_SEQ_KIND_GENERIC     (0 << VOS_SEQ_ELTYPE_BITS)
#define VOS_SEQ_KIND_CURVE       (1 << VOS_SEQ_ELTYPE_BITS)
#define VOS_SEQ_KIND_BIN_TREE    (2 << VOS_SEQ_ELTYPE_BITS)

/* types of sparse sequences (sets) */
#define VOS_SEQ_KIND_GRAPH       (3 << VOS_SEQ_ELTYPE_BITS)
#define VOS_SEQ_KIND_SUBDIV2D    (4 << VOS_SEQ_ELTYPE_BITS)

#define VOS_SEQ_FLAG_SHIFT       (VOS_SEQ_KIND_BITS + VOS_SEQ_ELTYPE_BITS)

/* flags for curves */
#define VOS_SEQ_FLAG_CLOSED     (1 << VOS_SEQ_FLAG_SHIFT)
#define VOS_SEQ_FLAG_SIMPLE     (2 << VOS_SEQ_FLAG_SHIFT)
#define VOS_SEQ_FLAG_CONVEX     (4 << VOS_SEQ_FLAG_SHIFT)
#define VOS_SEQ_FLAG_HOLE       (8 << VOS_SEQ_FLAG_SHIFT)

/* point sets */
#define VOS_SEQ_POINT_SET       (VOS_SEQ_KIND_GENERIC| VOS_SEQ_ELTYPE_POINT)
#define VOS_SEQ_POINT3D_SET     (VOS_SEQ_KIND_GENERIC| VOS_SEQ_ELTYPE_POINT3D)
#define VOS_SEQ_POLYLINE        (VOS_SEQ_KIND_CURVE  | VOS_SEQ_ELTYPE_POINT)
#define VOS_SEQ_POLYGON         (VOS_SEQ_FLAG_CLOSED | VOS_SEQ_POLYLINE )
#define VOS_SEQ_CONTOUR         VOS_SEQ_POLYGON
#define VOS_SEQ_SIMPLE_POLYGON  (VOS_SEQ_FLAG_SIMPLE | VOS_SEQ_POLYGON  )

/* chain-coded curves */
#define VOS_SEQ_CHAIN           (VOS_SEQ_KIND_CURVE  | VOS_SEQ_ELTYPE_CODE)
#define VOS_SEQ_CHAIN_CONTOUR   (VOS_SEQ_FLAG_CLOSED | VOS_SEQ_CHAIN)

/* binary tree for the contour */
#define VOS_SEQ_POLYGON_TREE    (VOS_SEQ_KIND_BIN_TREE  | VOS_SEQ_ELTYPE_TRIAN_ATR)

/* sequence of the connected components */
#define VOS_SEQ_CONNECTED_COMP  (VOS_SEQ_KIND_GENERIC  | VOS_SEQ_ELTYPE_CONNECTED_COMP)

/* sequence of the integer numbers */
#define VOS_SEQ_INDEX           (VOS_SEQ_KIND_GENERIC  | VOS_SEQ_ELTYPE_INDEX)

#define VOS_SEQ_ELTYPE( seq )   ((seq)->flags & VOS_SEQ_ELTYPE_MASK)
#define VOS_SEQ_KIND( seq )     ((seq)->flags & VOS_SEQ_KIND_MASK )

/* flag checking */
#define VOS_IS_SEQ_INDEX( seq )      ((VOS_SEQ_ELTYPE(seq) == VOS_SEQ_ELTYPE_INDEX) && \
    (VOS_SEQ_KIND(seq) == VOS_SEQ_KIND_GENERIC))

#define VOS_IS_SEQ_CURVE( seq )      (VOS_SEQ_KIND(seq) == VOS_SEQ_KIND_CURVE)
#define VOS_IS_SEQ_CLOSED( seq )     (((seq)->flags & VOS_SEQ_FLAG_CLOSED) != 0)
#define VOS_IS_SEQ_CONVEX( seq )     (((seq)->flags & VOS_SEQ_FLAG_CONVEX) != 0)
#define VOS_IS_SEQ_HOLE( seq )       (((seq)->flags & VOS_SEQ_FLAG_HOLE) != 0)


/* type checking macros */
#define VOS_IS_SEQ_POINT_SET( seq ) \
    ((VOS_SEQ_ELTYPE(seq) == VOS_32SC2 || VOS_SEQ_ELTYPE(seq) == VOS_32FC2))


#define VOS_IS_SEQ_POLYLINE( seq )   \
    (VOS_SEQ_KIND(seq) == VOS_SEQ_KIND_CURVE && VOS_IS_SEQ_POINT_SET(seq))

#define VOS_IS_SEQ_POLYGON( seq )   \
    (VOS_IS_SEQ_POLYLINE(seq) && VOS_IS_SEQ_CLOSED(seq))

#define VOS_IS_SEQ_CHAIN( seq )   \
    (VOS_SEQ_KIND(seq) == VOS_SEQ_KIND_CURVE && (seq)->elem_size == 1)


/****************************************************************************************/
/*                            Sequence writer & reader                                  */
/****************************************************************************************/

#define VOS_SEQ_WRITER_FIELDS()                                     \
    int          header_size;                                      \
    Seq*       seq;        /* the sequence written */            \
    SeqBlock*  block;      /* current block */                   \
    char*        ptr;        /* pointer to free space */           \
    char*        block_min;  /* pointer to the beginning of block*/\
    char*        block_max;  /* pointer to the end of block */

typedef struct SeqWriter
{
    VOS_SEQ_WRITER_FIELDS()
}
SeqWriter;


#define VOS_SEQ_READER_FIELDS()                                      \
    int          header_size;                                       \
    Seq*       seq;        /* sequence, beign read */             \
    SeqBlock*  block;      /* current block */                    \
    char*        ptr;        /* pointer to element be read next */  \
    char*        block_min;  /* pointer to the beginning of block */\
    char*        block_max;  /* pointer to the end of block */      \
    int          delta_index;/* = seq->first->start_index   */      \
    char*        prev_elem;  /* pointer to previous element */


typedef struct SeqReader
{
    VOS_SEQ_READER_FIELDS()
}
SeqReader;

/****************************************************************************************/
/*                                Operations on sequences                               */
/****************************************************************************************/


#define VOS_WRITE_SEQ_ELEM( elem, writer )             \
{                                                     \
    assert( (writer).seq->elem_size == sizeof(elem)); \
    if( (writer).ptr >= (writer).block_max )          \
{                                                 \
    CreateSeqBlock( &writer);                   \
    }                                                 \
    assert( (writer).ptr <= (writer).block_max - sizeof(elem));\
    memcpy((writer).ptr, &(elem), sizeof(elem));      \
    (writer).ptr += sizeof(elem);                     \
    }


/* move reader position forward */
#define VOS_NEXT_SEQ_ELEM( elem_size, reader )                 \
{                                                             \
    if( ((reader).ptr += (elem_size)) >= (reader).block_max ) \
{                                                         \
    ChangeSeqBlock( &(reader), 1 );                     \
    }                                                         \
    }


/* read element and move read position forward */
#define VOS_READ_SEQ_ELEM( elem, reader )                       \
{                                                              \
    assert( (reader).seq->elem_size == sizeof(elem));          \
    memcpy( &(elem), (reader).ptr, sizeof((elem)));            \
    VOS_NEXT_SEQ_ELEM( sizeof(elem), reader )                   \
    }


#endif /*_CXCORE_TYPES_H_*/

/* End of file. */


#ifndef _CXCORE_MISC_H_
#define _CXCORE_MISC_H_
#include "cxtypes.h"
#include <limits.h>


/****************************************************************************************\
*                              Compile-time tuning parameters                            *
\****************************************************************************************/

/* maximal size of vector to run matrix operations on it inline (i.e. w/o ipp calls) */
#define  VOS_MAX_INLINE_MAT_OP_SIZE  10

/* maximal linear size of matrix to allocate it on stack. */
#define  VOS_MAX_LOCAL_MAT_SIZE  32

/* maximal size of local memory storage */
#define  VOS_MAX_LOCAL_SIZE  \
    (VOS_MAX_LOCAL_MAT_SIZE*VOS_MAX_LOCAL_MAT_SIZE*(int)sizeof(double))

/* default image row align (in bytes) */
#define  VOS_DEFAULT_IMAGE_ROW_ALIGN  4

/* matrices are continuous by default */
#define  VOS_DEFAULT_MAT_ROW_ALIGN  1

/* maximum size of dynamic memory buffer.
   SysAlloc reports an error if a larger block is requested. */
#define  VOS_MAX_ALLOC_SIZE    (((size_t)1 << (sizeof(size_t)*8-2)))

/* the alignment of all the allocated buffers */
#define  VOS_MALLOC_ALIGN    32

/* default alignment for dynamic data strucutures, resided in storages. */
#define  VOS_STRUCT_ALIGN    ((int)sizeof(double))

/* default storage block size */
#define  VOS_STORAGE_BLOCK_SIZE   ((1<<16) - 128)

/* default memory block for sparse array elements */
#define  VOS_SPARSE_MAT_BLOCK    (1<<12)

/* initial hash table size */
#define  VOS_SPARSE_HASH_SIZE0    (1<<10)

/* maximal average node_count/hash_size ratio beyond which hash table is resized */
#define  VOS_SPARSE_HASH_RATIO    3

/* max length of strings */
#define  VOS_MAX_STRLEN  1024


/****************************************************************************************\
*                                  Common declarations                                   *
\****************************************************************************************/

/* get alloca declaration */
#ifdef __GNUC__
    #undef alloca
    #define alloca __builtin_alloca
#elif defined WIN32 || defined WIN64
    #if defined _MSC_VER || defined __BORLANDC__
        #include <malloc.h>
    #endif
#elif
    #error
#endif




#define cvStackAlloc(size) AlignPtr( alloca((size) + VOS_MALLOC_ALIGN), VOS_MALLOC_ALIGN )

/* default step, set in case of continuous data
   to work around checks for valid step in some ipp functions */
#define  VOS_STUB_STEP     (1 << 30)


#define  VOS_ORIGIN_TL  0
#define  VOS_ORIGIN_BL  1

/* IEEE754 constants and macros */
#define  VOS_TOGGLE_FLT(x) ((x)^((int)(x) < 0 ? 0x7fffffff : 0))


#define  VOS_NOP(a)      (a)
#define  VOS_ADD(a, b)   ((a) + (b))
#define  VOS_SUB(a, b)   ((a) - (b))
#define  VOS_MUL(a, b)   ((a) * (b))
#define  VOS_AND(a, b)   ((a) & (b))
#define  VOS_OR(a, b)    ((a) | (b))
#define  VOS_XOR(a, b)   ((a) ^ (b))
#define  VOS_ANDN(a, b)  (~(a) & (b))
#define  VOS_ORN(a, b)   (~(a) | (b))
#define  VOS_SQR(a)      ((a) * (a))

#define  VOS_LT(a, b)    ((a) < (b))
#define  VOS_LE(a, b)    ((a) <= (b))
#define  VOS_EQ(a, b)    ((a) == (b))
#define  VOS_NE(a, b)    ((a) != (b))
#define  VOS_GT(a, b)    ((a) > (b))
#define  VOS_GE(a, b)    ((a) >= (b))


/* general-purpose saturation macros */
#define  VOS_CAST_8U(t)  (uchar)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#define  VOS_CAST_8S(t)  (char)(!(((t)+128) & ~255) ? (t) : (t) > 0 ? 127 : -128)
#define  VOS_CAST_16U(t) (ushort)(!((t) & ~65535) ? (t) : (t) > 0 ? 65535 : 0)
#define  VOS_CAST_16S(t) (short)(!(((t)+32768) & ~65535) ? (t) : (t) > 0 ? 32767 : -32768)

#define  cvUnsupportedFormat "Unsupported format"

VOS_INLINE void* AlignPtr( const void* ptr, int align=32 )
{
    assert( (align & (align-1)) == 0 );
    return (void*)( ((size_t)ptr + align - 1) & ~(size_t)(align-1) );
}

VOS_INLINE int Align( int size, int align )
{
    assert( (align & (align-1)) == 0 && size < INT_MAX );
    return (size + align - 1) & -align;
}

VOS_INLINE  Size  GetMatSize( const Mat* mat )
{
    Size size = { mat->width, mat->height };
    return size;
}

#define  VOS_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))


#define VOS_SYS_MEMCPY_INT( dst, src, len )                                              \
{                                                                                   \
    size_t _icv_SYS_MEMCPY_i_, _icv_SYS_MEMCPY_len_ = (len);                                \
    int* _icv_SYS_MEMCPY_dst_ = (int*)(dst);                                            \
    const int* _icv_SYS_MEMCPY_src_ = (const int*)(src);                                \
    assert( ((size_t)_icv_SYS_MEMCPY_src_&(sizeof(int)-1)) == 0 &&                      \
            ((size_t)_icv_SYS_MEMCPY_dst_&(sizeof(int)-1)) == 0 );                      \
                                                                                    \
    for(_icv_SYS_MEMCPY_i_=0;_icv_SYS_MEMCPY_i_<_icv_SYS_MEMCPY_len_;_icv_SYS_MEMCPY_i_++)          \
        _icv_SYS_MEMCPY_dst_[_icv_SYS_MEMCPY_i_] = _icv_SYS_MEMCPY_src_[_icv_SYS_MEMCPY_i_];        \
}


#define VOS_SYS_MEMCPY_AUTO( dst, src, len )                                             \
{                                                                                   \
    size_t _icv_SYS_MEMCPY_i_, _icv_SYS_MEMCPY_len_ = (len);                                \
    char* _icv_SYS_MEMCPY_dst_ = (char*)(dst);                                          \
    const char* _icv_SYS_MEMCPY_src_ = (const char*)(src);                              \
    if( (_icv_SYS_MEMCPY_len_ & (sizeof(int)-1)) == 0 )                                 \
    {                                                                               \
        assert( ((size_t)_icv_SYS_MEMCPY_src_&(sizeof(int)-1)) == 0 &&                  \
                ((size_t)_icv_SYS_MEMCPY_dst_&(sizeof(int)-1)) == 0 );                  \
        for( _icv_SYS_MEMCPY_i_ = 0; _icv_SYS_MEMCPY_i_ < _icv_SYS_MEMCPY_len_;                 \
            _icv_SYS_MEMCPY_i_+=sizeof(int) )                                           \
        {                                                                           \
            *(int*)(_icv_SYS_MEMCPY_dst_+_icv_SYS_MEMCPY_i_) =                              \
            *(const int*)(_icv_SYS_MEMCPY_src_+_icv_SYS_MEMCPY_i_);                         \
        }                                                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        for(_icv_SYS_MEMCPY_i_ = 0; _icv_SYS_MEMCPY_i_ < _icv_SYS_MEMCPY_len_; _icv_SYS_MEMCPY_i_++)\
            _icv_SYS_MEMCPY_dst_[_icv_SYS_MEMCPY_i_] = _icv_SYS_MEMCPY_src_[_icv_SYS_MEMCPY_i_];    \
    }                                                                               \
}


/****************************************************************************************\
*                     Structures and macros for integration with IPP                     *
\****************************************************************************************/

/* IPP-compatible return codes */
typedef enum CvStatus
{
    VOS_BADMEMBLOCK_ERR          = -113,
    VOS_INPLACE_NOT_SUPPORTED_ERR= -112,
    VOS_UNMATCHED_ROI_ERR        = -111,
    VOS_NOTFOUND_ERR             = -110,
    VOS_BADCONVERGENCE_ERR       = -109,

    VOS_BADDEPTH_ERR             = -107,
    VOS_BADROI_ERR               = -106,
    VOS_BADHEADER_ERR            = -105,
    VOS_UNMATCHED_FORMATS_ERR    = -104,
    VOS_UNSUPPORTED_COI_ERR      = -103,
    VOS_UNSUPPORTED_CHANNELS_ERR = -102,
    VOS_UNSUPPORTED_DEPTH_ERR    = -101,
    VOS_UNSUPPORTED_FORMAT_ERR   = -100,

    VOS_BADARG_ERR      = -49,  //
    VOS_NOTDEFINED_ERR  = -48,  //

    VOS_BADCHANNELS_ERR = -47,  //
    VOS_BADRANGE_ERR    = -44,  //
    VOS_BADSTEP_ERR     = -29,  //

    VOS_BADFLAG_ERR     =  -12,
    VOS_DIV_BY_ZERO_ERR =  -11, //
    VOS_BADCOEF_ERR     =  -10,

    VOS_BADFACTOR_ERR   =  -7,
    VOS_BADPOINT_ERR    =  -6,
    VOS_BADSCALE_ERR    =  -4,
    VOS_OUTOFMEM_ERR    =  -3,
    VOS_NULLPTR_ERR     =  -2,
    VOS_BADSIZE_ERR     =  -1,
    VOS_NO_ERR          =   0,
    VOS_OK              =   VOS_NO_ERR
}
CvStatus;

#define VOS_ERROR_FROM_STATUS( result )                \
    VOS_ERROR( SysErrorFromStatus( result ), "function failed" )

#define VOS_FUN_CALL( Func )                                              \
{                                                                      \
      CvStatus  fun_call_result;                                      \
      fun_call_result = Func;                                         \
                                                                       \
      if( fun_call_result < 0 )                                       \
            VOS_ERROR_FROM_STATUS( (fun_call_result));                 \
}

#endif /*_CXCORE_MISC_H_*/

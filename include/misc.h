
#ifndef _CXCORE_MISC_H_
#define _CXCORE_MISC_H_
#include "types.h"
#include <limits.h>

#define  VOS_MAX_INLINE_MAT_OP_SIZE  10

#define  VOS_MAX_LOCAL_MAT_SIZE  32

#define  VOS_MAX_LOCAL_SIZE  \
    (VOS_MAX_LOCAL_MAT_SIZE*VOS_MAX_LOCAL_MAT_SIZE*(int)sizeof(double))
#define  VOS_DEFAULT_IMAGE_ROW_ALIGN  4
#define  VOS_DEFAULT_MAT_ROW_ALIGN  1

#define  VOS_MAX_ALLOC_SIZE    (((size_t)1 << (sizeof(size_t)*8-2)))

#define  VOS_MALLOC_ALIGN    32
#define  VOS_STRUCT_ALIGN    ((int)sizeof(double))
#define  VOS_STORAGE_BLOCK_SIZE   ((1<<16) - 128)

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


#define sysStackAlloc(size) AlignPtr( alloca((size) + VOS_MALLOC_ALIGN), VOS_MALLOC_ALIGN )

#define  VOS_STUB_STEP     (1 << 30)


#define  VOS_ORIGIN_TL  0
#define  VOS_ORIGIN_BL  1

#define  VOS_NOP(a)      (a)


/* general-purpose saturation macros */
#define  VOS_CAST_8U(t)  (uchar)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#define  VOS_CAST_8S(t)  (char)(!(((t)+128) & ~255) ? (t) : (t) > 0 ? 127 : -128)
#define  VOS_CAST_16U(t) (ushort)(!((t) & ~65535) ? (t) : (t) > 0 ? 65535 : 0)
#define  VOS_CAST_16S(t) (short)(!(((t)+32768) & ~65535) ? (t) : (t) > 0 ? 32767 : -32768)

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



typedef enum VosStatus
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
      VosStatus  fun_call_result;                                      \
      fun_call_result = Func;                                         \
                                                                       \
      if( fun_call_result < 0 )                                       \
            VOS_ERROR_FROM_STATUS( (fun_call_result));                 \
}

#endif /*_CXCORE_MISC_H_*/

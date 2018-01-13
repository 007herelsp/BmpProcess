
#ifndef _CXCORE_MISC_H_
#define _CXCORE_MISC_H_
#include "types.h"
#include <limits.h>

#define VOS_MAX_INLINE_MAT_OP_SIZE 10

#define VOS_MAX_LOCAL_MAT_SIZE 32

#define VOS_MAX_LOCAL_SIZE \
    (VOS_MAX_LOCAL_MAT_SIZE * VOS_MAX_LOCAL_MAT_SIZE * (int)sizeof(double))
#define VOS_DEFAULT_IMAGE_ROW_ALIGN 4
#define VOS_DEFAULT_MAT_ROW_ALIGN 1

#define VOS_MAX_ALLOC_SIZE (((size_t)1 << (sizeof(size_t) * 8 - 2)))

#define VOS_MALLOC_ALIGN 32
#define VOS_STRUCT_ALIGN ((int)sizeof(double))
#define VOS_STORAGE_BLOCK_SIZE ((1 << 16) - 128)

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

#define sysStackAlloc(size) AlignPtr(alloca((size) + VOS_MALLOC_ALIGN), VOS_MALLOC_ALIGN)

#define VOS_STUB_STEP (1 << 30)

#define VOS_ORIGIN_TL 0
#define VOS_ORIGIN_BL 1

#define VOS_NOP(a) (a)

#define VOS_CAST_8U(t) (uchar)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#define VOS_CAST_8S(t) (char)(!(((t) + 128) & ~255) ? (t) : (t) > 0 ? 127 : -128)
#define VOS_CAST_16U(t) (ushort)(!((t) & ~65535) ? (t) : (t) > 0 ? 65535 : 0)
#define VOS_CAST_16S(t) (short)(!(((t) + 32768) & ~65535) ? (t) : (t) > 0 ? 32767 : -32768)

VOS_INLINE void *AlignPtr(const void *ptr, int align = 32)
{
    assert((align & (align - 1)) == 0);
    return (void *)(((size_t)ptr + align - 1) & ~(size_t)(align - 1));
}

VOS_INLINE int Align(int size, int align)
{
    assert((align & (align - 1)) == 0 && size < INT_MAX);
    return (size + align - 1) & -align;
}

VOS_INLINE Size GetMatSize(const Mat *mat)
{
    Size size = {mat->width, mat->height};
    return size;
}

#define VOS_DESCALE(x, n) (((x) + (1 << ((n)-1))) >> (n))

#define VOS_SYS_MEMCPY_AUTO(dst, src, len)                                                        \
    \
{                                                                                            \
        size_t _i_SYS_MEMCPY_i_, _i_SYS_MEMCPY_len_ = (len);                                      \
        char *_i_SYS_MEMCPY_dst_ = (char *)(dst);                                                 \
        const char *_i_SYS_MEMCPY_src_ = (const char *)(src);                                     \
        if ((_i_SYS_MEMCPY_len_ & (sizeof(int) - 1)) == 0)                                        \
        {                                                                                         \
            assert(((size_t)_i_SYS_MEMCPY_src_ & (sizeof(int) - 1)) == 0 &&                       \
                   ((size_t)_i_SYS_MEMCPY_dst_ & (sizeof(int) - 1)) == 0);                        \
            for (_i_SYS_MEMCPY_i_ = 0; _i_SYS_MEMCPY_i_ < _i_SYS_MEMCPY_len_;                     \
                 _i_SYS_MEMCPY_i_ += sizeof(int))                                                 \
            {                                                                                     \
                *(int *)(_i_SYS_MEMCPY_dst_ + _i_SYS_MEMCPY_i_) =                                 \
                    *(const int *)(_i_SYS_MEMCPY_src_ + _i_SYS_MEMCPY_i_);                        \
            }                                                                                     \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            for (_i_SYS_MEMCPY_i_ = 0; _i_SYS_MEMCPY_i_ < _i_SYS_MEMCPY_len_; _i_SYS_MEMCPY_i_++) \
                _i_SYS_MEMCPY_dst_[_i_SYS_MEMCPY_i_] = _i_SYS_MEMCPY_src_[_i_SYS_MEMCPY_i_];      \
        }                                                                                         \
    \
}


#endif /*_CXCORE_MISC_H_*/

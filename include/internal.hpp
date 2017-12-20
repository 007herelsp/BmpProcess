
#ifndef __OPENCV_CORE_INTERNAL_HPP__
#define __OPENCV_CORE_INTERNAL_HPP__

#include "core.hpp"
#include "types_c.h"

#include <vector>

#ifndef FALSE
#  define FALSE 0
#endif
#ifndef TRUE
#  define TRUE 1
#endif



/* default image row align (in bytes) */
#define  CV_DEFAULT_IMAGE_ROW_ALIGN  4

/* matrices are continuous by default */

/* maximum size of dynamic memory buffer.
   cvAlloc reports an error if a larger block is requested. */
#define  CV_MAX_ALLOC_SIZE    (((size_t)1 << (sizeof(size_t)*8-2)))

/* the alignment of all the allocated buffers */
#define  CV_MALLOC_ALIGN    16

/* default alignment for dynamic data strucutures, resided in storages. */
#define  CV_STRUCT_ALIGN    ((int)sizeof(double))

/* default storage block size */
#define  CV_STORAGE_BLOCK_SIZE   ((1<<16) - 128)

#ifdef __GNUC__
#  define CV_DECL_ALIGNED(x) __attribute__ ((aligned (x)))
#elif defined _MSC_VER
#  define CV_DECL_ALIGNED(x) __declspec(align(x))
#else
#  define CV_DECL_ALIGNED(x)
#endif

#ifndef CV_IMPL
#  define CV_IMPL CV_EXTERN_C
#endif


/* default step, set in case of continuous data
   to work around checks for valid step in some ipp functions */


#define  CV_ORIGIN_TL  0
#define  CV_ORIGIN_BL  1

/* IEEE754 constants and macros */
#define  CV_TOGGLE_FLT(x) ((x)^((int)(x) < 0 ? 0x7fffffff : 0))



/* general-purpose saturation macros */
#define  CV_CAST_32F(t) (float)(t)

CV_INLINE void* cvAlignPtr( const void* ptr, int align CV_DEFAULT(32) )
{
    assert( (align & (align-1)) == 0 );
    return (void*)( ((size_t)ptr + align - 1) & ~(size_t)(align-1) );
}

CV_INLINE int cvAlign( int size, int align )
{
    assert( (align & (align-1)) == 0 && size < INT_MAX );
    return (size + align - 1) & -align;
}

CV_INLINE  CvSize  cvGetMatSize( const CvMat* mat )
{
    CvSize size;
    size.width = mat->cols;
    size.height = mat->rows;
    return size;
}

#define  CV_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))

#define CV_IMPLEMENT_QSORT_EX( func_name, T, LT, user_data_type )                   \
void func_name( T *array, size_t total, user_data_type aux )                        \
{                                                                                   \
    int isort_thresh = 7;                                                           \
    T t;                                                                            \
    int sp = 0;                                                                     \
                                                                                    \
    struct                                                                          \
    {                                                                               \
        T *lb;                                                                      \
        T *ub;                                                                      \
    }                                                                               \
    stack[48];                                                                      \
                                                                                    \
    (void)aux;                                                                      \
                                                                                    \
    if( total <= 1 )                                                                \
        return;                                                                     \
                                                                                    \
    stack[0].lb = array;                                                            \
    stack[0].ub = array + (total - 1);                                              \
                                                                                    \
    while( sp >= 0 )                                                                \
    {                                                                               \
        T* left = stack[sp].lb;                                                     \
        T* right = stack[sp--].ub;                                                  \
                                                                                    \
        for(;;)                                                                     \
        {                                                                           \
            int i, n = (int)(right - left) + 1, m;                                  \
            T* ptr;                                                                 \
            T* ptr2;                                                                \
                                                                                    \
            if( n <= isort_thresh )                                                 \
            {                                                                       \
            insert_sort:                                                            \
                for( ptr = left + 1; ptr <= right; ptr++ )                          \
                {                                                                   \
                    for( ptr2 = ptr; ptr2 > left && LT(ptr2[0],ptr2[-1]); ptr2--)   \
                        CV_SWAP( ptr2[0], ptr2[-1], t );                            \
                }                                                                   \
                break;                                                              \
            }                                                                       \
            else                                                                    \
            {                                                                       \
                T* left0;                                                           \
                T* left1;                                                           \
                T* right0;                                                          \
                T* right1;                                                          \
                T* pivot;                                                           \
                T* a;                                                               \
                T* b;                                                               \
                T* c;                                                               \
                int swap_cnt = 0;                                                   \
                                                                                    \
                left0 = left;                                                       \
                right0 = right;                                                     \
                pivot = left + (n/2);                                               \
                                                                                    \
                if( n > 40 )                                                        \
                {                                                                   \
                    int d = n / 8;                                                  \
                    a = left, b = left + d, c = left + 2*d;                         \
                    left = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))     \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                                                                                    \
                    a = pivot - d, b = pivot, c = pivot + d;                        \
                    pivot = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))    \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                                                                                    \
                    a = right - 2*d, b = right - d, c = right;                      \
                    right = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))    \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                }                                                                   \
                                                                                    \
                a = left, b = pivot, c = right;                                     \
                pivot = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))        \
                                   : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));       \
                if( pivot != left0 )                                                \
                {                                                                   \
                    CV_SWAP( *pivot, *left0, t );                                   \
                    pivot = left0;                                                  \
                }                                                                   \
                left = left1 = left0 + 1;                                           \
                right = right1 = right0;                                            \
                                                                                    \
                for(;;)                                                             \
                {                                                                   \
                    while( left <= right && !LT(*pivot, *left) )                    \
                    {                                                               \
                        if( !LT(*left, *pivot) )                                    \
                        {                                                           \
                            if( left > left1 )                                      \
                                CV_SWAP( *left1, *left, t );                        \
                            swap_cnt = 1;                                           \
                            left1++;                                                \
                        }                                                           \
                        left++;                                                     \
                    }                                                               \
                                                                                    \
                    while( left <= right && !LT(*right, *pivot) )                   \
                    {                                                               \
                        if( !LT(*pivot, *right) )                                   \
                        {                                                           \
                            if( right < right1 )                                    \
                                CV_SWAP( *right1, *right, t );                      \
                            swap_cnt = 1;                                           \
                            right1--;                                               \
                        }                                                           \
                        right--;                                                    \
                    }                                                               \
                                                                                    \
                    if( left > right )                                              \
                        break;                                                      \
                    CV_SWAP( *left, *right, t );                                    \
                    swap_cnt = 1;                                                   \
                    left++;                                                         \
                    right--;                                                        \
                }                                                                   \
                                                                                    \
                if( swap_cnt == 0 )                                                 \
                {                                                                   \
                    left = left0, right = right0;                                   \
                    goto insert_sort;                                               \
                }                                                                   \
                                                                                    \
                n = MIN( (int)(left1 - left0), (int)(left - left1) );               \
                for( i = 0; i < n; i++ )                                            \
                    CV_SWAP( left0[i], left[i-n], t );                              \
                                                                                    \
                n = MIN( (int)(right0 - right1), (int)(right1 - right) );           \
                for( i = 0; i < n; i++ )                                            \
                    CV_SWAP( left[i], right0[i-n+1], t );                           \
                n = (int)(left - left1);                                            \
                m = (int)(right1 - right);                                          \
                if( n > 1 )                                                         \
                {                                                                   \
                    if( m > 1 )                                                     \
                    {                                                               \
                        if( n > m )                                                 \
                        {                                                           \
                            stack[++sp].lb = left0;                                 \
                            stack[sp].ub = left0 + n - 1;                           \
                            left = right0 - m + 1, right = right0;                  \
                        }                                                           \
                        else                                                        \
                        {                                                           \
                            stack[++sp].lb = right0 - m + 1;                        \
                            stack[sp].ub = right0;                                  \
                            left = left0, right = left0 + n - 1;                    \
                        }                                                           \
                    }                                                               \
                    else                                                            \
                        left = left0, right = left0 + n - 1;                        \
                }                                                                   \
                else if( m > 1 )                                                    \
                    left = right0 - m + 1, right = right0;                          \
                else                                                                \
                    break;                                                          \
            }                                                                       \
        }                                                                           \
    }                                                                               \
}

#define CV_IMPLEMENT_QSORT( func_name, T, cmp )  \
    CV_IMPLEMENT_QSORT_EX( func_name, T, cmp, int )

/****************************************************************************************\
*                     Structures and macros for integration with IPP                     *
\****************************************************************************************/

/* IPP-compatible return codes */
typedef enum CvStatus
{
    CV_BADMEMBLOCK_ERR          = -113,
    CV_INPLACE_NOT_SUPPORTED_ERR= -112,
    CV_UNMATCHED_ROI_ERR        = -111,
    CV_NOTFOUND_ERR             = -110,
    CV_BADCONVERGENCE_ERR       = -109,

    CV_BADDEPTH_ERR             = -107,
    CV_BADROI_ERR               = -106,
    CV_BADHEADER_ERR            = -105,
    CV_UNMATCHED_FORMATS_ERR    = -104,
    CV_UNSUPPORTED_COI_ERR      = -103,
    CV_UNSUPPORTED_CHANNELS_ERR = -102,
    CV_UNSUPPORTED_DEPTH_ERR    = -101,
    CV_UNSUPPORTED_FORMAT_ERR   = -100,

    CV_BADARG_ERR               = -49,  //ipp comp
    CV_NOTDEFINED_ERR           = -48,  //ipp comp

    CV_BADCHANNELS_ERR          = -47,  //ipp comp
    CV_BADRANGE_ERR             = -44,  //ipp comp
    CV_BADSTEP_ERR              = -29,  //ipp comp

    CV_BADFLAG_ERR              =  -12,
    CV_DIV_BY_ZERO_ERR          =  -11, //ipp comp
    CV_BADCOEF_ERR              =  -10,

    CV_BADFACTOR_ERR            =  -7,
    CV_BADPOINT_ERR             =  -6,
    CV_BADSCALE_ERR             =  -4,
    CV_OUTOFMEM_ERR             =  -3,
    CV_NULLPTR_ERR              =  -2,
    CV_BADSIZE_ERR              =  -1,
    CV_NO_ERR                   =   0,
    CV_OK                       =   CV_NO_ERR
}
CvStatus;

#define CV_NOTHROW throw()


#ifdef __cplusplus


#define CV_CheckGlError() CV_DbgAssert( (cv::ogl::checkError(__FILE__, __LINE__, CV_Func)) )

#endif //__cplusplus

#endif // __OPENCV_CORE_INTERNAL_HPP__

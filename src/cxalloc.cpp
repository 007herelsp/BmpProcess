
#include "_cxcore.h"

// default <malloc>
static void*
icvDefaultAlloc( size_t size, void* )
{
    char *ptr, *ptr0 = (char*)malloc(
        (size_t)(size + VOS_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)));

    if( !ptr0 )
        return 0;

    // align the pointer
    ptr = (char*)cvAlignPtr(ptr0 + sizeof(char*) + 1, VOS_MALLOC_ALIGN);
    *(char**)(ptr - sizeof(char*)) = ptr0;

    return ptr;
}


// default <free>
static int
icvDefaultFree( void* ptr, void* )
{
    // Pointer must be aligned by VOS_MALLOC_ALIGN
    if( ((size_t)ptr & (VOS_MALLOC_ALIGN-1)) != 0 )
        return VOS_BADARG_ERR;
    free( *((char**)ptr - 1) );

    return VOS_OK;
}


// pointers to allocation functions, initially set to default
static CvAllocFunc p_cvAlloc = icvDefaultAlloc;
static CvFreeFunc p_cvFree = icvDefaultFree;
static void* p_cvAllocUserData = 0;

VOS_IMPL  void*  cvAlloc( size_t size )
{
    void* ptr = 0;
    
    VOS_FUNCNAME( "cvAlloc" );

    __BEGIN__;

    if( (size_t)size > VOS_MAX_ALLOC_SIZE )
        VOS_ERROR( VOS_StsOutOfRange,
                  "Negative or too large argument of cvAlloc function" );

    ptr = p_cvAlloc( size, p_cvAllocUserData );
    if( !ptr )
        VOS_ERROR( VOS_StsNoMem, "Out of memory" );

    __END__;

    return ptr;
}


VOS_IMPL  void  cvFree_( void* ptr )
{
    VOS_FUNCNAME( "cvFree_" );

    __BEGIN__;

    if( ptr )
    {
        CVStatus status = p_cvFree( ptr, p_cvAllocUserData );
        if( status < 0 )
            VOS_ERROR( status, "Deallocation error" );
    }

    __END__;
}

/* End of file. */

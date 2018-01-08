#include "core.h"
#include "misc.h"

static void*
iDefaultAlloc( size_t size)
{
    char *ptr, *ptr0 = (char*)malloc(
        (size_t)(size + VOS_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)));

    if( !ptr0 )
        return 0;

    // align the pointer
    ptr = (char*)AlignPtr(ptr0 + sizeof(char*) + 1, VOS_MALLOC_ALIGN);
    *(char**)(ptr - sizeof(char*)) = ptr0;

    return ptr;
}


// default <free>
static int
iDefaultFree( void* ptr)
{
    // Pointer must be aligned by VOS_MALLOC_ALIGN
    if( ((size_t)ptr & (VOS_MALLOC_ALIGN-1)) != 0 )
        return VOS_BADARG_ERR;
    free( *((char**)ptr - 1) );

    return VOS_OK;
}


 void*  SysAlloc( size_t size )
{
    void* ptr = 0;

    VOS_FUNCNAME( "SysAlloc" );

    __BEGIN__;

    if( (size_t)size > VOS_MAX_ALLOC_SIZE )
        VOS_ERROR( VOS_StsOutOfRange,
                  "Negative or too large argument of SysAlloc function" );

    ptr = iDefaultAlloc( size );
    if( !ptr )
        VOS_ERROR( VOS_StsNoMem, "Out of memory" );

    __END__;

    return ptr;
}


 void  SysFree_( void* ptr )
{
    VOS_FUNCNAME( "SysFree_" );

    __BEGIN__;

    if( ptr )
    {
        OPTStatus status = iDefaultFree( ptr );
        if( status < 0 )
            VOS_ERROR( status, "Deallocation error" );
    }

    __END__;
}

/* End of file. */

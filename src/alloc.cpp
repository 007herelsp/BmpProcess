#include "core.precomp.hpp"

namespace cv
{

static void* OutOfMemoryError(size_t size)
{
    CV_Error_(CV_StsNoMem, ("Failed to allocate %lu bytes", (unsigned long)size));
    return NULL;
}

void* fastMalloc( size_t size )
{
    uchar* udata = (uchar*)malloc(size + sizeof(void*) + CV_MALLOC_ALIGN);
    if(!udata)
        return OutOfMemoryError(size);
    uchar** adata = alignPtr((uchar**)udata + 1, CV_MALLOC_ALIGN);
    adata[-1] = udata;
    return adata;
}

void fastFree(void* ptr)
{
    if(ptr)
    {
        uchar* udata = ((uchar**)ptr)[-1];
        CV_DbgAssert(udata < (uchar*)ptr &&
               ((uchar*)ptr - udata) <= (ptrdiff_t)(sizeof(void*)+CV_MALLOC_ALIGN));
        free(udata);
    }
}
};


CV_IMPL void* cvAlloc( size_t size )
{
    return cv::fastMalloc( size );
}

CV_IMPL void cvFree_( void* ptr )
{
    cv::fastFree( ptr );
}


/* End of file. */

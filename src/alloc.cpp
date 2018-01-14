#include "core.h"
#include "misc.h"

static void *iDefaultAlloc(size_t size)
{
    char *ptr, *ptr0 = (char *)malloc(
                   (size_t)(size + VOS_MALLOC_ALIGN * ((size >= 4096) + 1) + sizeof(char *)));

    if (!ptr0)
        return NULL;

    // align the pointer
    ptr = (char *)SysAlignPtr(ptr0 + sizeof(char *) + 1, VOS_MALLOC_ALIGN);
    *(char **)(ptr - sizeof(char *)) = ptr0;

    return ptr;
}

static int iDefaultFree(void *ptr)
{
    // check aligned
    if (((size_t)ptr & (VOS_MALLOC_ALIGN - 1)) != 0)
        return VOS_StsBadArg;
    free(*((char **)ptr - 1));

    return VOS_StsOk;
}

void *SysAlloc(size_t size)
{
    void *ptr = NULL;

    VOS_FUNCNAME("SysAlloc");

    __BEGIN__;

    if ((size_t)size > VOS_MAX_ALLOC_SIZE)
        VOS_ERROR(VOS_StsOutOfRange,"");

    ptr = iDefaultAlloc(size);
    if (!ptr)
        VOS_ERROR(VOS_StsNoMem, "");

    __END__;

    return ptr;
}

void SysFree_(void *ptr)
{
    VOS_FUNCNAME("SysFree_");

    __BEGIN__;

    if (ptr)
    {
        int status = iDefaultFree(ptr);
        if (status < 0)
            VOS_ERROR(status, "");
    }

    __END__;
}

/* End of file. */

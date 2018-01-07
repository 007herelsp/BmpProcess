#include "core.h"
#include "misc.h"

void ConvertScale(const Mat *src, Mat *dst)
{
    VOS_FUNCNAME("ConvertScale");

    __BEGIN__;

    if (src != dst)
    {
        Copy(src, dst);
    }

    __END__;
}

/* End of file. */

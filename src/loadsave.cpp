#include "process.h"

#include "grfmt_bmp.h"

static void *
iLoadImage(const char *filename)
{
    GrFmtBmpReader *reader = NULL;
    IplImage *image = NULL;
    Mat hdr, *matrix = NULL;

    VOS_FUNCNAME("LoadImage");

    __BEGIN__;

    Size size;

    if (!filename || strlen(filename) == 0)
        VOS_ERROR(VOS_StsNullPtr, "null filename");

    reader = new GrFmtBmpReader(filename);
    if (!reader)
        EXIT;

    if (!reader->ReadHeader())
        EXIT;

    size.width = reader->GetWidth();
    size.height = reader->GetHeight();

    VOS_CALL(image = CreateImage(size, SYS_DEPTH_8U, 3));
    matrix = GetMat(image, &hdr);

    if (!reader->ReadData(matrix->data.ptr, matrix->step, 1))
    {
        ReleaseImage(&image);
        EXIT;
    }

    __END__;

    delete reader;

    if (GetErrStatus() < 0)
    {
        ReleaseImage(&image);
    }

    return (void *)image;
}

 IplImage *
LoadImage(const char *filename)
{
    return (IplImage *)iLoadImage(filename);
}

 int
SaveImage(const char *filename, const VOID *arr)
{
    int origin = 0;
    GrFmtBmpWriter *writer = NULL;
    Mat *temp = NULL, *temp2 = NULL;

    VOS_FUNCNAME("SaveImage");

    __BEGIN__;

    Mat stub, *image;
    int channels, ipl_depth;

    if (!filename || strlen(filename) == 0)
        VOS_ERROR(VOS_StsNullPtr, "null filename");

    VOS_CALL(image = GetMat(arr, &stub));

    if (VOS_IS_IMAGE(arr))
        origin = ((IplImage *)arr)->origin;

    channels = VOS_MAT_CN(image->type);
    if (1 != channels  && 3 != channels)
        VOS_ERROR(VOS_BadNumChannels, "");

    writer = new GrFmtBmpWriter(filename);
    if (!writer)
        VOS_ERROR(VOS_StsError, "could not find a filter for the specified extension");

    if (origin)
    {
        VOS_ERROR(VOS_StsError, "could not find a filter for the specified extension");
    }

    ipl_depth = ToIplDepth(image->type);

    if (!writer->WriteImage(image->data.ptr, image->step, image->width,
                            image->height, ipl_depth, channels))
        VOS_ERROR(VOS_StsError, "could not save the image");

    __END__;

    delete writer;
    ReleaseMat(&temp);
    ReleaseMat(&temp2);

    return GetErrStatus() >= 0;
}

/* End of file. */

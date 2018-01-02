#include "_highgui.h"
#include "grfmt_base.h"
#include "grfmt_bmp.h"

/****************************************************************************************\
*                              Image Readers & Writers Class                             *
\****************************************************************************************/

static void *
icvLoadImage(const char *filename, int flags)
{
    GrFmtBmpReader *reader = 0;
    IplImage *image = 0;
    Mat hdr, *matrix = 0;

    VOS_FUNCNAME("cvLoadImage");

    __BEGIN__;

    Size size;
    int iscolor;
    int cn;

    if (!filename || strlen(filename) == 0)
        VOS_ERROR(VOS_StsNullPtr, "null filename");

    reader = new GrFmtBmpReader(filename);
    if (!reader)
        EXIT;

    if (!reader->ReadHeader())
        EXIT;

    size.width = reader->GetWidth();
    size.height = reader->GetHeight();
    iscolor = 1;

    cn = iscolor ? 3 : 1;
    int type;
    type = IPL_DEPTH_8U;
    VOS_CALL(image = cvCreateImage(size, type, cn));
    matrix = GetMat(image, &hdr);

    if (!reader->ReadData(matrix->data.ptr, matrix->step, iscolor))
    {
        ReleaseImage(&image);
        EXIT;
    }

    __END__;

    delete reader;

    if (cvGetErrStatus() < 0)
    {
        ReleaseImage(&image);
    }

    return (void *)image;
}

 IplImage *
cvLoadImage(const char *filename, int iscolor)
{
    return (IplImage *)icvLoadImage(filename, iscolor);
}

 int
cvSaveImage(const char *filename, const CvArr *arr)
{
    int origin = 0;
    GrFmtBmpWriter *writer = 0;
    Mat *temp = 0, *temp2 = 0;

    VOS_FUNCNAME("cvSaveImage");

    __BEGIN__;

    Mat stub, *image;
    int channels, ipl_depth;

    if (!filename || strlen(filename) == 0)
        VOS_ERROR(VOS_StsNullPtr, "null filename");

    VOS_CALL(image = GetMat(arr, &stub));

    if (VOS_IS_IMAGE(arr))
        origin = ((IplImage *)arr)->origin;

    channels = VOS_MAT_CN(image->type);
    if (channels != 1 && channels != 3 && channels != 4)
        VOS_ERROR(VOS_BadNumChannels, "");

    writer = new GrFmtBmpWriter(filename);
    if (!writer)
        VOS_ERROR(VOS_StsError, "could not find a filter for the specified extension");

    if (origin)
    {
        VOS_ERROR(VOS_StsError, "could not find a filter for the specified extension");
    }

    ipl_depth = cvCvToIplDepth(image->type);

    if (!writer->WriteImage(image->data.ptr, image->step, image->width,
                            image->height, ipl_depth, channels))
        VOS_ERROR(VOS_StsError, "could not save the image");

    __END__;

    delete writer;
    ReleaseMat(&temp);
    ReleaseMat(&temp2);

    return cvGetErrStatus() >= 0;
}

/* End of file. */

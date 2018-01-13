#include "process.h"
#include "misc.h"
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

 int SaveImage(const char *filename, const VOID *arr)
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

static int BGR2Gray_8u_CnC1R(const uchar *src, int srcstep,
				  uchar *dstR, uchar *dstG, uchar *dstB, int dststep, Size size,
				  int src_cn)
{
	int i;
	srcstep -= size.width * src_cn;
	for (; size.height--; src += srcstep, dstR += dststep, dstG += dststep, dstB += dststep)
	{
		for (i = 0; i < size.width; i++, src += src_cn)
		{
			dstR[i] = src[2];
			dstG[i] = src[1];
			dstB[i] = src[0];
		}
	}

	return VOS_StsOk;
}

void CvtBgr2Gray_8u_C3C1(const VOID *srcarr, VOID *dstarrR, VOID *dstarrG, VOID *dstarrB)
{
	VOS_FUNCNAME("CvtBgr2Gray_8u_C3C1");

	__BEGIN__;

	Mat srcstub, *src = (Mat *)srcarr;
	Mat dststubR, *dstR = (Mat *)dstarrR;

	Mat dststubG, *dstG = (Mat *)dstarrG;
	Mat dststubB, *dstB = (Mat *)dstarrB;
	Size size;
	int src_step, dst_step;
	int src_cn, dst_cn, depth;

	VOS_CALL(src = GetMat(srcarr, &srcstub));
	VOS_CALL(dstR = GetMat(dstarrR, &dststubR));
	VOS_CALL(dstG = GetMat(dstarrG, &dststubG));
	VOS_CALL(dstB = GetMat(dstarrB, &dststubB));

	if (!VOS_ARE_SIZES_EQ(dstG, dstR))
		VOS_ERROR(VOS_StsUnmatchedSizes, "");

	if (!VOS_ARE_SIZES_EQ(dstG, dstB))
		VOS_ERROR(VOS_StsUnmatchedSizes, "");

	if (!VOS_ARE_SIZES_EQ(src, dstR))
		VOS_ERROR(VOS_StsUnmatchedSizes, "");

	if (!VOS_ARE_DEPTHS_EQ(src, dstR))
		VOS_ERROR(VOS_StsUnmatchedFormats, "");

	if (!VOS_ARE_DEPTHS_EQ(dstG, dstR))
		VOS_ERROR(VOS_StsUnmatchedFormats, "");

	if (!VOS_ARE_DEPTHS_EQ(dstB, dstR))
		VOS_ERROR(VOS_StsUnmatchedFormats, "");

	depth = VOS_MAT_DEPTH(src->type);
	if (depth != VOS_8U)
		VOS_ERROR(VOS_StsUnsupportedFormat, "");

	src_cn = VOS_MAT_CN(src->type);
	dst_cn = VOS_MAT_CN(dstR->type);
	size = GetMatSize(src);
	src_step = src->step;
	dst_step = dstR->step;

	if (3 != src_cn || 1 != dst_cn)
		VOS_ERROR(VOS_BadNumChannels,
				  "Incorrect number of channels for this conversion code");

	VOS_FUN_CALL(BGR2Gray_8u_CnC1R(src->data.ptr, src_step,
								   dstR->data.ptr, dstG->data.ptr, dstB->data.ptr, dst_step, size, src_cn));
	__END__;
}

/* End of file. */

/* End of file. */

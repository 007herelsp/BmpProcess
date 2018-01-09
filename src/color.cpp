#include "process.h"
#include "misc.h"

static VosStatus
BGR2Gray_8u_CnC1R(const uchar *src, int srcstep,
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


	return VOS_OK;
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

	if (3!=src_cn   ||  1!=dst_cn )
		VOS_ERROR(VOS_BadNumChannels,
		"Incorrect number of channels for this conversion code");

	VOS_FUN_CALL(BGR2Gray_8u_CnC1R(src->data.ptr, src_step,
        dstR->data.ptr, dstG->data.ptr, dstB->data.ptr, dst_step, size, src_cn));
	__END__;
}

/* End of file. */

#include "process.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <malloc.h>
#include <set>

#define ERR_1 -1
using namespace std;

static double angle(Point *pt1, Point *pt2, Point *pt0)
{
	double dx1 = pt1->x - pt0->x;
	double dy1 = pt1->y - pt0->y;
	double dx2 = pt2->x - pt0->x;
	double dy2 = pt2->y - pt0->y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}
//static const float MAX_P = 40.0f;
#define MAX_P 10.0
typedef struct stBox
{
	Point pt[4];
	Box2D box;
	bool isRect;
	double contourArea;

	bool operator<(const struct stBox &x) const
	{

		if (fabs(box.center.x - x.box.center.x) <= MAX_P && fabs(box.center.y - x.box.center.y) <= MAX_P)
		{
			return false;
		}

		bool ret = false;
		if (isRect != x.isRect)
		{
			return isRect;
		}

		else
		{
			if (box.center.x < x.box.center.x)
			{
				ret = true;
			}
			else if ((box.center.x - x.box.center.x) <= DBL_EPSILON)
			{
				ret = (box.center.y < x.box.center.y);
			}
			else
			{
				ret = false;
			}
		}
		return ret;
	}

} Box;

bool isEqu(float x1, float x2)
{
	return (fabs(x1 - x2) <= MAX_P);
}

struct SymUBoxCmp
{
	bool operator()(const Box &x, const Box &y) const
	{
		bool ret = false;

		if (x.isRect && (!y.isRect))
		{
			return true;
		}

		if (x.box.center.x < y.box.center.x)
		{
			ret = true;
		}
		else if (fabs(x.box.center.x - y.box.center.x) <= DBL_EPSILON)
		{
			ret = (x.box.center.y < y.box.center.y);
		}
		else
		{
			ret = false;
		}

		return ret;
	}
};
void AddRecord(set<Box> &lstRes, Box &box, int &iRectCun)
{
	set<Box>::iterator itu;
	Box tbox;
	bool flag = true;
	for (itu = lstRes.begin(); itu != lstRes.end(); itu++)
	{
		tbox = *itu;

		if (fabs(tbox.box.center.x - box.box.center.x) <= MAX_P && fabs(tbox.box.center.y - box.box.center.y) <= MAX_P)
		{
			flag = false;
			//选择最大的面积
			//int width = VOS_MAX(tbox.box.size.width, box.box.size.width);
			if (tbox.contourArea < box.contourArea)
			{
				flag = true;
				if (tbox.isRect)
				{
					iRectCun--;
				}
				lstRes.erase(tbox);
			}
			break;
		}
	}
	if (flag)
	{
		if (box.isRect)
		{
			iRectCun++;
		}
		lstRes.insert(box);
	}
}
#define SHAPE 4
#define MIN_ContourArea 500
#define MAX_ContourArea 1000000
void StartSearchProcess(IplImage *lpSrcImg, set<Box> &lstRes)
{
	MemStorage *storage = CreateMemStorage();
	Seq *contours = NULL;
	Seq *result = NULL;
	double s, t;
	double dContourArea;
	Box2D End_Rage2D;
	int index = 0;
	int iRectCun = 0;
	FindContours(lpSrcImg, storage, &contours, sizeof(Contour),
				 VOS_RETR_LIST, VOS_CHAIN_APPROX_SIMPLE, InitPoint(0, 0));

	while (contours)
	{
		result = ApproxPoly(contours, sizeof(Contour), storage,
							ContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (SHAPE == result->total)
			{
				dContourArea = fabs(ContourArea(result, VOS_WHOLE_SEQ));
				if (dContourArea >= MIN_ContourArea && dContourArea <= MAX_ContourArea && CheckContourConvexity(result))
				{
					End_Rage2D = MinAreaRect2(result);
					s = 0;
					Box box = {0};
					box.box = End_Rage2D;
					for (index = 2; index < 5; index++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (index >= 2)
						{
							t = fabs(angle(
								(Point *)GetSeqElem(result, index % SHAPE),
								(Point *)GetSeqElem(result, index - 2),
								(Point *)GetSeqElem(result, index - 1)));
							s = s > t ? s : t;
						}
					}

					Point *tp;
					for (int i = 0; i < SHAPE; i++)
					{
						tp = (Point *)GetSeqElem(result, i);
						box.pt[i].x = tp->x;
						box.pt[i].y = tp->y;
						//  SeqPush(squares, (Point *)GetSeqElem(result, i));
					}
					if (s < 0.1)
					{
						box.isRect = true;
						//box.pt = box.box.pt;
					}
					else
					{
						box.isRect = false;
					}

					//printf("hello: %g, %g\n", box.box.center.x, box.box.center.y);

					//printf("centerInfo:[%f,%f]:[%f,%f]\n", End_Rage2D.center.x, End_Rage2D.center.y, End_Rage2D.size.width, End_Rage2D.size.height);
					//lstRes.insert(box);
					box.contourArea = dContourArea;
					lstRes.insert(box);
					iRectCun++;
				}
			}
		}
		contours = contours->h_next;
	}
	//printf("%d\n", iRectCun++);
	ReleaseMemStorage(&storage);
	return;
}

int Process(set<Box> &setURes, IplImage *lpTargetImg, int argc, char *argv[])
{
	set<Box>::iterator itu;
	Point2D32f srcTri[SHAPE], dstTri[SHAPE];
	IplImage *temp = NULL;
	Point p[SHAPE];
	Mat *warp_mat;
	int iCount = 0;
	Box box;
	itu = setURes.begin();
	int fileIndex = 0;
	for (itu = setURes.begin(); itu != setURes.end(); itu++)
	{
		box = *itu;
		if (box.isRect)
		{
			iCount++;
		}
		if (fileIndex < argc)
		{
			IplImage *lpSrcImg = LoadImage(argv[fileIndex++]);
			if (lpSrcImg != NULL)
			{
				temp = lpSrcImg;
				warp_mat = CreateMat(3, 3, VOS_64FC1);
				Point pt;
				for (int j = 0; j < SHAPE; j++)
				{
					for (int i = 0; i < SHAPE - j; i++)
					{
						if (box.pt[i].x > box.pt[i + 1].x)
						{
							pt = box.pt[i];
							box.pt[i] = box.pt[i + 1];
							box.pt[i + 1] = pt;
						}
					}
				}
				//printf("after centerInfo:[%f,%f]:[%f,%f], Rect=%d\n", box.box.center.x, box.box.center.y, box.box.size.width, box.box.size.height, box.isRect);

				if (box.pt[0].y > box.pt[1].y)
				{
					p[0] = box.pt[0];
					p[1] = box.pt[1];
				}
				else
				{
					p[0] = box.pt[1];
					p[1] = box.pt[0];
				}

				if (box.pt[2].y > box.pt[3].y)
				{
					p[2] = box.pt[3];
					p[3] = box.pt[2];
				}
				else
				{
					p[2] = box.pt[2];
					p[3] = box.pt[3];
				}
				int diff = +1;
				if (p[0].x - diff >= 0)
				{
					p[0].x -= diff;
				}
				p[0].y += diff;

				p[1].x -= diff;
				p[1].y -= diff;

				p[2].x += diff;
				p[2].y -= diff;
				p[3].x += diff;
				p[3].y += diff;
				for (int i = 0; i < SHAPE; i++)
				{
					dstTri[i].x = (float)p[i].x;
					dstTri[i].y = (float)p[i].y;
				}

				srcTri[1].x = (float)0;
				srcTri[1].y = (float)0;
				srcTri[2].x = (float)temp->width - 1 + 1;
				srcTri[2].y = (float)0;
				srcTri[3].x = (float)temp->width - 1 + 1;
				srcTri[3].y = (float)temp->height - 1 + 1;
				srcTri[0].x = (float)0; //bot right
				srcTri[0].y = (float)temp->height - 1 + 1;

				GetPerspectiveTransform(dstTri, srcTri, warp_mat);										 //�����Ե�������任
				WarpPerspective(temp, lpTargetImg, warp_mat, VOS_INTER_LINEAR | (VOS_WARP_INVERSE_MAP)); //��ͼ��������任
			}
		}
	}
	return iCount;
}

int main(int argc, char **args)
{
	if (3 > argc)
	{
		return ERR_1;
	}
	const char *OutputPath = (const char *)args[1];
	const char *TargetPicPath = (const char *)args[2];

	IplImage *lpTargetImg = LoadImage(TargetPicPath);
	if (NULL == lpTargetImg)
	{
		return ERR_1;
	}

	const int channel = 3;
	IplImage *imagChannels[channel] = {0};
	for (int i = 0; i < channel; i++)
	{
		imagChannels[i] = CreateImage(GetSize(lpTargetImg), 8, 1);

		assert(NULL != imagChannels[i]);
	}

	CvtBgr2Gray_8u_C3C1(lpTargetImg, imagChannels[0], imagChannels[1], imagChannels[2]);

	IplImage *lpCannyImg = CreateImage(GetSize(lpTargetImg), 8, 1);
	IplImage *lpDilateImg = CreateImage(GetSize(lpTargetImg), 8, 1);

	set<Box> lstRes;
	int iCunt = 0;
	char name[100];
	if (NULL != lpCannyImg && NULL != lpDilateImg)
	{
		for (int i = 0; i < channel; i++)
		{
			//GaussianSmooth(imagChannels[i], imagChannels[i], 3, 3, 0, 0);
			//sprintf(name, "canny%d.bmp", i);
			Canny(imagChannels[i], lpCannyImg, 0, 0, 3);
			//SaveImage(name, lpCannyImg);
			ReleaseImage(&imagChannels[i]);
			Dilate(lpCannyImg, lpDilateImg, 0, 1);
			Erode(lpDilateImg, lpDilateImg, 0, 1);
			StartSearchProcess(lpDilateImg, lstRes);
		}

		IplImage *lpOutImg = CloneImage(lpTargetImg);

		if (NULL != lpOutImg)
		{
			iCunt = Process(lstRes, lpOutImg, argc - 3, &args[3]);
			SaveImage(OutputPath, lpOutImg);
			ReleaseImage(&lpOutImg);
		}

		ReleaseImage(&lpTargetImg);
		ReleaseImage(&lpDilateImg);
		ReleaseImage(&lpCannyImg);
	}
	printf("%d\n", iCunt);
	return 0;
}

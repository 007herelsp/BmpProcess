
#include "cv.h"
#include "highgui.h"
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

typedef struct stBox
{
	Point pt[4];
	Box2D box;
	bool isRect;
} Box;

#define MAX_P 40.0

bool isEqu(float x1, float x2)
{
	return (fabs(x1 - x2) <= MAX_P);
}

struct SymUBoxCmp
{
	bool operator()(const Box &x, const Box &y) const
	{
		if (fabs(x.box.center.x - y.box.center.x) <= MAX_P && fabs(x.box.center.y - y.box.center.y) <= MAX_P)
		{
			if (fabs(x.box.size.width - y.box.size.width) <= MAX_P && fabs(x.box.size.height - y.box.size.height) <= MAX_P)
			{
				return false;
			}
		}

		bool ret = false;
		if (x.box.center.x == y.box.center.x)
		{
			ret = x.box.center.y < y.box.center.y;
		}
		else
		{
			ret = x.box.center.x < y.box.center.x;
		}
		return ret;
	}
};

void SearchProcess_v3(IplImage *lpSrcImg, set<Box, SymUBoxCmp> &lstRes)
{
	MemStorage *storage = CreateMemStorage();
	Seq *contours = NULL;
	Seq *result = NULL;
	double s, t;
	double dContourArea;
	Box2D End_Rage2D;
	int index = 0;
	// ����һ�����������ڴ洢�����ǵ�
	FindContours(lpSrcImg, storage, &contours, sizeof(CvContour),
				 VOS_RETR_LIST, VOS_CHAIN_APPROX_SIMPLE, InitPoint(0, 0));
	int iCount = 0;
	SaveImage("c.bmp", lpSrcImg);

	// �����ҵ���ÿ������contours
	while (contours)
	{
		//��ָ�����ȱƽ����������
		result = ApproxPoly(contours, sizeof(CvContour), storage,
							VOS_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (4 == result->total)
			{
				dContourArea = fabs(ContourArea(result, VOS_WHOLE_SEQ));
				if (dContourArea >= 500 && dContourArea <= 1000000 && CheckContourConvexity(result))
				{
					End_Rage2D = MinAreaRect2(contours);
					s = 0;
					Box box = {0};
					box.box = End_Rage2D;
					for (index = 2; index < 5; index++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (index >= 2)
						{
							t = fabs(angle(
								(Point *)GetSeqElem(result, index % 4),
								(Point *)GetSeqElem(result, index - 2),
								(Point *)GetSeqElem(result, index - 1)));
							s = s > t ? s : t;
						}
					}

					// if ����ֵ �㹻С�������϶��Ƕ�Ϊ90��ֱ��
					//cos0.1=83�ȣ��ܽϺõ�����ֱ��
					Point *tp;
					for (int i = 0; i < 4; i++)
					{
						tp = (Point *)GetSeqElem(result, i);
						box.pt[i].x = tp->x;
						box.pt[i].y = tp->y;
						//  SeqPush(squares, (Point *)GetSeqElem(result, i));
					}
					if (s < 0.1)
					{
						box.isRect = true;
					}
					else
					{
						box.isRect = false;
					}

					//printf("centerInfo:[%f,%f]:[%f,%f]\n", End_Rage2D.center.x, End_Rage2D.center.y, End_Rage2D.size.width, End_Rage2D.size.height);
					lstRes.insert(box);
					iCount++;
				}
			}
		}
		// ����������һ������
		contours = contours->h_next;
	}

	ReleaseMemStorage(&storage);

	return ;
}

int process_v3(set<Box, SymUBoxCmp> &setURes, IplImage *lpTargetImg, int argc, char *argv[])
{
	set<Box, SymUBoxCmp>::iterator itu;
	Point2D32f srcTri[4], dstTri[4];
	IplImage *temp;
	Point p[4];
	Mat *warp_mat;
	int iCount = 0;
	Box tbox;
	for (itu = setURes.begin(); itu != setURes.end(); itu++)
	{
		if ((*itu).isRect)
		{
			iCount++;
		}
		tbox = *itu;
	}

	Box box;
	itu = setURes.begin();
	for (int i = 0; i < argc && itu != setURes.end(); i++)
	{
		IplImage *lpSrcImg = LoadImage(argv[i], 1);
		if (lpSrcImg != NULL)
		{
			box = *itu;
			itu++;

			temp = lpSrcImg;

			warp_mat = CreateMat(3, 3, VOS_64FC1);

			Point pt;
			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 4 - j; i++)
				{
					if (box.pt[i].x > box.pt[i + 1].x)
					{
						pt = box.pt[i];
						box.pt[i] = box.pt[i + 1];
						box.pt[i + 1] = pt;
					}
				}
			}
			//printf("after centerInfo:[%f,%f]:[%f,%f]\n", box.box.center.x, box.box.center.y, box.box.size.width, box.box.size.height);

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
			for (int i = 0; i < 4; i++)
			{
				dstTri[i].x = p[i].x;
				dstTri[i].y = p[i].y;
			}

			if (box.box.size.width > box.box.size.height)
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1 + 1;
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1 + 1;
				srcTri[3].y = temp->height - 1 + 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1 + 1;
			}
			else
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1 + 1;
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1 + 1;
				srcTri[3].y = temp->height - 1 + 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1 + 1;
			}

			GetPerspectiveTransform(dstTri, srcTri, warp_mat);										 //�����Ե�������任
			WarpPerspective(temp, lpTargetImg, warp_mat, VOS_INTER_LINEAR | (VOS_WARP_INVERSE_MAP)); //��ͼ��������任
		}
	}
	return iCount;
}

set<Box, SymUBoxCmp> SearchProcess_v2(IplImage *lpSrcImg)
{
	MemStorage *storage = CreateMemStorage();
	Seq *contours = NULL;
	Seq *result = NULL;
	double s, t;
	double dContourArea;
	Box2D End_Rage2D;
	int index = 0;
	set<Box, SymUBoxCmp> lstRes;
	// ����һ�����������ڴ洢�����ǵ�
	FindContours(lpSrcImg, storage, &contours, sizeof(CvContour),
				 VOS_RETR_LIST, VOS_CHAIN_APPROX_SIMPLE, InitPoint(0, 0));
	int iCount = 0;
	SaveImage("c.bmp", lpSrcImg);

	// �����ҵ���ÿ������contours
	while (contours)
	{
		//��ָ�����ȱƽ����������
		result = ApproxPoly(contours, sizeof(CvContour), storage,
							VOS_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (4 == result->total)
			{
				dContourArea = fabs(ContourArea(result, VOS_WHOLE_SEQ));
				if (dContourArea >= 500 && dContourArea <= 1000000 && CheckContourConvexity(result))
				{
					End_Rage2D = MinAreaRect2(contours);
					s = 0;
					Box box = {0};
					box.box = End_Rage2D;
					for (index = 2; index < 5; index++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (index >= 2)
						{
							t = fabs(angle(
								(Point *)GetSeqElem(result, index % 4),
								(Point *)GetSeqElem(result, index - 2),
								(Point *)GetSeqElem(result, index - 1)));
							s = s > t ? s : t;
						}
					}

					// if ����ֵ �㹻С�������϶��Ƕ�Ϊ90��ֱ��
					//cos0.1=83�ȣ��ܽϺõ�����ֱ��
					Point *tp;
					for (int i = 0; i < 4; i++)
					{
						tp = (Point *)GetSeqElem(result, i);
						box.pt[i].x = tp->x;
						box.pt[i].y = tp->y;
						//  SeqPush(squares, (Point *)GetSeqElem(result, i));
					}
					if (s < 0.1)
					{
						box.isRect = true;
					}
					else
					{
						box.isRect = false;
					}

					if (box.box.size.width > 600)
					{
						int ib = 0;
					}
					//printf("centerInfo:[%f,%f]:[%f,%f]\n", End_Rage2D.center.x, End_Rage2D.center.y, End_Rage2D.size.width, End_Rage2D.size.height);
					lstRes.insert(box);
					iCount++;
				}
			}
		}
		// ����������һ������
		contours = contours->h_next;
	}

	ReleaseMemStorage(&storage);

	return std::move(lstRes);
}

int process_v2(IplImage *lpImg, IplImage *lpTargetImg, int argc, char *argv[])
{
	set<Box, SymUBoxCmp> setURes = SearchProcess_v2(lpImg);
	set<Box, SymUBoxCmp>::iterator itu;
	Point2D32f srcTri[4], dstTri[4];
	IplImage *temp;
	Point p[4];
	Mat *warp_mat;
	int iCount = 0;
	Box tbox;
	for (itu = setURes.begin(); itu != setURes.end(); itu++)
	{
		if ((*itu).isRect)
		{
			iCount++;
		}
		tbox = *itu;
	}

	Box box;
	itu = setURes.begin();
	for (int i = 0; i < argc && itu != setURes.end(); i++)
	{
		//printf("%s\n", argv[i]);
		IplImage *lpSrcImg = LoadImage(argv[i], 1);
		if (lpSrcImg != NULL)
		{
			box = *itu;
			itu++;

			temp = lpSrcImg;

			warp_mat = CreateMat(3, 3, VOS_64FC1);

			//����
			Point pt;
			for (int j = 0; j < 4; j++) /* ���ݷ�Ҫ����n��*/
			{
				for (int i = 0; i < 4 - j; i++) /* ֵ�Ƚϴ��Ԫ�س���ȥ��ֻ��ʣ�µ�Ԫ���е����ֵ�ٳ���ȥ�Ϳ����� */
				{
					if (box.pt[i].x > box.pt[i + 1].x) /* ��ֵ�Ƚϴ��Ԫ�س����� */
					{
						pt = box.pt[i];
						box.pt[i] = box.pt[i + 1];
						box.pt[i + 1] = pt;
					}
				}
			}
			//printf("after centerInfo:[%f,%f]:[%f,%f]\n", box.box.center.x, box.box.center.y, box.box.size.width, box.box.size.height);

			//��p0��
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
			for (int i = 0; i < 4; i++)
			{
				dstTri[i].x = p[i].x;
				dstTri[i].y = p[i].y;
			}

			//����������任
			if (box.box.size.width > box.box.size.height)
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1 + 1; //��Сһ������
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1 + 1;
				srcTri[3].y = temp->height - 1 + 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1 + 1;
			}
			else
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1 + 1; //��Сһ������
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1 + 1;
				srcTri[3].y = temp->height - 1 + 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1 + 1;
			}

			GetPerspectiveTransform(dstTri, srcTri, warp_mat);										 //�����Ե�������任
			WarpPerspective(temp, lpTargetImg, warp_mat, VOS_INTER_LINEAR | (VOS_WARP_INVERSE_MAP)); //��ͼ��������任
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

	IplImage *lpTargetImg = LoadImage(TargetPicPath, 1);
	if (NULL == lpTargetImg)
	{
		return ERR_1;
	}

	int channel = 3;
	IplImage *imagChannels[channel] = {0};
	for (int i = 0; i < channel; i++)
	{
		imagChannels[i] = CreateImage(GetSize(lpTargetImg), 8, 1);

		assert(NULL != imagChannels[i] );
	}

	CvtColor(lpTargetImg, imagChannels[0], imagChannels[1], imagChannels[2], VOS_RGB2GRAY);

	IplImage *lpCannyImg = CreateImage(GetSize(lpTargetImg), 8, 1);
	IplImage *lpDilateImg = CreateImage(GetSize(lpTargetImg), 8, 1);

	set<Box, SymUBoxCmp> lstRes;
	char name[100];
	if (NULL != lpCannyImg && NULL != lpDilateImg)
	{
		for (int i = 0; i < channel; i++)
		{
			sprintf( name, "grayimg_%d.bmp",i);
			SaveImage(name, imagChannels[i]);
			Canny(imagChannels[i], lpCannyImg, 14.4, 36, 3);
			ReleaseImage(&imagChannels[i]);
			sprintf( name, "cannyimg_%d.bmp",i);
			SaveImage(name, lpCannyImg);
			Dilate(lpCannyImg, lpDilateImg, 0, 1);
			Erode(lpDilateImg, lpDilateImg, 0, 1);
			sprintf( name, "erodeimg_%d.bmp",i);
			SaveImage(name, lpDilateImg);
			SearchProcess_v3(lpDilateImg, lstRes);
		}

		IplImage *lpOutImg = CloneImage(lpTargetImg);
		int iCunt = 0;
		if (NULL != lpOutImg)
		{
			iCunt = process_v3(lstRes, lpOutImg, argc - 3, &args[3]);
			SaveImage(OutputPath, lpOutImg);

			ReleaseImage(&lpOutImg);
		}

		printf("%d\n", iCunt);

		ReleaseImage(&lpTargetImg);
		ReleaseImage(&lpDilateImg);
		ReleaseImage(&lpCannyImg);
	}

	return 0;
}

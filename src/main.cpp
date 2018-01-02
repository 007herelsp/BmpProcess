//#include "core.hpp"
//#include "types_c.h"
//#include "core_c.h"
//#include "imgproc_c.h"
//#include "highgui_c.h"
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
//#include "common.h"

#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <malloc.h>

#include <set>

#define ERR_1 -1
using namespace std;

#define cvShowImage(x, y)

#define cvWaitKey(x)

double angle(Point *pt1, Point *pt2, Point *pt0)
{
	double dx1 = pt1->x - pt0->x;
	double dy1 = pt1->y - pt0->y;
	double dx2 = pt2->x - pt0->x;
	double dy2 = pt2->y - pt0->y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

struct SymCmp
{
	bool operator()(const Box2D &x, const Box2D &y) const
	{
		if (x.center.x == y.center.x)
		{
			return x.center.y < y.center.y;
		}
		else
		{
			return x.center.x < y.center.x;
		}
	}
};

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

struct SymBoxCmp
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
		return true;
	}
};

set<Box, SymUBoxCmp> SearchProcess_v2(IplImage *lpSrcImg)
{
	MemStorage *storage = CreateMemStorage();
	Seq_t *contours = NULL;
	Seq_t *result = NULL;
	double s, t;
	double dContourArea;
	Box2D End_Rage2D;
	int index = 0;
	set<Box, SymUBoxCmp> lstRes;
	// ����һ�����������ڴ洢�����ǵ�
	cvFindContours(lpSrcImg, storage, &contours, sizeof(CvContour),
				   VOS_RETR_LIST, VOS_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	int iCount = 0;
	cvSaveImage("c.bmp", lpSrcImg);
	// �����ҵ���ÿ������contours
	while (contours)
	{
		//��ָ�����ȱƽ����������
		result = cvApproxPoly(contours, sizeof(CvContour), storage,
							  VOS_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (result->total == 4)
			{
				dContourArea = fabs(cvContourArea(result, VOS_WHOLE_SEQ));
				if (dContourArea >= 500 && dContourArea <= 1000000 && cvCheckContourConvexity(result))
				{
					End_Rage2D = cvMinAreaRect2(contours);
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
						//  cvSeqPush(squares, (Point *)GetSeqElem(result, i));
					}
					if (s < 0.1)
					{
						box.isRect = true;
					}
					else
					{
						box.isRect = false;
					}
					printf("centerInfo:[%f,%f]:[%f,%f]\n", End_Rage2D.center.x, End_Rage2D.center.y, End_Rage2D.size.width, End_Rage2D.size.height);
					lstRes.insert(box);
					iCount++;

					// if (abs(x.box.center.x - y.box.center.x) <= MAX_P && abs(x.box.center.y - y.box.center.y) <= MAX_P)
					// {
					//     //if(abs(x.box.size.width - y.box.size.width)  <= MAX_P && abs(x.box.size.height - y.box.size.height)<= MAX_P )
					//     {
					//         return false;
					//     }
					// }
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

	set<Box, SymBoxCmp> lstRes;
	set<Box, SymUBoxCmp> setURes = SearchProcess_v2(lpImg);
	set<Box, SymUBoxCmp>::iterator itu;
	Point2D32f srcTri[4], dstTri[4];
	IplImage *temp;
	Point p[4];
	Mat *warp_mat;
	int iCount = 0;

	for (itu = setURes.begin(); itu != setURes.end(); itu++)
	{
		if ((*itu).isRect)
		{
			iCount++;
		}
		lstRes.insert(*itu);
	}
	set<Box, SymBoxCmp>::iterator it;

	Box box;
	it = lstRes.begin();
	for (int i = 0; i < argc && it != lstRes.end(); i++)
	{
		printf("%s\n", argv[i]);
		IplImage *lpSrcImg = cvLoadImage(argv[i], 1);
		if (lpSrcImg != NULL)
		{
			box = *it;
			it++;

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
			printf("after centerInfo:[%f,%f]:[%f,%f]\n", box.box.center.x, box.box.center.y, box.box.size.width, box.box.size.height);

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
			int diff = +5;
			p[0].x -= diff;
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
				srcTri[2].x = temp->width - 1; //��Сһ������
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1;
				srcTri[3].y = temp->height - 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1;
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

				// srcTri[2].x = 0;
				// srcTri[2].y = 0;
				// srcTri[3].x = temp->width - 1; //��Сһ������
				// srcTri[3].y = 0;
				// srcTri[0].x = temp->width - 1;
				// srcTri[0].y = temp->height - 1;
				// srcTri[1].x = 0; //bot right
				// srcTri[1].y = temp->height - 1;
			}

			cvGetPerspectiveTransform(dstTri, srcTri, warp_mat);									   //�����Ե�������任
			cvWarpPerspective(temp, lpTargetImg, warp_mat, VOS_INTER_LINEAR | (VOS_WARP_INVERSE_MAP)); //��ͼ��������任
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

	IplImage *lpTargetImg = cvLoadImage(TargetPicPath, 1);
	if (NULL == lpTargetImg)
	{
		return ERR_1;
	}

	IplImage *gray = cvCreateImage(GetSize(lpTargetImg), 8, 1);
	CvtColor(lpTargetImg, gray, VOS_RGB2GRAY);
	IplImage *lpCannyImg = cvCreateImage(GetSize(lpTargetImg), 8, 1);
	IplImage *tmp = cvCreateImage(GetSize(lpTargetImg), 8, 1);
	IplImage *lpDilateImg = cvCreateImage(GetSize(lpTargetImg), 8, 1);
	cvSmooth(gray, tmp, VOS_GAUSSIAN, 3, 3, 0, 0); //��˹�˲�
	cvSaveImage("tmp.bmp", tmp);
	printf("save\n");
	if (NULL != lpCannyImg && NULL != lpDilateImg)
	{
		//cvSmooth(gray, gray, VOS_GAUSSIAN, 3, 3, 0, 0);
		Canny(gray, lpCannyImg, 0.5, 20, 3);
		cvSaveImage("canny1.bmp", lpCannyImg);
		cvDilate(lpCannyImg, lpDilateImg, 0, 1);
		cvSaveImage("lpDilate.bmp", lpDilateImg);
		cvErode(lpDilateImg, lpDilateImg, 0, 1);
		cvSaveImage("lpErode.bmp", lpDilateImg);

		IplImage *pGrayImage = cvCreateImage(GetSize(lpTargetImg), IPL_DEPTH_8U, 1);
		IplImage *pGrayEqualizeImage = cvCreateImage(GetSize(lpTargetImg), IPL_DEPTH_8U, 1);
		cvSmooth(lpTargetImg, lpTargetImg, VOS_GAUSSIAN, 5, 5, 0, 0); //��˹�˲�
		cvSaveImage("pTargetImg.bmp", lpTargetImg);
		CvtColor(lpTargetImg, pGrayImage, VOS_BGR2GRAY);
		cvSaveImage("pGrayImage.bmp", pGrayImage);
		//cvEqualizeHist(pGrayImage, pGrayEqualizeImage);
		cvSaveImage("pGrayEqualizeImage.bmp", pGrayEqualizeImage);

		//Canny(pGrayEqualizeImage, lpCannyImg, 0.5, 20, 3);
		//cvSaveImage("canny2.bmp", lpCannyImg);
	}
	//����Ŀ������
	IplImage *lpOutImg = CloneImage(lpTargetImg);
	int iCunt = 0;
	if (NULL != lpOutImg)
	{
		iCunt = process_v2(lpDilateImg, lpOutImg, argc - 3, &args[3]);

		cvSaveImage(OutputPath, lpOutImg);
	}

	//�������
	printf("%d\n", iCunt);
	//�ͷ���Դ
	ReleaseImage(&lpOutImg);
	ReleaseImage(&lpTargetImg);
	ReleaseImage(&lpDilateImg);
	ReleaseImage(&lpCannyImg);

	return 0;
}

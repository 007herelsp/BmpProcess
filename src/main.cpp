#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/core/core_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include "process.h"

#include <stdio.h>
#include "common.h"

#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <malloc.h>

#include <set>
using namespace std;

#define cvShowImage(x, y)

#define cvWaitKey(x)

double angle(CvPoint *pt1, CvPoint *pt2, CvPoint *pt0)
{
	double dx1 = pt1->x - pt0->x;
	double dy1 = pt1->y - pt0->y;
	double dx2 = pt2->x - pt0->x;
	double dy2 = pt2->y - pt0->y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

//drawSquares函数用来画出在图像中找到的所有正方形轮廓
void drawSquares(IplImage *img, CvSeq *squares)
{
	CvSeqReader reader;
	IplImage *cpy = cvCloneImage(img);
	int i;
	cvStartReadSeq(squares, &reader, 0);
	printf("total = %d\n", squares->total);
	// read 4 sequence elements at a time (all vertices of a square)

	for (i = 0; i < squares->total; i += 4)
	{
		CvPoint pt[4], *rect = pt;
		int count = 4;

		// read 4 vertices
		CV_READ_SEQ_ELEM(pt[0], reader);
		CV_READ_SEQ_ELEM(pt[1], reader);
		CV_READ_SEQ_ELEM(pt[2], reader);
		CV_READ_SEQ_ELEM(pt[3], reader);

		// draw the square as a closed polyline
		// cvPolyLine(cpy, &rect, &count, 1, 1, CV_RGB(0, 0, 255), 2, CV_AA, 0);
	}

	cvShowImage("xx", cpy);
	/*const char* filename = "111111111111111111111.jpg";
	const CvArr* image = cpy;
	int cvSaveImage("1111111111111111111111.jpg",image);*/
	char *filename2 = "rsult.bmp"; //图像名
	cvSaveImage(filename2, cpy);   //把图像写入文件

	//Mat mtx(cpy);

	cvReleaseImage(&cpy);
}
//旋转图像内容不变，尺寸相应变大
IplImage *rotateImage2(IplImage *img, IplImage *img_rotate, int degree)
{
	double angle = degree * CV_PI / 180.;
	double a = sin(angle), b = cos(angle);
	int width = img->width, height = img->height;
	//旋转后的新图尺寸
	int width_rotate = int(height * fabs(a) + width * fabs(b));
	int height_rotate = int(width * fabs(a) + height * fabs(b));
	//IplImage *img_rotate = cvCreateImage(cvSize(width_rotate, height_rotate), img->depth, img->nChannels);
	//cvZero(img_rotate);
	//保证原图可以任意角度旋转的最小尺寸
	int tempLength = sqrt((double)width * width + (double)height * height) + 10;
	int tempX = (tempLength + 1) / 2 - width / 2;
	int tempY = (tempLength + 1) / 2 - height / 2;
	IplImage *temp = cvCreateImage(cvSize(tempLength, tempLength), img->depth, img->nChannels);
	cvZero(temp);
	//将原图复制到临时图像tmp中心
	//cvSetImageROI(img_rotate, cvRect(tempX, tempY, width, height));
	// cvCopy(img, temp, NULL);

	//旋转数组map
	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
	// [ m3  m4  m5 ] ===>  [ A21  A22   b2 ]
	float m[6];
	int w = img->width;
	int h = img->height;
	m[0] = b;
	m[1] = a;
	m[3] = -m[1];
	m[4] = m[0];
	// 将旋转中心移至图像中间
	m[2] = w * 0.5f;
	m[5] = h * 0.5f;
	CvMat M = cvMat(2, 3, CV_32F, m);
	//cvGetQuadrangleSubPix(img, img_rotate, &M);
	cvReleaseImage(&temp);
	cvResetImageROI(img_rotate);
	return img_rotate;
}

IplImage *rotateImage3(IplImage *img, IplImage *img_rotate, float degree)
{
	double angle = degree * CV_PI / 180.;
	double a = sin(angle), b = cos(angle);
	int width = img->width, height = img->height;
	//旋转后的新图尺寸
	int width_rotate = int(height * fabs(a) + width * fabs(b));
	int height_rotate = int(width * fabs(a) + height * fabs(b));
	//IplImage *img_rotate = cvCreateImage(cvSize(width_rotate, height_rotate), img->depth, img->nChannels);
	//cvZero(img_rotate);
	//保证原图可以任意角度旋转的最小尺寸
	int tempLength = sqrt((double)width * width + (double)height * height) + 10;
	int tempX = (tempLength + 1) / 2 - width / 2;
	int tempY = (tempLength + 1) / 2 - height / 2;
	IplImage *temp = cvCreateImage(cvSize(tempLength, tempLength), img->depth, img->nChannels);
	cvZero(temp);
	//将原图复制到临时图像tmp中心
	cvSetImageROI(temp, cvRect(tempX, tempY, width, height));
	cvCopy(img, temp, NULL);
	cvResetImageROI(temp);
	//旋转数组map
	// [ m0  m1  m2 ] ===>  [ A11  A12   b1 ]
	// [ m3  m4  m5 ] ===>  [ A21  A22   b2 ]
	float m[6];
	int w = temp->width;
	int h = temp->height;
	m[0] = b;
	m[1] = a;
	m[3] = -m[1];
	m[4] = m[0];
	// 将旋转中心移至图像中间
	m[2] = w * 0.5f;
	m[5] = h * 0.5f;
	CvMat M = cvMat(2, 3, CV_32F, m);
	//cvGetQuadrangleSubPix(temp, img_rotate, &M);
	cvReleaseImage(&temp);
	return img_rotate;
}

IplImage *protc(IplImage *imgin, IplImage *imgin2, float theta)
{
	int x;
	int y;

	for (int i = 0; i < imgin->height; i++)
	{
		for (int j = 0; j < imgin->width; j++)
		{
			float fy = (float(j) * cos(theta) + float(i) * sin(theta)) + 200;
			float fx = (float(-j) * sin(theta) + float(i) * cos(theta)) + 200;
			x = (int)(fx + 1);
			y = (int)(fy + 1);
			imgin2->imageData[(x * imgin2->widthStep + 3 * y) + 0] = imgin->imageData[(i * imgin->widthStep + 3 * j) + 0];
			imgin2->imageData[(x * imgin2->widthStep + 3 * y) + 1] = imgin->imageData[(i * imgin->widthStep + 3 * j) + 1];
			imgin2->imageData[(x * imgin2->widthStep + 3 * y) + 2] = imgin->imageData[(i * imgin->widthStep + 3 * j) + 2];
		}
	}

	return imgin2;
}

struct SymCmp
{
	bool operator()(const CvBox2D &x, const CvBox2D &y) const
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

set<CvBox2D, SymCmp> SearchProcess(IplImage *lpSrcImg)
{
	CvMemStorage *storage = cvCreateMemStorage();
	CvSeq *contours = NULL;
	CvSeq *result = NULL;
	double s, t;
	double dContourArea;
	CvBox2D End_Rage2D;
	CvPoint2D32f rectpoint[4];
	int index = 0;
	set<CvBox2D, SymCmp> lstRes;
	// 创建一个空序列用于存储轮廓角点
	CvSeq *squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);
	cvFindContours(lpSrcImg, storage, &contours, sizeof(CvContour),
		CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	// 遍历找到的每个轮廓contours
	while (contours)
	{
		//用指定精度逼近多边形曲线
		result = cvApproxPoly(contours, sizeof(CvContour), storage,
			CV_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (result->total == 4)
			{
				dContourArea = fabs(cvContourArea(result, CV_WHOLE_SEQ));
				if (dContourArea >= 500 && dContourArea <= 1000000 && cvCheckContourConvexity(result))
				{
					End_Rage2D = cvMinAreaRect2(contours);
					s = 0;
					for (index = 0; index < 5; index++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (index >= 2)
						{
							t = fabs(angle(
								(CvPoint *)cvGetSeqElem(result, index),
								(CvPoint *)cvGetSeqElem(result, index - 2),
								(CvPoint *)cvGetSeqElem(result, index - 1)));
							s = s > t ? s : t;
						}
					}

					// if 余弦值 足够小，可以认定角度为90度直角
					//cos0.1=83度，能较好的趋近直角
					if (s < 0.1)
					{
						// for (i = 0; i < 4; i++)
						// {
						//     cvSeqPush(squares, (CvPoint *)cvGetSeqElem(result, i));
						// }
						lstRes.insert(End_Rage2D);
					}
				}
			}
		}
		// 继续查找下一个轮廓
		contours = contours->h_next;
	}
	return std::move(lstRes);
}

typedef struct stBox
{
	CvPoint pt[4];
	CvBox2D box;
	bool isRect;
} Box;

#define MAX_P 40.0

bool isEqu(float x1, float x2)
{
	return (abs(x1 - x2) <= MAX_P);
}

struct SymBoxCmp
{
	bool operator()(const Box &x, const Box &y) const
	{
		if (abs(x.box.center.x - y.box.center.x) <= MAX_P && abs(x.box.center.y - y.box.center.y) <= MAX_P)
		{
			if (abs(x.box.size.width - y.box.size.width) <= MAX_P && abs(x.box.size.height - y.box.size.height) <= MAX_P)
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
		if (abs(x.box.center.x - y.box.center.x) <= MAX_P && abs(x.box.center.y - y.box.center.y) <= MAX_P)
		{
			if (abs(x.box.size.width - y.box.size.width) <= MAX_P && abs(x.box.size.height - y.box.size.height) <= MAX_P)
			{
				return false;
			}
		}
		return true;
	}
};

set<Box, SymUBoxCmp> SearchProcess_v2(IplImage *lpSrcImg)
{
	CvMemStorage *storage = cvCreateMemStorage();
	CvSeq *contours = NULL;
	CvSeq *result = NULL;
	double s, t;
	double dContourArea;
	CvBox2D End_Rage2D;
	CvPoint2D32f rectpoint[4];
	int index = 0;
	set<Box, SymUBoxCmp> lstRes;
	// 创建一个空序列用于存储轮廓角点
	CvSeq *squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);
	cvFindContours(lpSrcImg, storage, &contours, sizeof(CvContour),
		CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
	int iCount = 0;
	cvSaveImage("c.bmp", lpSrcImg);
	// 遍历找到的每个轮廓contours
	while (contours)
	{
		//用指定精度逼近多边形曲线
		result = cvApproxPoly(contours, sizeof(CvContour), storage,
			CV_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);
		if (NULL != result)
		{
			if (result->total == 4)
			{
				dContourArea = fabs(cvContourArea(result, CV_WHOLE_SEQ));
				if (dContourArea >= 500 && dContourArea <= 1000000 && cvCheckContourConvexity(result))
				{
					End_Rage2D = cvMinAreaRect2(contours);
					s = 0;
					Box box;
					box.box = End_Rage2D;
					for (index = 2; index < 5; index++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (index >= 2)
						{
							t = fabs(angle(
								(CvPoint *)cvGetSeqElem(result, index % 4),
								(CvPoint *)cvGetSeqElem(result, index - 2),
								(CvPoint *)cvGetSeqElem(result, index - 1)));
							s = s > t ? s : t;
						}
					}

					// if 余弦值 足够小，可以认定角度为90度直角
					//cos0.1=83度，能较好的趋近直角
					CvPoint *tp;
					for (int i = 0; i < 4; i++)
					{
						tp = (CvPoint *)cvGetSeqElem(result, i);
						box.pt[i].x = tp->x;
						box.pt[i].y = tp->y;
						//  cvSeqPush(squares, (CvPoint *)cvGetSeqElem(result, i));
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
		// 继续查找下一个轮廓
		contours = contours->h_next;
	}

	cvReleaseMemStorage(&storage);

	return std::move(lstRes);
}

IplImage *protc2(IplImage *imgin, IplImage *imgin2, float theta);
int process(IplImage *lpImg, IplImage *lpTargetImg, int argc, char *argv[])
{
	set<CvBox2D, SymCmp> lstRes = SearchProcess(lpImg);

	set<CvBox2D, SymCmp>::iterator it;

	CvBox2D box;
	for (it = lstRes.begin(); it != lstRes.end(); it++)
	{
		box = *it;
	}
	it = lstRes.begin();
	for (int i = 0; i < argc && it != lstRes.end(); i++)
	{
		printf("%s\n", argv[i]);
		IplImage *lpSrcImg = cvLoadImage(argv[i], 1);
		if (lpSrcImg != NULL)
		{
			box = *it;
			it++;

			IplImage *temp = cvCreateImage(cvSize((int)box.size.width, (int)box.size.height), lpSrcImg->depth, lpSrcImg->nChannels);

			cvResize(lpSrcImg, temp, CV_INTER_AREA);
			temp = lpSrcImg;
			CvPoint2D32f srcTri[4], dstTri[4];
			CvMat *warp_mat = cvCreateMat(3, 3, CV_32FC1);
			//cvBoxPoints(box, dstTri); //计算二维盒子顶点

			//计算矩阵仿射变换
			if (box.size.width > box.size.height)
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1; //缩小一个像素
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1;
				srcTri[3].y = temp->height - 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1;
			}
			else
			{
				srcTri[2].x = 0;
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1; //缩小一个像素
				srcTri[3].y = 0;
				srcTri[0].x = temp->width - 1;
				srcTri[0].y = temp->height - 1;
				srcTri[1].x = 0; //bot right
				srcTri[1].y = temp->height - 1;
			}
			//改变目标图像大小
			// dstTri[0].x =893;// temp->width * 0.05;
			// dstTri[0].y =598;// temp->height * 0.33;
			// dstTri[1].x =893;// temp->width * 0.9;
			// dstTri[1].y =477;// temp->height * 0.25;
			// dstTri[2].x =1107;// temp->width * 0.2;
			// dstTri[2].y =477; temp->height * 0.7;
			// dstTri[3].x =1107;// temp->width * 0.8;
			// dstTri[3].y =598;// temp->height * 0.9;

			cvGetPerspectiveTransform(dstTri, srcTri, warp_mat);                                     //由三对点计算仿射变换
			cvWarpPerspective(temp, lpTargetImg, warp_mat, CV_INTER_LINEAR | (CV_WARP_INVERSE_MAP)); //对图像做仿射变换
		}
	}

	return lstRes.size();
}

int process_v2(IplImage *lpImg, IplImage *lpTargetImg, int argc, char *argv[])
{


	set<Box, SymBoxCmp> lstRes;
	set<Box, SymUBoxCmp> setURes = SearchProcess_v2(lpImg);
	set<Box, SymUBoxCmp>::iterator itu;
	CvPoint2D32f srcTri[4], dstTri[4];
	IplImage *temp;
	CvPoint p[4];
	CvMat *warp_mat;
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

			warp_mat = cvCreateMat(3, 3, CV_32FC1);

			//排序
			CvPoint pt;
			for (int j = 0; j < 4; j++) /* 气泡法要排序n次*/
			{
				for (int i = 0; i < 4 - j; i++) /* 值比较大的元素沉下去后，只把剩下的元素中的最大值再沉下去就可以啦 */
				{
					if (box.pt[i].x > box.pt[i + 1].x) /* 把值比较大的元素沉到底 */
					{
						pt = box.pt[i];
						box.pt[i] = box.pt[i + 1];
						box.pt[i + 1] = pt;
					}
				}
			}
			printf("after centerInfo:[%f,%f]:[%f,%f]\n", box.box.center.x, box.box.center.y, box.box.size.width, box.box.size.height);

			//找p0点
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

			for (int i = 0; i < 4; i++)
			{
				CvPoint pt = cvPointFrom32f(dstTri[i]);

				// cvCircle(lpTargetImg, pt, 4 + 4 * i, cvScalar(0, 0, 255), 1, 8, 0);
			}
			//计算矩阵仿射变换
			if (box.box.size.width > box.box.size.height)
			{
				srcTri[1].x = 0;
				srcTri[1].y = 0;
				srcTri[2].x = temp->width - 1; //缩小一个像素
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
				srcTri[2].x = temp->width - 1 + 1; //缩小一个像素
				srcTri[2].y = 0;
				srcTri[3].x = temp->width - 1 + 1;
				srcTri[3].y = temp->height - 1 + 1;
				srcTri[0].x = 0; //bot right
				srcTri[0].y = temp->height - 1 + 1;

				// srcTri[2].x = 0;
				// srcTri[2].y = 0;
				// srcTri[3].x = temp->width - 1; //缩小一个像素
				// srcTri[3].y = 0;
				// srcTri[0].x = temp->width - 1;
				// srcTri[0].y = temp->height - 1;
				// srcTri[1].x = 0; //bot right
				// srcTri[1].y = temp->height - 1;
			}

			cvGetPerspectiveTransform(dstTri, srcTri, warp_mat);                                     //由三对点计算仿射变换
			cvWarpPerspective(temp, lpTargetImg, warp_mat, CV_INTER_LINEAR | (CV_WARP_INVERSE_MAP)); //对图像做仿射变换
		}
	}
	return iCount;
}

int main(int argc, char** args)
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
	IplImage *gray = cvCreateImage(cvGetSize(lpTargetImg), 8, 1);
	cvCvtColor(lpTargetImg, gray, CV_RGB2GRAY);
	IplImage *lpCannyImg = cvCreateImage(cvGetSize(lpTargetImg), 8, 1);
	IplImage *tmp = cvCreateImage(cvGetSize(lpTargetImg), 8, 1);
	IplImage *lpDilateImg = cvCreateImage(cvGetSize(lpTargetImg), 8, 1);
	cvSmooth(gray, tmp, CV_BILATERAL, 3, 3, 0, 0); //高斯滤波
	cvSaveImage("tmp.bmp", tmp);
	printf("save\n");
	if (NULL != lpCannyImg && NULL != lpDilateImg)
	{
		cvCanny(lpTargetImg, lpCannyImg, 0.5, 20, 3);
		cvSaveImage("canny1.bmp", lpCannyImg);
		cvDilate(lpCannyImg, lpDilateImg, 0, 1);
		cvErode(lpDilateImg, lpDilateImg, 0, 1);
		cvSaveImage("lpDilateImg.bmp", lpDilateImg);

		IplImage *pGrayImage = cvCreateImage(cvGetSize(lpTargetImg), IPL_DEPTH_8U, 1);
		IplImage *pGrayEqualizeImage = cvCreateImage(cvGetSize(lpTargetImg), IPL_DEPTH_8U, 1);
		cvSmooth(lpTargetImg, lpTargetImg, CV_GAUSSIAN, 3, 3, 0, 0); //高斯滤波
		cvCvtColor(lpTargetImg, pGrayImage, CV_BGR2GRAY);
		cvSaveImage("pGrayImage.bmp", pGrayImage);
		//cvEqualizeHist(pGrayImage, pGrayEqualizeImage);
		cvSaveImage("pGrayEqualizeImage.bmp", pGrayEqualizeImage);

		cvCanny(pGrayEqualizeImage, lpCannyImg, 0.5, 20, 3);
		cvSaveImage("canny2.bmp", lpCannyImg);
	}
	//查找目标区域
	IplImage *lpOutImg = cvCloneImage(lpTargetImg);
	int iCunt = 0;
	if (NULL != lpOutImg)
	{
		// iCunt = process(lpDilateImg, lpOutImg, argc - 3, &args[3]);
		iCunt = process_v2(lpDilateImg, lpOutImg, argc - 3, &args[3]);

		cvSaveImage(OutputPath, lpOutImg);
	}

	//输出个数
	printf("%d\n", iCunt);
	//释放资源
	cvReleaseImage(&lpOutImg);
	cvReleaseImage(&lpTargetImg);
	cvReleaseImage(&lpDilateImg);
	cvReleaseImage(&lpCannyImg);

	return 0;
}

int main1(int argc, char **argv)
{

	IplImage *frame = NULL;
	IplImage *laplace = NULL;    //拉普拉斯转换后的单通道图像
	IplImage *ColorImage = NULL; //用于显示最终转换后的图像

	frame = cvLoadImage(argv[1], 1);
	IplImage *timg = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
	cvSmooth(frame, timg, CV_BILATERAL, 3, 3, 0, 0); //双边滤波
	IplImage *panel[3];                              //三个通道

	if (!laplace) //创建需要创建的变量
	{
		for (int i = 0; i < 3; i++)
			panel[i] = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
		laplace = cvCreateImage(cvGetSize(frame), IPL_DEPTH_16S, 1);
		ColorImage = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
	}
	cvSplit(timg, panel[0], panel[1], panel[2], NULL); //分割图像到单通道
	for (int i = 0; i < 3; i++)
	{
		// cvLaplace(panel[i], laplace, 3);            //每一个通道做拉普拉斯变换
		cvConvertScaleAbs(laplace, panel[i], 1, 0); //做类型转换，转换到8U
	}

	cvDilate(panel[0], panel[0], 0, 2);
	cvDilate(panel[1], panel[1], 0, 2);
	cvDilate(panel[2], panel[2], 0, 2);
	cvMerge(panel[0], panel[1], panel[2], NULL, ColorImage); //合并图像通道
	ColorImage->origin = 0;                                  //0--正面对摄像头；1--倒过来对摄像头
	cvShowImage("Laplace", ColorImage);
	///cvWaitKey(0);

	return 0;
}

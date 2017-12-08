// HoughChange.cpp : 定义控制台应用程序的入口点。
//
/*
#include "stdafx.h"

#include<opencv2\opencv.hpp>
#include <iostream>

#include <math.h>


using namespace cv;
using namespace std;

int main(void)
{
IplImage *src = cvLoadImage("F:\\vs数字图像处理典型算法\\Hough变换\\翻牌状态.jpg",0);
if (src)
{
IplImage *dst = cvCreateImage(cvGetSize(src),8,1);
IplImage *color_dst = cvCreateImage(cvGetSize(src),8,3);
CvMemStorage *storage = cvCreateMemStorage();
CvSeq *lines = 0;

cvCanny(src,dst,50,100,3);

cvCvtColor(dst,color_dst,CV_GRAY2BGR);
#if 0
lines = cvHoughLines2(dst,storage,CV_HOUGH_STANDARD,1,CV_PI/180,150,0,0);

for (int i=0;i<lines->total;i++)
{
float *line = (float *)cvGetSeqElem(lines,i);
float rho = line[0];  //像素
float theta = line[1];  //弧度
CvPoint pt1,pt2;
double a = cos(theta);
double b = sin(theta);
if (fabs(a)<0.001)
{
pt1.x = pt2.x = cvRound(rho);
pt1.y = 0;
pt2.y = color_dst->height;
}
else if (fabs(b)<0.001)
{
pt1.y = pt2.y = cvRound(rho);
pt1.x = 0;
pt2.x = color_dst->width;
}
else
{
pt1.x = 0;
pt1.y = cvRound(rho/b);
pt2.x = cvRound(rho/a);
pt2.y = 0;
}

cvLine(color_dst,pt1,pt2,CV_RGB(255,0,0),1,8);
}
#else
lines = cvHoughLines2(dst,storage,CV_HOUGH_PROBABILISTIC,1,CV_PI/180,60,30,5);
for (int i=0;i<lines->total;i++)
{
CvPoint *line = (CvPoint *)cvGetSeqElem(lines,i);
cvLine(color_dst,line[0],line[1],CV_RGB(255,0,0),1,CV_AA);
}
#endif
cvNamedWindow("Source");
cvShowImage("Source",src);

cvNamedWindow("Hough");
cvShowImage("Hough",color_dst);

cvWaitKey(0);

cvReleaseImage(&src);
cvReleaseImage(&dst);
cvReleaseImage(&color_dst);
cvReleaseMemStorage(&storage);

cvDestroyAllWindows();

return 1;
}
}  */

//#include <opencv2/opencv.hpp>
#include <stack>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <iostream>
#include <math.h>
#include <set>
using namespace cv;
using namespace std;
//angle函数用来返回（两个向量之间找到角度的余弦值）
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
		cvPolyLine(cpy, &rect, &count, 1, 1, CV_RGB(0, 0, 255), 2, CV_AA, 0);
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

int main(int argc, char **args)
{
	IplImage *src = cvLoadImage(args[1], 1); //2222222
	if (src)
	{
		set<int> sets;
		int rgb;
		set<int>::iterator ite;
		CvScalar cs, hs; //声明像素变量
		CvScalar cs2;	//声明像素变量
						 // printf("count = %d\n", sets.size());
						 // for (int i = 0; i < src->height; i++)
						 // {
						 // 	for (int j = 0; j < src->width; j++)
						 // 	{
						 // 		cs = cvGet2D(src, i, j); //获取像素
						 // 		rgb = 0;
						 // 		rgb = (int)cs.val[0] << 24 | (int)cs.val[1] << 16 | (int)cs.val[2];

		// 		ite = sets.find(rgb);
		// 		if (ite == sets.end())
		// 		{
		// 			sets.insert(rgb);
		// 		}
		// 	}
		// }

		// printf("count = %d\n", sets.size());

		IplImage *dst = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *dl = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *dst_color = cvCreateImage(cvGetSize(src), 8, 3);
		CvMemStorage *storage = cvCreateMemStorage();
		CvSeq *lines = 0;
		IplImage *gray = cvCreateImage(cvGetSize(src), 8, 1);
		int d = 0;

		IplImage *gray2 = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *hsv = cvCreateImage(cvGetSize(src), 8, 3);
		cvCvtColor(src, gray, CV_RGB2GRAY);
		cvCvtColor(src, hsv, CV_BGR2HSV);

		// for (int i = 0; i < src->height; i++)
		// {
		// 	for (int j = 0; j < src->width; j++)
		// 	{

		// 		cs = cvGet2D(src, i, j);   //获取像素
		// 		cs2 = cvGet2D(gray, i, j); //获取像素
		// 		d = MAX(cs.val[0], MAX(cs.val[1], cs.val[2]));
		// 		cs2.val[0] = d;
		// 		cvSet2D(gray, i, j, cs2); //将改变的像素保存到图片中
		// 	}
		// }

		cvNamedWindow("gray");
		cvShowImage("gray", gray);
		cvNamedWindow("hsv");
		cvShowImage("hsv", hsv);
		IplImage *h_plane = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *s_plane = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *v_plane = cvCreateImage(cvGetSize(src), 8, 1);
		IplImage *planes[] = {h_plane, s_plane};
		cvCvtPixToPlane(hsv, h_plane, s_plane, v_plane, 0);
		cvShowImage("h_plane", h_plane);
		cvShowImage("s_plane", s_plane);
		cvShowImage("v_plane", v_plane);
		CvSeq *contours = NULL;
		CvSeq *result = NULL;
		double s, t;
		// 创建一个空序列用于存储轮廓角点
		CvSeq *squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);

		for (int qq = 0; qq < 255; qq++)
		{
			if (104 == qq)
			{
				int a = 0;
			}
			else
			{
				//	continue;
			}

			// for (int i = 0; i < gray->height; i++)
			// {
			// 	for (int j = 0; j < gray->width; j++)
			// 	{
			// 		hs = cvGet2D(hsv, i, j);
			// 		cs = cvGet2D(gray, i, j); //获取像素
			// 		if (cs.val[0] != qq)
			// 		{
			// 			cs.val[0] = 0;
			// 		}
			// 		else
			// 		{
			// 			cs.val[0] = 255;
			// 		}
			// 		cvSet2D(gray2, i, j, cs); //将改变的像素保存到图片中
			// 	}
			// }
			//IplImage *out = cvCreateImage(cvSize(cvGetSize(dst).width, cvGetSize(dst).height), IPL_DEPTH_8U, 1);

					cvSmooth(gray, dl, CV_BILATERAL, 1, 150, 240, 480); //双边滤波
		cvShowImage("CV_BILATERAL", dl);
			//cvSmooth(gray, out, CV_GAUSSIAN, 3, 3, 0, 0);
			cvCanny(dl, dst, 50, 150, 3);
			//cvSmooth(gray, dst, CV_GAUSSIAN, 3, 3, 0, 0);
			//cvShowImage("out", dst);
			cvSaveImage("canny.bmp", dst);
			//cvCvtColor(dst, dst_color, CV_GRAY2RGB);

			lines = cvHoughLines2(dst, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI / 360, 60, 100, 10);
			for (int i = 0; i < lines->total; i++)
			{
				CvPoint *line = (CvPoint *)cvGetSeqElem(lines, i);
				cvLine(dst_color, line[0], line[1], CV_RGB(255, 0, 0), 1, CV_AA);
			}

			// 找到所有轮廓并且存储在序列中
			cvFindContours(dst, storage, &contours, sizeof(CvContour),
						   CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

			// 遍历找到的每个轮廓contours
			while (contours)
			{
				//用指定精度逼近多边形曲线
				result = cvApproxPoly(contours, sizeof(CvContour), storage,
									  CV_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.01, 0);

				if (result->total == 4 &&
					fabs(cvContourArea(result, CV_WHOLE_SEQ)) > 500 &&
					fabs(cvContourArea(result, CV_WHOLE_SEQ)) < 1000000 &&
					cvCheckContourConvexity(result))
				{
					s = 0;
					int i = 0;
					for (i = 0; i < 5; i++)
					{
						// find minimum angle between joint edges (maximum of cosine)
						if (i >= 2)
						{
							t = fabs(angle(
								(CvPoint *)cvGetSeqElem(result, i),
								(CvPoint *)cvGetSeqElem(result, i - 2),
								(CvPoint *)cvGetSeqElem(result, i - 1)));
							s = s > t ? s : t;
						}
					}

					// if 余弦值 足够小，可以认定角度为90度直角
					//cos0.1=83度，能较好的趋近直角
					if (s < 0.1)
					{
						char name[30];
						sprintf(name, "img%d.bmp", qq);
						cvSaveImage(name, dst);
						for (i = 0; i < 4; i++)
						{
							cvSeqPush(squares, (CvPoint *)cvGetSeqElem(result, i));
						}
					}
				}

				// 继续查找下一个轮廓
				contours = contours->h_next;
			}
		}
		drawSquares(src, squares);

		cvShowImage("befor", src);
		cvShowImage("Hough after", dst_color);
		cvWaitKey(0);
		cvReleaseImage(&src);
		cvReleaseImage(&dst);
		cvReleaseImage(&dst_color);
	}
	else
	{
		printf("load image error\n");
	}
	cvDestroyAllWindows();
	return 0;
}

//
//
//#include "cv.h"
//#include "highgui.h"
//
//#include<opencv2/core/core.hpp>
//#include<opencv2/highgui/highgui.hpp>
//#include<opencv2/imgproc/imgproc.hpp>
//#include<opencv2/opencv.hpp>
//#include <stdio.h>
//#include <math.h>
//#include <string.h>
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <io.h>
//#include <atlbase.h>
//
//
//
//
//using namespace std;
//using namespace cv;
//
//
//
//int thresh = 50;
//IplImage* img = NULL;
//IplImage* img0 = NULL;
//CvMemStorage* storage = NULL;
//const char * wndname = "正方形检测 demo";
//
////angle函数用来返回（两个向量之间找到角度的余弦值）
//double angle(CvPoint* pt1, CvPoint* pt2, CvPoint* pt0)
//{
//	double dx1 = pt1->x - pt0->x;
//	double dy1 = pt1->y - pt0->y;
//	double dx2 = pt2->x - pt0->x;
//	double dy2 = pt2->y - pt0->y;
//	return (dx1*dx2 + dy1*dy2) / sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
//}
//
//// 返回图像中找到的所有轮廓序列，并且序列存储在内存存储器中
//
//CvSeq* findSquares4(IplImage* img, CvMemStorage* storage)
//{
//	CvSeq* contours;
//	int i, c, l, N = 255;
//	CvSize sz = cvSize(img->width & -2, img->height & -2);
//
//	IplImage* timg = cvCloneImage(img);
//	IplImage* gray = cvCreateImage(sz, 8, 1);
//	IplImage* grayTMP = cvCreateImage(sz, 8, 1);
//	IplImage* pyr = cvCreateImage(cvSize(sz.width / 2, sz.height / 2), 8, 3);
//	IplImage* tgray;// = cvCreateImage(sz, 8, 1);;
//	CvSeq* result;
//	double s, t;
//	// 创建一个空序列用于存储轮廓角点
//	CvSeq* squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);
//
//	//cvSetImageROI(timg, cvRect(0, 0, sz.width, sz.height));
//	// 过滤噪音
//	//cvPyrDown(timg, pyr, 7);
//	//cvPyrUp(pyr, timg, 7);
//	tgray = cvCreateImage(sz, 8, 1);
//
//	// 红绿蓝3色分别尝试提取
//	//for (c = 0; c < 3; c++)
//	{
//		// 提取 the c-th color plane
//		//cvSetImageCOI(timg, c + 1);
//		cvCvtColor(timg, tgray, CV_RGB2GRAY);
//		//cvCopy(timg, tgray, 0);
//
//		// 尝试各种阈值提取得到的（N=11）
//		for (l = 1; l < N; l++)
//		{
//			// apply Canny. Take the upper threshold from slider
//			// Canny helps to catch squares with gradient shading
//			if (l == 0)
//			{
//				cvCanny(tgray, gray, 0.2, 0.75, 5);
//				//使用任意结构元素膨胀图像
//				cvDilate(gray, gray, 0, 1);
//			}
//			else
//			{
//				// apply threshold if l!=0:
//				cvThreshold(tgray, grayTMP, (l + 1) * 255 / N, 255, CV_THRESH_BINARY);
//			}
//
//			cvCanny(grayTMP, gray, 0.2, 0.75, 5);
//
//			// 找到所有轮廓并且存储在序列中
//			cvFindContours(gray, storage, &contours, sizeof(CvContour),
//				CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
//
//			// 遍历找到的每个轮廓contours
//			while (contours)
//			{
//				//用指定精度逼近多边形曲线
//				result = cvApproxPoly(contours, sizeof(CvContour), storage,
//					CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0);
//
//
//				if (result->total == 4 &&
//					fabs(cvContourArea(result, CV_WHOLE_SEQ)) > 500 &&
//					fabs(cvContourArea(result, CV_WHOLE_SEQ)) < 100000 &&
//					cvCheckContourConvexity(result))
//				{
//					s = 0;
//
//					for (i = 0; i < 5; i++)
//					{
//						// find minimum angle between joint edges (maximum of cosine)
//						if (i >= 2)
//						{
//							t = fabs(angle(
//								(CvPoint*)cvGetSeqElem(result, i),
//								(CvPoint*)cvGetSeqElem(result, i - 2),
//								(CvPoint*)cvGetSeqElem(result, i - 1)));
//							s = s > t ? s : t;
//						}
//					}
//
//					// if 余弦值 足够小，可以认定角度为90度直角
//					//cos0.1=83度，能较好的趋近直角
//					if (s < 0.1)
//						for (i = 0; i < 4; i++)
//							cvSeqPush(squares,
//								(CvPoint*)cvGetSeqElem(result, i));
//				}
//
//				// 继续查找下一个轮廓
//				contours = contours->h_next;
//			}
//		}
//	}
//	cvReleaseImage(&gray);
//	cvReleaseImage(&pyr);
//	cvReleaseImage(&tgray);
//	cvReleaseImage(&timg);
//
//	return squares;
//}
//
////drawSquares函数用来画出在图像中找到的所有正方形轮廓
//void drawSquares(IplImage* img, CvSeq* squares)
//{
//	CvSeqReader reader;
//	IplImage* cpy = cvCloneImage(img);
//	int i;
//	cvStartReadSeq(squares, &reader, 0);
//
//	// read 4 sequence elements at a time (all vertices of a square)
//	for (i = 0; i < squares->total; i += 4)
//	{
//		CvPoint pt[4], *rect = pt;
//		int count = 4;
//
//		// read 4 vertices
//		CV_READ_SEQ_ELEM(pt[0], reader);
//		CV_READ_SEQ_ELEM(pt[1], reader);
//		CV_READ_SEQ_ELEM(pt[2], reader);
//		CV_READ_SEQ_ELEM(pt[3], reader);
//
//		// draw the square as a closed polyline
//		cvPolyLine(cpy, &rect, &count, 1, 1, CV_RGB(0, 255, 0), 2, CV_AA, 0);
//	}
//
//	cvShowImage(wndname, cpy);
//	/*const char* filename = "111111111111111111111.jpg";
//	const CvArr* image = cpy;
//	int cvSaveImage("1111111111111111111111.jpg",image);*/
//	char* filename2 = "G:\\yy2.bmp"; //图像名
//	cvSaveImage(filename2, cpy);//把图像写入文件
//
//
//
//	//Mat mtx(cpy);
//
//	cvReleaseImage(&cpy);
//}
//
//
//char* names[] = { "G:\\YY.bmp", 0 };
//
//
//
//
//int main(int argc, char** argv)
//{
//	int i, c;
//	storage = cvCreateMemStorage(0);
//
//
//
//
//	for (i = 0; names[i] != 0; i++)
//	{
//		img0 = cvLoadImage(names[i], 1);
//		if (!img0)
//		{
//			cout << "不能载入" << names[i] << "继续下一张图片" << endl;
//			continue;
//		}
//		img = cvCloneImage(img0);
//		cvNamedWindow(wndname, 1);
//
//		// find and draw the squares
//		drawSquares(img, findSquares4(img, storage));
//
//		c = cvWaitKey(0);
//
//		cvReleaseImage(&img);
//		cvReleaseImage(&img0);
//
//		cvClearMemStorage(storage);
//
//		if ((char)c == 27)
//			break;
//	}
//
//	cvDestroyWindow(wndname);
//	return 0;
//}
//
//

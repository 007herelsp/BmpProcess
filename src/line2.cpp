//正方形检测源码
//载入数张包含各种形状的图片，检测出其中的正方形
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
using namespace std;

int thresh = 10;
IplImage *img = NULL;
IplImage *img0 = NULL;
CvMemStorage *storage = NULL;
const char *wndname = "正方形检测 demo";

//angle函数用来返回（两个向量之间找到角度的余弦值）
double angle(CvPoint *pt1, CvPoint *pt2, CvPoint *pt0)
{
    double dx1 = pt1->x - pt0->x;
    double dy1 = pt1->y - pt0->y;
    double dx2 = pt2->x - pt0->x;
    double dy2 = pt2->y - pt0->y;
    return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

IplImage *doPryDown(IplImage *in, int filter = CV_GAUSSIAN_5x5)
{
    //assert(in->width%2==0 && in->height%2==0);
    int width = (int)in->width / 2;
    int height = (int)in->height / 2;
    IplImage *out = cvCreateImage(cvSize(width, height), in->depth, in->nChannels);
    cvPyrDown(in, out, CV_GAUSSIAN_5x5); //filter=7 目前只支持CV_GAUSSIAN_5x5
    return (out);
}

IplImage *doCanny(IplImage *in, double lowThresh, double highThresh, double aperture)
{
    if (in->nChannels != 1)
    {
        return 0;
    }
    IplImage *out = cvCreateImage(cvSize(cvGetSize(in).width, cvGetSize(in).height), IPL_DEPTH_8U, 1);
    cvCanny(in, out, lowThresh, highThresh, aperture);
    return (out);
}

// 返回图像中找到的所有轮廓序列，并且序列存储在内存存储器中

CvSeq *findSquares4(IplImage *img, CvMemStorage *storage)
{
    CvSeq *contours;
    int i, c, l, N = 255;
    CvSize sz = cvSize(img->width & -2, img->height & -2);
    IplImage *img2 = cvCloneImage(img);
    CvSeq *lines = 0;
    CvMemStorage *storageLines = cvCreateMemStorage();
    // 创建一个空序列用于存储轮廓角点
    IplImage *timg = cvCloneImage(img);
    IplImage *gray = cvCreateImage(sz, 8, 1);
    IplImage *pyr = cvCreateImage(cvSize(sz.width / 2, sz.height / 2), 8, 3);
    IplImage *tgray;
    CvSeq *result;
    double s, t;
    // 创建一个空序列用于存储轮廓角点
    CvSeq *squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);

    // cvSetImageROI(timg, cvRect(0, 0, sz.width, sz.height));
    // 过滤噪音
    //cvDilate(timg, timg, 0, 2);
    //cvPyrDown(timg, pyr, 7);
    //cvPyrUp(pyr, timg, 7);
    //cvPyrDown(timg, pyr, 7);
    //cvPyrDown(timg, pyr, 7);

    // cvPyrUp(pyr, timg, 7);
    cvSmooth(timg, timg, CV_GAUSSIAN, 5, 5, 0, 0); //5x5
    // cvSmooth(timg,timg,CV_GAUSSIAN,5,5,0,0);//5x5
    tgray = cvCreateImage(sz, 8, 1);
    CvScalar cs, hs; //声明像素变量
    CvScalar cs2;    //声明像素变量
    // 红绿蓝3色分别尝试提取
    for (c = 0; c < 3; c++)
    {
        // 提取 the c-th color plane
        cvSetImageCOI(timg, c + 1);
        cvCopy(timg, tgray, 0);

        // 尝试各种阈值提取得到的（N=11）
        for (l = 0; l < 256; l++)
        {
            // apply Canny. Take the upper threshold from slider
            // Canny helps to catch squares with gradient shading
            if (l == 0)
            {
                cvCanny(tgray, gray, 0, 50, 5);
                //使用任意结构元素膨胀图像
                cvDilate(gray, gray, 0, 4);
                char name[20] = {0};
                sprintf(name, "canny%d.bmp", l);
                cvSaveImage(name, gray);
            }
            else
            {
                // apply threshold if l!=0:
                cvThreshold(tgray, gray, (l + 1) * 255 / N, 255, CV_THRESH_BINARY);
                // char name[20] = {0};
                // sprintf(name, "gray%d.bmp", l);
                // cvSaveImage(name, gray);
            }
            //Hough很慢
            // lines = cvHoughLines2(gray, storageLines, CV_HOUGH_PROBABILISTIC, 1, CV_PI / 180, 60, 100, 10);
            // for (int i = 0; i < lines->total; i++)
            // {
            //     CvPoint *line = (CvPoint *)cvGetSeqElem(lines, i);
            //     cvLine(img2, line[0], line[1], CV_RGB(255, 0, 0), 1, CV_AA);
            // }
            /////////////////

            // 找到所有轮廓并且存储在序列中
            cvFindContours(gray, storage, &contours, sizeof(CvContour),
                           CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

            // 遍历找到的每个轮廓contours
            while (contours)
            {
                //用指定精度逼近多边形曲线
                result = cvApproxPoly(contours, sizeof(CvContour), storage,
                                      CV_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.02, 0);

                if (result->total == 4 &&
                    fabs(cvContourArea(result, CV_WHOLE_SEQ)) > 500 &&
                    fabs(cvContourArea(result, CV_WHOLE_SEQ)) < 100000 &&
                    cvCheckContourConvexity(result))
                {
                    s = 0;

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
    }
    cvReleaseImage(&gray);
    cvReleaseImage(&pyr);
    cvReleaseImage(&tgray);
    cvReleaseImage(&timg);
    cvShowImage("lines", img2);
    return squares;
}

//drawSquares函数用来画出在图像中找到的所有正方形轮廓
void drawSquares(IplImage *img, CvSeq *squares)
{
    CvSeqReader reader;
    IplImage *cpy = cvCloneImage(img);
    int i;
    cvStartReadSeq(squares, &reader, 0);

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
        cvPolyLine(cpy, &rect, &count, 1, 1, CV_RGB(0, 255, 0), 2, CV_AA, 0);
    }

    cvShowImage(wndname, cpy);
    cvReleaseImage(&cpy);
}

int main(int argc, char **argv)
{
    int i, c;
    storage = cvCreateMemStorage(0);

    for (i = 1; i < argc; i++)
    {
        img0 = cvLoadImage(argv[i], 1);
        if (!img0)
        {
            cout << "load error" << argv[i] << "继续下一张图片" << endl;
            continue;
        }
        img = cvCloneImage(img0);
        cvNamedWindow(wndname, 1);

        // find and draw the squares
        drawSquares(img, findSquares4(img, storage));

        c = cvWaitKey(0);

        cvReleaseImage(&img);
        cvReleaseImage(&img0);

        cvClearMemStorage(storage);

        if ((char)c == 27)
            break;
    }

    cvDestroyWindow(wndname);
    return 0;
}
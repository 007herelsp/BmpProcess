#include "opencv2/core/types_c.h"
#include "opencv2/core/core_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include "opencv2/highgui/highgui.hpp"

#include "process.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <malloc.h>

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
    cvGetQuadrangleSubPix(temp, img_rotate, &M);
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

void DrawBox(CvBox2D box, IplImage *img)
{
    CvPoint2D32f point[4];
    int i;
    for (i = 0; i < 4; i++)
    {
        point[i].x = 0;
        point[i].y = 0;
    }
    cvBoxPoints(box, point); //计算二维盒子顶点
    CvPoint pt[4];
    for (i = 0; i < 4; i++)
    {
        pt[i].x = (int)point[i].x;
        pt[i].y = (int)point[i].y;
    }
    cvLine(img, pt[0], pt[1], CV_RGB(255, 0, 0), 2, 8, 0);
    cvLine(img, pt[1], pt[2], CV_RGB(255, 0, 0), 2, 8, 0);
    cvLine(img, pt[2], pt[3], CV_RGB(255, 0, 0), 2, 8, 0);
    cvLine(img, pt[3], pt[0], CV_RGB(255, 0, 0), 2, 8, 0);

    printf("ok\n");
}

int main(int argc, char **args)
{

//////////
#if 0
    IplImage *srcb;
    /* the first command line parameter must be image file name */
    srcb = cvLoadImage(args[1], 1);
    IplImage *dst = cvLoadImage(args[2], 1);
    int angle1 = 45;
    CvSize czSize; //目标图像尺寸
    if (NULL != srcb)
    {

        czSize.width = 100;
        czSize.height = 100;
        //创建图像并缩放
        IplImage *pDstImage = cvCreateImage(czSize, srcb->depth, srcb->nChannels);
        cvResize(srcb, pDstImage, CV_INTER_AREA);
        cvShowImage("src", srcb);
        cvSaveImage("srcb.bmp", srcb); //把图像写入文件
        cvSaveImage("dst.bmp", dst);   //把图像写入文件
        IplImage *dst2 = rotateImage2(pDstImage, dst, angle1);
        //IplImage *dst2 = protc(pDstImage, dst, angle1);
        cvShowImage("dst2", dst2);
        cvSaveImage("dst2.bmp", dst2); //把图像写入文件
        cvShowImage("pDstImage", pDstImage);
        cvSaveImage("pDstImage.bmp", pDstImage); //把图像写入文件
    }
    cvWaitKey(0);
    return 0;
#endif
    //////////

    IplImage *src = cvLoadImage(args[1], 1); //2222222
    if (src)
    {
        int rgb;
        CvScalar cs, hs; //声明像素变量
        CvScalar cs2;    //声明像素变量

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

        cvShowImage("gray", gray);
        cvShowImage("hsv", hsv);
        IplImage *h_plane = cvCreateImage(cvGetSize(src), 8, 1);
        IplImage *s_plane = cvCreateImage(cvGetSize(src), 8, 1);
        IplImage *v_plane = cvCreateImage(cvGetSize(src), 8, 1);
        IplImage *planes[] = {h_plane, s_plane};
        // cvCvtPixToPlane(hsv, h_plane, s_plane, v_plane, 0);
        // cvShowImage("h_plane", h_plane);
        // cvShowImage("s_plane", s_plane);
        // cvShowImage("v_plane", v_plane);
        CvSeq *contours = NULL;
        CvSeq *result = NULL;
        double s, t;
        // 创建一个空序列用于存储轮廓角点
        CvSeq *squares = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);

        for (int qq = 0; qq < 1; qq++)
        {
            if (104 == qq)
            {
                int a = 0;
            }
            else
            {
                //	continue;
            }

            cvSmooth(gray, dl, CV_BILATERAL, 1, 150, 240, 480); //双边滤波
            cvShowImage("CV_BILATERAL", dl);
            //cvSmooth(gray, out, CV_GAUSSIAN, 3, 3, 0, 0);
            cvCanny(dl, dst, 50, 150, 3);
            //cvSmooth(gray, dst, CV_GAUSSIAN, 3, 3, 0, 0);
            //cvShowImage("out", dst);
            cvSaveImage("canny.bmp", dst);
            //cvCvtColor(dst, dst_color, CV_GRAY2RGB);

            IplImage *frame = NULL;
            IplImage *laplace = NULL;    //拉普拉斯转换后的单通道图像
            IplImage *ColorImage = NULL; //用于显示最终转换后的图像

            frame = cvLoadImage(args[1], 1);
            IplImage *timg = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
            //cvSmooth(frame, timg, CV_BILATERAL, 3, 3, 0, 0); //双边滤波
            timg = frame;
            IplImage *panel[3]; //三个通道
            cvCvtColor(frame, dst, CV_RGB2GRAY);
            cvCanny(dl, dst, 0.5, 20, 3);
            //cvSmooth(gray, dst, CV_GAUSSIAN, 3, 3, 0, 0);
            cvShowImage("cvCanny", dst);
            cvSaveImage("canny.bmp", dst);
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
                cvLaplace(panel[i], laplace, 9);            //每一个通道做拉普拉斯变换
                cvConvertScaleAbs(laplace, panel[i], 1, 0); //做类型转换，转换到8U
            }

            //cvDilate(panel[0], panel[0], 0, 2);
            //cvDilate(panel[1], panel[1], 0, 2);
            //cvDilate(panel[2], panel[2], 0, 2);
            cvMerge(panel[0], panel[1], panel[2], NULL, ColorImage); //合并图像通道
            ColorImage->origin = 0;                                  //0--正面对摄像头；1--倒过来对摄像头
            cvShowImage("Laplace", ColorImage);
            cvCvtColor(ColorImage, dst, CV_RGB2GRAY);
            cvShowImage("dst", dst);
            //dst = ColorImage;

            // lines = cvHoughLines2(dst, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI / 360, 60, 100, 10);
            // for (int i = 0; i < lines->total; i++)
            // {
            // 	CvPoint *line = (CvPoint *)cvGetSeqElem(lines, i);
            // 	cvLine(dst_color, line[0], line[1], CV_RGB(255, 0, 0), 1, CV_AA);
            // }

            // 找到所有轮廓并且存储在序列中
            cvFindContours(dst, storage, &contours, sizeof(CvContour),
                           CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
            CvBox2D End_Rage2D;
            CvPoint2D32f rectpoint[4];
            // 遍历找到的每个轮廓contours
            IplImage *tttt = cvCloneImage(frame);
            while (contours)
            {
                End_Rage2D = cvMinAreaRect2(contours);
                cvBoxPoints(End_Rage2D, rectpoint);

                DrawBox(End_Rage2D, tttt);
                contours = contours->h_next;
                continue;
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
                    //if (s < 0.1)
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
            cvSaveImage("tttt.bmp",tttt);
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
    //cvDestroyAllWindows();
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
        cvLaplace(panel[i], laplace, 3);            //每一个通道做拉普拉斯变换
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <iostream>
#include <math.h>
using namespace cv;
CvBox2D Box2D;
int main(int argc, char *argv[])
{
    IplImage *src = cvLoadImage(argv[1], 1);

    IplImage *g_plane = cvCreateImage(cvGetSize(src), 8, 1);
    IplImage *b_plane = cvCreateImage(cvGetSize(src), 8, 1);
    IplImage *r_plane = cvCreateImage(cvGetSize(src), 8, 1);


    ///***************************************************
    int data1[16] =
        {
            0, 0, 0, 0,
            0, 0, 1, 0, ///图像的腐蚀
            0, 1, 1, 0,
            0, 0, 0, 0};

    int data[16] =
        {
            1, 0, 0, 1,
            0, 1, 1, 0, ///图像的腐蚀
            0, 1, 1, 0,
            1, 0, 0, 1};
    
    IplConvKernel *Element = cvCreateStructuringElementEx(4, 4, 2, 2, CV_SHAPE_CUSTOM, data);
    ;
    cvErode(src, src, Element, 2);
    // ***********************************************/
    IplImage *src_RGB = cvCreateImage(cvGetSize(src), 8, 3);
    IplImage *ZeroImage = cvCreateImage(cvGetSize(src), 8, 3);
    cvZero(ZeroImage);
    src_RGB = cvCloneImage(src); //copy Image
    IplImage *src_gray = cvCreateImage(cvGetSize(src), 8, 1);
    IplImage *Canny_Image = cvCreateImage(cvGetSize(src), 8, 1);
    IplImage *Threshold = cvCreateImage(cvGetSize(src), 8, 1);
    cvCvtColor(src, src_gray, CV_RGB2GRAY);
    cvShowImage("GRAY", src_gray); //显示灰色图像

    //转为二值化图像
    //cvThreshold(src_gray, Threshold, 120, 255, CV_THRESH_BINARY);

    // cvShowImage("Threshold", Threshold);
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvSeq *Image_Seq;
    //使用canny算法提取图像的轮廓
    cvCanny(src_gray, Canny_Image, 125, 255, 3);
    cvShowImage("Canny_Image", Canny_Image);
    //使用cvfindcontour函数提取轮廓
    cvFindContours(Canny_Image, storage, &Image_Seq, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);

    cvDrawContours(src_RGB, Image_Seq, CV_RGB(200, 65, 45), CV_RGB(100, 165, 45), 2, 2, 8);
    cvDrawContours(ZeroImage, Image_Seq, CV_RGB(200, 65, 45), CV_RGB(255, 2555, 255), 2, 2, 8);

    CvSeq *Cal_Contours = Image_Seq;
    CvPoint2D32f Array[4];
    for (; Cal_Contours != 0; Cal_Contours = Cal_Contours->h_next)
    { //show  Angle to Image
        CvSeq *InterCon = Cal_Contours->v_next;
        for (; InterCon != 0; InterCon = InterCon->h_next)
        {
            Box2D = cvMinAreaRect2(Cal_Contours);

            CvBox2D FitBox;
            FitBox = cvMinAreaRect2(Cal_Contours);
            //  cvFitEllipse2(Cal_Contours);
            // printf("Cal_Contours of angle %f",Box2D.angle);
            CvBox2D ellipse = cvFitEllipse2(Cal_Contours);
            cvBoxPoints(FitBox, Array);
            cvEllipseBox(src_RGB, ellipse, CV_RGB(255, 255, 0)); //在图上画椭圆
            printf("%f %f ", Array[0].x, Array[0].y);
            printf("\n");
            cvLine(ZeroImage, cvPoint(Array[0].x, Array[0].y), cvPoint(Array[1].x, Array[1].y), CV_RGB(114, 45, 255), 2, 8);
            cvLine(ZeroImage, cvPoint(Array[1].x, Array[1].y), cvPoint(Array[2].x, Array[2].y), CV_RGB(114, 45, 255), 2, 8);
            cvLine(ZeroImage, cvPoint(Array[2].x, Array[2].y), cvPoint(Array[3].x, Array[3].y), CV_RGB(114, 45, 255), 2, 8);
            cvLine(ZeroImage, cvPoint(Array[3].x, Array[3].y), cvPoint(Array[0].x, Array[0].y), CV_RGB(114, 45, 255), 2, 8);
            //在图上画圆
            //cvCircle(ZeroImage,cvPoint((int)Box2D.center.x,(int)Box2D.center.y) ,50,CV_RGB(45,89,56),2,8);
            ////draw RectAngle to Image
            // CvRect rect= cvBoundingRect(Cal_Contours);
            // cvRectangle(ZeroImage,cvPoint(rect.x,rect.y),cvPoint(rect.x+rect.width ,rect.y+rect.width),CV_RGB(114,45,255),2,8);
        }
    }

    //显示图像
    cvNamedWindow("ZeroImage", 0);
    cvShowImage("ZeroImage", ZeroImage);
    cvShowImage("src", Canny_Image);
    cvShowImage("src_RGB", src_RGB);
    cvWaitKey();
    return 0;
}
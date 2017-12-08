#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <iostream>
#include <math.h>
using namespace cv;
using namespace std;

int main(int argc, char **args)
{
    IplImage *src = cvLoadImage(args[1], 1); //2222222
    if (src)
    {
        int rgb;
        IplImage *gray = cvCreateImage(cvGetSize(src), 8, 1);
        CvScalar cs, cs2; //声明像素变量
        for (int i = 0; i < src->height; i++)
        {
            for (int j = 0; j < src->width; j++)
            {
                cs = cvGet2D(src, i, j);   //获取像素
                cs2 = cvGet2D(gray, i, j); //获取像素
                unsigned char d = MAX(cs.val[0], MAX(cs.val[1], cs.val[2]));
                cs2.val[0] = 255 - d;
                cvSet2D(gray, i, j, cs2); //将改变的像素保存到图片中
            }
        }
        // for (int i= 40; i < 100; i+=4)
        // {
        //     cs2 = cvGet2D(gray, 40, i); //获取像素
        //     cs2.val[0] = 255;
        //     cvSet2D(gray, 40, i, cs2); //将改变的像素保存到图片中
        // }

        cvShowImage("gray", gray);
        cvWaitKey(0);
        cvReleaseImage(&src);
        cvReleaseImage(&gray);
    }
    else
    {
        printf("load image error\n");
    }
    cvDestroyAllWindows();
    return 0;
}

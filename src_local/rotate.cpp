#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include "opencv2/highgui/highgui.hpp"

#include <math.h>

// IplImage *ImgRotate(IplImage *imgin, IplImage *imgin2, float theta)
// {
//     IplImage *imgout = NULL;
//     int oldWidth = imgin2->width;
//     int oldHeight = imgin2->height;

//     // 源图四个角的坐标（以图像中心为坐标系原点）
//     float fSrcX1, fSrcY1, fSrcX2, fSrcY2, fSrcX3, fSrcY3, fSrcX4, fSrcY4;
//     fSrcX1 = (float)(-(oldWidth - 1) / 2);
//     fSrcY1 = (float)((oldHeight - 1) / 2);
//     fSrcX2 = (float)((oldWidth - 1) / 2);
//     fSrcY2 = (float)((oldHeight - 1) / 2);
//     fSrcX3 = (float)(-(oldWidth - 1) / 2);
//     fSrcY3 = (float)(-(oldHeight - 1) / 2);
//     fSrcX4 = (float)((oldWidth - 1) / 2);
//     fSrcY4 = (float)(-(oldHeight - 1) / 2);

//     // 旋转后四个角的坐标（以图像中心为坐标系原点）
//     float fDstX1, fDstY1, fDstX2, fDstY2, fDstX3, fDstY3, fDstX4, fDstY4;
//     fDstX1 = cos(theta) * fSrcX1 + sin(theta) * fSrcY1;
//     fDstY1 = -sin(theta) * fSrcX1 + cos(theta) * fSrcY1;
//     fDstX2 = cos(theta) * fSrcX2 + sin(theta) * fSrcY2;
//     fDstY2 = -sin(theta) * fSrcX2 + cos(theta) * fSrcY2;
//     fDstX3 = cos(theta) * fSrcX3 + sin(theta) * fSrcY3;
//     fDstY3 = -sin(theta) * fSrcX3 + cos(theta) * fSrcY3;
//     fDstX4 = cos(theta) * fSrcX4 + sin(theta) * fSrcY4;
//     fDstY4 = -sin(theta) * fSrcX4 + cos(theta) * fSrcY4;

//     int newWidth = (max(fabs(fDstX4 - fDstX1), fabs(fDstX3 - fDstX2)) + 0.5);
//     int newHeight = (max(fabs(fDstY4 - fDstY1), fabs(fDstY3 - fDstY2)) + 0.5);

//     imgOut = cvCloneImage(imgin);

//     float dx = -0.5 * newWidth * cos(theta) - 0.5 * newHeight * sin(theta) + 0.5 * oldWidth;
//     float dy = 0.5 * newWidth * sin(theta) - 0.5 * newHeight * cos(theta) + 0.5 * oldHeight;

//     int x, y;
//     for (int i = 0; i < newHeight; i++)
//     {
//         for (int j = 0; j < newWidth; j++)
//         {
//             x = float(j) * cos(theta) + float(i) * sin(theta) + dx;
//             y = float(-j) * sin(theta) + float(i) * cos(theta) + dy;

//             if ((x < 0) || (x >= oldWidth) || (y < 0) || (y >= oldHeight))
//             {
//                 // if (imgIn.channels() == 3)
//                 // {
//                 //     imgOut.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0);
//                 // }
//                 // else if (imgIn.channels() == 1)
//                 // {
//                 //     imgOut.at<uchar>(i, j) = 0;
//                 // }
//             }
//             else
//             {
//                 // if (imgIn.channels() == 3)
//                 // {
//                 //     imgOut.at<cv::Vec3b>(i, j) = imgIn.at<cv::Vec3b>(y, x);
//                 // }
//                 // else if (imgIn.channels() == 1)
//                 // {
//                 //     imgOut.at<uchar>(i, j) = imgIn.at<uchar>(y, x);
//                 // }
//             }
//         }
//     }
// }

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

int main(int argc, char **argv)
{
    IplImage *src;
    /* the first command line parameter must be image file name */
    src = cvLoadImage(argv[1], 1);
    IplImage *dst = cvLoadImage(argv[2], 1);
    int angle = 45;
    CvSize czSize; //目标图像尺寸
    if (NULL != src)
    {

        czSize.width = 100;
        czSize.height = 100;
        //创建图像并缩放
        IplImage *pDstImage = cvCreateImage(czSize, src->depth, src->nChannels);
        cvResize(src, pDstImage, CV_INTER_AREA);
        cvShowImage("src", src);
        IplImage *dst2 = rotateImage2(pDstImage, dst, angle);
        cvShowImage("dst2", dst2);
        cvShowImage("pDstImage", pDstImage);
    }
    cvWaitKey(0);
    return 0;
}

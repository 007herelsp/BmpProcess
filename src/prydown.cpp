#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
using namespace std;

IplImage * doCanny(IplImage * in,double lowThresh,double highThresh,double aperture)
{
    if(in->nChannels !=1)
    {
        return 0;
    }
    IplImage *out =cvCreateImage(cvSize(cvGetSize(in).width,cvGetSize(in).height),IPL_DEPTH_8U,1);
    cvCanny(in,out,lowThresh,highThresh,aperture);
    return (out);

}
IplImage * doPryDown(IplImage * in,int filter=CV_GAUSSIAN_5x5)
{
    //assert(in->width%2==0 && in->height%2==0);
    int width =(int) in->width/2;
    int height = (int) in->height/2;
    IplImage * out=cvCreateImage(cvSize(width,height),in->depth,in->nChannels);
    cvPyrDown(in,out,CV_GAUSSIAN_5x5);//filter=7 目前只支持CV_GAUSSIAN_5x5
    return (out);
}
int main(int argc, char** args)
{
    cvNamedWindow("example-in");
    cvNamedWindow("example-out");
    IplImage * in=cvLoadImage(args[1],0);
    cvShowImage("example-in",in);
    IplImage * out=NULL;
    //做两次连续的缩放变换和边缘检测
    out = doPryDown(in,CV_GAUSSIAN_5x5);
    out = doPryDown(in,CV_GAUSSIAN_5x5);
    out = doCanny(in,50,150,3);

    cvShowImage("example-out",out);

    cvWaitKey(0);//key point
    cvReleaseImage(&in);
    cvReleaseImage(&out);
    cvDestroyWindow("example-in");    
    cvDestroyWindow("example-out");
}
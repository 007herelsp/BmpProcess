#include <opencv2/opencv.hpp>  
#include <opencv2/highgui/highgui.hpp>  
#include <opencv2/imgproc/imgproc.hpp>  
  
using namespace cv;  
  
int main(int argc, char ** args)  
{  
    Mat grad_x, grad_y,dst;  
    Mat src = imread(args[1]);  
  
    namedWindow("原图");  
    imshow("原图", src);  
  
    //x方向  
    Sobel(src, grad_x, CV_8U, 1, 0, 3, 1, 0, BORDER_DEFAULT);  
      
    namedWindow("x方向");  
    imshow("x方向", grad_x);  
  
    //y方向  
    Sobel(src, grad_y, CV_8U, 0, 1, 3, 1, 0, BORDER_DEFAULT);  
    namedWindow("y方向");  
    imshow("y方向", grad_y);  
  
    //合并的  
    addWeighted(grad_x, 0.5, grad_y, 0.5, 0, dst);  
    namedWindow("x+y");  
    imshow("x+y", dst);  
  
    waitKey(0);  
    return 0;  
}  
#include <opencv2/highgui/highgui.hpp>      
#include <opencv2/imgproc/imgproc.hpp>  
#include <iostream>  
  
using namespace cv;  
  
int main(int argc, char *argv[])  
{  
    Mat image = imread(argv[1], 1);  
    if (image.empty())  
    {  
        std::cout << "打开图片失败,请检查" << std::endl;  
        return -1;  
    }  
    imshow("befor", image);  
    Mat imageEnhance;  
    Mat kernel = (Mat_<float>(3, 3) << 0, -1, 0, 0, 5, 0, 0, -1, 0);  
    filter2D(image, imageEnhance, CV_8UC3, kernel);  
    imshow("after", imageEnhance); 
     imwrite("imageEnhance.bmp", imageEnhance);  
    waitKey();  
    return 0;  
}  
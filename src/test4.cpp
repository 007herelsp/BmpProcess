#include <opencv2/highgui/highgui.hpp>      
#include <opencv2/imgproc/imgproc.hpp>  
  
using namespace cv;  
  
int main(int argc, char *argv[])  
{  
    Mat image = imread(argv[1]);  
    Mat imageLog(image.size(), CV_32FC3);  
    for (int i = 0; i < image.rows; i++)  
    {  
        for (int j = 0; j < image.cols; j++)  
        {  
            imageLog.at<Vec3f>(i, j)[0] = log(1 + image.at<Vec3b>(i, j)[0]);  
            imageLog.at<Vec3f>(i, j)[1] = log(1 + image.at<Vec3b>(i, j)[1]);  
            imageLog.at<Vec3f>(i, j)[2] = log(1 + image.at<Vec3b>(i, j)[2]);  
        }  
    }  
    //归一化到0~255    
    normalize(imageLog, imageLog, 0, 255, CV_MINMAX);  
    //转换成8bit图像显示    
    convertScaleAbs(imageLog, imageLog);  
    imshow("Soure", image);  
    imshow("after", imageLog); 
    imwrite("imageLog.bmp", imageLog);  
    waitKey();    
    return 0;  
}
//  基于局部自适应阈值的图像二值化
//  Author：www.icvpr.com
//  Blog:   http://blog.csdn.net/icvpr

#include <iostream>
#include <opencv2/opencv.hpp>

int main(int argc, char **argv)
{
    cv::Mat image = cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    if (image.empty())
    {
        std::cout << "read image failure" << std::endl;
        return -1;
    }

    // 全局二值化
    int th = 100;
    cv::Mat global;
    cv::threshold(image, global, th, 255, CV_THRESH_BINARY);

    // 局部二值化

    int blockSize = 5;
    int constValue = 2;
    cv::Mat local;
    cv::adaptiveThreshold(image, local, 255, 0, CV_THRESH_BINARY, blockSize, constValue);
    cv::Mat Canny_Image;//= cv:createImage(image.shape, 8, 1);
    cv::Canny(local, Canny_Image, 125, 255, 3);
    cv::imshow("Canny_Image", Canny_Image);
    cv::imwrite("global.bmp", global);
    cv::imwrite("local.bmp", local);

    cv::imshow("globalThreshold", global);
    cv::imshow("localThreshold", local);
    cv::waitKey(0);

    return 0;
}
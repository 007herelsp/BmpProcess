#include "opencv2/highgui/highgui.hpp"
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
    imshow("before", image);
    Mat imageRGB[3];
    split(image, imageRGB);
    for (int i = 0; i < 3; i++)
    {
        equalizeHist(imageRGB[i], imageRGB[i]);
    }
    merge(imageRGB, 3, image);
    imshow("after", image);
    imwrite("imageHist.bmp", image);
    waitKey();
    return 0;
}
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

//-----------------------------------【命名空间声明部分】---------------------------------------
//     描述：包含程序所使用的命名空间
//-----------------------------------------------------------------------------------------------
using namespace std;
using namespace cv;

//-----------------------------------【main( )函数】--------------------------------------------
//     描述：控制台应用程序的入口函数，我们的程序从这里开始
//-----------------------------------------------------------------------------------------------
int main(int argc, char** args)
{

    //载入原图
    Mat image = imread(args[1]);

    //创建窗口

    //显示原图
    imshow("brfore", image);

    Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
    Mat out;
    dilate(image, out, element);

    //显示效果图
    imshow("after", out);
    imwrite("imageDilate.bmp", out);
    waitKey(0);

    return 0;
}
//-----------------------------------【头文件包含部分】---------------------------------------
//     描述：包含程序所依赖的头文件
//----------------------------------------------------------------------------------------------
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

    //显示原图
    imshow("before", image);

    //获取自定义核
    Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
    Mat out;

    //进行腐蚀操作
    erode(image, out, element);

    //显示效果图
    imshow("after", out);
    imwrite("imageErode.bmp", out);

    waitKey(0);

    return 0;
}
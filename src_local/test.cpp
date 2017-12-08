#include <iostream>
#include <stack>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// 8 邻域
static Point connects[8] = {Point(-1, -1), Point(0, -1), Point(1, -1), Point(1, 0), Point(1, 1), Point(0, 1), Point(-1, 1), Point(-1, 0)};

int main(int argc, char** args)
{
    // 原图
    Mat src = imread(args[1], 0);
    // 结果图
    Mat res = Mat::zeros(src.rows, src.cols, CV_8U);
    // 用于标记是否遍历过某点
    Mat flagMat;
    res.copyTo(flagMat);
    // 二值图像
    Mat bin;
    //threshold(src, bin, 80, 255, CV_THRESH_BINARY);
     src.copyTo(bin);;
     imshow("bin", bin);
    // 初始3个种子点
    stack<Point> seeds;
    seeds.push(Point(0, 0));
    seeds.push(Point(186, 166));
    seeds.push(Point(327, 43));
    res.at<uchar>(0, 0) = 255;
    res.at<uchar>(166, 186) = 255;
    res.at<uchar>(43, 327) = 255;

    while (!seeds.empty())
    {
        Point seed = seeds.top();
        seeds.pop();

        // 标记为已遍历过的点
        flagMat.at<uchar>(seed.y, seed.x) = 1;

        // 遍历8邻域
        for (size_t i = 0; i < 8; i++)
        {
            int tmpx = seed.x + connects[i].x;
            int tmpy = seed.y + connects[i].y;

            if (tmpx < 0 || tmpy < 0 || tmpx >= src.cols || tmpy >= src.rows)
                continue;
            // 前景点且没有被标记过的点
            if (bin.at<uchar>(tmpy, tmpx) != 0 && flagMat.at<uchar>(tmpy, tmpx) == 0)
            {
                res.at<uchar>(tmpy, tmpx) = 255;   // 生长
                flagMat.at<uchar>(tmpy, tmpx) = 1; // 标记
                seeds.push(Point(tmpx, tmpy));     // 种子压栈
            }
        }
    }

    imshow("RES", res);
    imwrite("res.jpg", res);
    waitKey(0);
    return 1;
}
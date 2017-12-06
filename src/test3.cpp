#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace cv;
using namespace std;
Mat src, src_gray;
Mat dst, detected_edges;
int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 200;
int ratio = 3;
int kernel_size = 3;
const char *window_name = "Edge Map";
static void CannyThreshold(int, void *)
{

    Canny(src_gray, detected_edges, lowThreshold, lowThreshold * ratio, kernel_size);
    dst = Scalar::all(0);
    src.copyTo(dst, detected_edges);
    imshow(window_name, dst);
}
int main(int, char **argv)
{

    src = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if (!src.data)
    {
        return -1;
    }
    dst.create(src.size(), src.type());
    cvtColor(src, src_gray, CV_BGR2GRAY);
    namedWindow(window_name, CV_WINDOW_AUTOSIZE);
    createTrackbar("Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold);
    CannyThreshold(0, 0);
    waitKey(0);
    return 0;
}
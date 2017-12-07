#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#define min(x, y) (x < y ? x : y)

#define R_THRESHHOLD 125
#define S_THRESHHOLD 60

//RGB+HSI颜色模型
void colorModel(IplImage *src, IplImage *dst)
{
    int step = NULL;
    int rows = src->height;
    int cols = src->width;
    for (int i = 0; i < rows; i++)
    {
        //uchar* dataS = src.ptr<uchar>(i);
        //uchar* dataD = dst.ptr<uchar>(i);
        uchar *dataS = (uchar *)src->imageData;
        uchar *dataD = (uchar *)dst->imageData;
        for (int j = 0; j < cols; j++)
        {
            step = i * src->widthStep + j * src->nChannels;
            ;
            float S;
            float b = dataS[step] / 255.0;
            float g = dataS[step + 1] / 255.0;
            float r = dataS[step + 2] / 255.0;
            float minRGB = min(min(r, g), b);
            float den = r + g + b;
            if (den == 0) //分母不能为0
                S = 0;
            else
                S = (1 - 3 * minRGB / den) * 100;
            if (dataS[step + 2] <= R_THRESHHOLD || dataS[step + 2] < 165)
            {
                dataD[step] = 0;
                dataD[step + 1] = 0;
                dataD[step + 2] = 0;
            }
            else if (dataS[step + 2] <= dataS[step + 1] || dataS[step + 1] <= dataS[step])
            {
                dataD[step] = 0;
                dataD[step + 1] = 0;
                dataD[step + 2] = 0;
            }
            else if (S <= (float)(S_THRESHHOLD * (255 - dataS[step + 2])) / R_THRESHHOLD)
            {
                dataD[step] = 0;
                dataD[step + 1] = 0;
                dataD[step + 2] = 0;
            }
            else
            {
                dataD[step] = dataS[step];
                dataD[step + 1] = dataS[step + 1];
                dataD[step + 2] = dataS[step + 2];
            }
        }
    }
}

//根据分割结果确定轮廓并填充
void fillSeg(IplImage *src, IplImage *tempdst)
{
    CvSeq *contour = NULL;
    CvMemStorage *storage = cvCreateMemStorage();
    //在二值图像中寻找轮廓,CV_CHAIN_APPROX_SIMPLE - 压缩水平、垂直和对角分割，即函数只保留末端的象素点
    cvFindContours(src, storage, &contour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
    cvZero(tempdst);
    for (contour; contour != 0; contour = contour->h_next)
    {
        //轮廓的方向影响面积的符号。因此函数也许会返回负的结果。应用函数 fabs() 得到面积的绝对值。
        double area = cvContourArea(contour, CV_WHOLE_SEQ);
        //计算整个轮廓或部分轮廓的面积
        if (fabs(area) < 10)
        {
            continue;
        }
        //	CvScalar color = CV_RGB( 255, 255, 255 );
        CvPoint *point = new CvPoint[contour->total];
        CvPoint *Point;

        //printf("图像分割contour->total\t%d\n",contour->total);
        for (int i = 0; i < contour->total; i++)
        {
            Point = (CvPoint *)cvGetSeqElem(contour, i);
            point[i].x = Point->x;
            point[i].y = Point->y;
        }
        int pts[1] = {contour->total};
        cvFillPoly(tempdst, &point, pts, 1, CV_RGB(255, 255, 255)); //填充多边形内部
    }
}
int main(int argc, char** args)
{
    IplImage *img = NULL;     //输入图像，8bit 3通道
    IplImage *colTemp = NULL; //颜色分割后(有内部空洞)的火焰图片
    IplImage *gray = NULL;    //灰度图
    IplImage *mask = NULL;    //二值图，用于复制图像的掩膜
    IplImage *dst = NULL;     //输出火焰疑似图像，8bit、3通道

    img = cvLoadImage(args[1]);                  //载入原始图片
    if(NULL == img)
    {
        return -1;
    }
    colTemp = cvCreateImage(cvGetSize(img), img->depth, img->nChannels); //经过颜色分割后(有内部空洞)的火焰图片
    gray = cvCreateImage(cvGetSize(img), img->depth, 1);
    mask = cvCreateImage(cvGetSize(img), img->depth, 1);
    dst = cvCreateImage(cvGetSize(img), img->depth, img->nChannels); //经过填补后的火焰图片
    cvZero(dst);
    colorModel(img, colTemp);
    cvCvtColor(colTemp, gray, CV_BGR2GRAY);
    //使用cvFindContours函数与cvFillPoly填充连通区内部空洞
    fillSeg(gray, mask);
    cvCopy(img, dst, mask);
    cvShowImage("before", img);
    cvShowImage("spilt", colTemp);
    cvShowImage("after", dst);
    cvShowImage("mask", mask);
    cvWaitKey();
}
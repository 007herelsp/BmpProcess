#include "vostype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

typedef struct stImgSize
{
    U32 cx;
    U32 cy;
}ImgSize, *lpImgSize;

void CreatGauss(double sigma, double **pdKernel, int *pnWidowSize);

void GaussianSmooth(ImgSize sz, LPBYTE pGray, LPBYTE pResult, double sigma);

void Grad(ImgSize sz, LPBYTE pGray, int *pGradX, int *pGradY, int *pMag);

void NonmaxSuppress(int *pMag, int *pGradX, int *pGradY, ImgSize sz, LPBYTE pNSRst);

void EstimateThreshold(int *pMag, ImgSize sz, int *pThrHigh, int *pThrLow, LPBYTE pGray,
                       double dRatHigh, double dRatLow);

void Hysteresis(int *pMag, ImgSize sz, double dRatLow, double dRatHigh, LPBYTE pResult);

void TraceEdge(int y, int x, int nThrLow, LPBYTE pResult, int *pMag, ImgSize sz);

void Canny(LPBYTE pGray, ImgSize sz, double sigma, double dRatLow,
           double dRatHigh, LPBYTE pResult);
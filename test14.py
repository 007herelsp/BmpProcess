# coding=UTF-8
import numpy
import argparse
import cv2
import numpy
import argparse
import cv2

image = cv2.imread('Target14.bmp')
cv2.imshow("Original", image)

gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
cv2.imshow("Gray", gray)

sobelx = cv2.Sobel(gray, cv2.CV_64F, 1, 0)
sobely = cv2.Sobel(gray, cv2.CV_64F, 0, 1)

sobelx = numpy.uint8(numpy.absolute(sobelx))
sobely = numpy.uint8(numpy.absolute(sobely))
sobelcombine = cv2.bitwise_or(sobelx,sobely)
#display two images in a figure
cv2.imshow("Edge detection by Sobel", numpy.hstack([gray,sobelx,sobely, sobelcombine]))

cv2.imwrite("1_edge_by_sobel.bmp", numpy.hstack([gray,sobelx,sobely, sobelcombine]))


if(cv2.waitKey(0)==27):
	cv2.destroyAllWindows()

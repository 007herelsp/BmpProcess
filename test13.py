 # coding=UTF-8
import numpy
import argparse
import cv2

import cv2
import numpy as np

image = cv2.imread('Target14.bmp')
cv2.imshow("Original", image)

gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
cv2.imshow("Gray", gray)

    #if don't use a floating point data type when computing
    #the gradient magnitude image, you will miss edges
lap = cv2.Laplacian(gray, cv2.CV_64F)
lap = numpy.uint8(numpy.absolute(lap))


    #display two images in a figure
cv2.imshow("Edge detection by Laplacaian", numpy.hstack([lap, gray]))

cv2.imwrite("1_edge_by_laplacian.bmp", numpy.hstack([gray, lap]))


if(cv2.waitKey(0)==27):
     cv2.destroyAllWindows()
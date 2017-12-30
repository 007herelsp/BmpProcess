#coding=utf-8
import cv2
import numpy as np
import matplotlib.pyplot as plt
img = cv2.imread('./images/Target17.bmp',0) #直接读为灰度图像


#9---滤波领域直径
#后面两个数字：空间高斯函数标准差，灰度值相似性标准差
blur = cv2.bilateralFilter(img,9,75,75)
cv2.imshow('gray', img)
cv2.imshow('Result', blur)
cv2.waitKey(0)
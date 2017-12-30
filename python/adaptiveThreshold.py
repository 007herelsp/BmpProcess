#coding=utf-8
import cv2
import matplotlib.pyplot as plt

img = cv2.imread('./images/Target17.bmp',0) #直接读为灰度图像
ret2,th2 = cv2.threshold(img,0,255,cv2.THRESH_BINARY+cv2.THRESH_OTSU)
cv2.imshow('gray1', th2)
ret,th1 = cv2.threshold(img,127,255,cv2.THRESH_BINARY)
th2 = cv2.adaptiveThreshold(img,255,cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY,11,2) #换行符号 \
th3 = cv2.adaptiveThreshold(img,255,cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY,11,5) #换行符号 \
images = [img,th1,th2,th3]
plt.figure()
for i in xrange(4):
    plt.subplot(2,2,i+1),plt.imshow(images[i],'gray')
plt.show()
cv2.waitKey(0)
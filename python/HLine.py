#coding=utf-8
import cv2
import numpy as np  
#OpenCV定义的结构元素  
kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(3, 3))  
img = cv2.imread("D:/BmpProcess/images/Target18.bmp")

#img = cv2.GaussianBlur(img,(3,3),0)
edges = cv2.Canny(img, 50, 150, apertureSize = 3)
cv2.imshow('Canny', edges)

#腐蚀图像  
eroded = cv2.erode(img,kernel)  
#膨胀图像  
dilated = cv2.dilate(eroded,kernel)  
#显示膨胀后的图像  
cv2.imshow("Dilated Image",dilated); 

#显示腐蚀后的图像  
cv2.imshow("Eroded Image",eroded);  
 
result = eroded
#将两幅图像相减获得边，第一个参数是膨胀后的图像，第二个参数是腐蚀后的图像  
#result = cv2.absdiff(dilated,eroded); 
#上面得到的结果是灰度图，将其二值化以便更清楚的观察结果  
#result = cv2.cvtColor(result, cv2.COLOR_BGR2GRAY) 
#retval, result = cv2.threshold(result,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU) 
cv2.imshow("result Image",result);  

#result = img.copy()

#经验参数
minLineLength = 5
maxLineGap = 5
threshold = 5
lines = cv2.HoughLinesP(edges,1,np.pi/180,threshold,minLineLength,maxLineGap)
for x1,y1,x2,y2 in lines[0]:
	cv2.line(img,(x1,y1),(x2,y2),(0,255,0),2)

cv2.imshow('Result', img)
cv2.waitKey(0)
cv2.destroyAllWindows()
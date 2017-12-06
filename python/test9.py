#coding=utf-8 
import cv2 
import numpy as np
 
img = cv2.imread("Target14.bmp")    #载入图像
h, w = img.shape[:2]      #获取图像的高和宽 
cv2.imshow("Origin", img)     #显示原始图像
 
blured = cv2.blur(img,(5,5))    #进行滤波去掉噪声
cv2.imshow("Blur", blured)     #显示低通滤波后的图像
 
mask = np.zeros((h+2, w+2), np.uint8)  #掩码长和宽都比输入图像多两个像素点，满水填充不会超出掩码的非零边缘 
#进行泛洪填充
cv2.floodFill(blured, mask, (w-1,h-1), (255,255,255), (2,2,2),(3,3,3),8)
cv2.imshow("floodfill", blured) 
 
#得到灰度图
gray = cv2.cvtColor(blured,cv2.COLOR_BGR2GRAY) 
cv2.imshow("gray", gray) 
 
 
#定义结构元素 
kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(50, 50))
#开闭运算，先开运算去除背景噪声，再继续闭运算填充目标内的孔洞
opened = cv2.morphologyEx(gray, cv2.MORPH_OPEN, kernel) 
closed = cv2.morphologyEx(opened, cv2.MORPH_CLOSE, kernel) 
cv2.imshow("closed", closed) 
 
#求二值图
ret, binary = cv2.threshold(closed,250,255,cv2.THRESH_BINARY) 
cv2.imshow("binary", binary) 
 
#找到轮廓
contours, hierarchy = cv2.findContours(binary,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE) 
#绘制轮廓
 
cv2.drawContours(img,contours,-1,(0,0,255),3) 
#绘制结果
cv2.imshow("result", img)



contours, hierarchy = cv2.findContours(binary,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)  
print (len(contours))
ic = 0

 

emptyImage = np.zeros(binary.shape, np.uint8)  
for element in contours:
	#cv2.drawContours(emptyImage,element,-1,(0,255,0),2)
	#continue
	epsilon = 0.1 * cv2.arcLength(element, True)
	cnt = cv2.approxPolyDP(element, epsilon, closed=True)
	#########
	x,y,w,h =cv2.boundingRect(element)
	area = cv2.contourArea(cnt)
	if area <10 :
		continue
	#cv2.rectangle(emptyImage,(x,y),(x+w,y+h),(0,255,0),2)
	#continue
	#########

	lc = len(cnt)
	area = cv2.contourArea(cnt)
	if area < 10 or area >100000000000 :
		continue
	if lc== 4 :
		rect = cv2.minAreaRect(cnt)
		x,y,w,h = cv2.boundingRect(cnt)

		#if (abs(w-h) <1) :
		#	continue
		
		print x,y,w,h
	 
 
		box = cv2.cv.BoxPoints(rect)
		box = np.int0(box)
 
		cv2.drawContours(emptyImage,[box],0,(255,255,255),2)
	 
print ic


#cv2.drawContours(emptyImage2,contours,-1,(255,255,255),4)  
  
cv2.imshow("emptyImage2", emptyImage) 
 
cv2.waitKey(0) 
cv2.destroyAllWindows()
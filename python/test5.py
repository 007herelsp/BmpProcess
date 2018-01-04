# encoding: utf-8    

#检测边缘
#膨胀时，图像中的物体会向周围“扩张”；腐蚀时，图像中的物体会“收缩”。由于变化区域只发生在边缘，所以这时将两幅图像相减，
#得到的就是图像中物体的边缘。
  
import cv2    
import numpy as np    
  
image = cv2.imread("./images/Target17.bmp",0);    
#构造一个3×3的结构元素     
element = cv2.getStructuringElement(cv2.MORPH_RECT,(3, 3))    
dilate = cv2.dilate(image, element)    
erode = cv2.erode(image, element)    

erode1 = cv2.dilate(erode, element) 
cv2.imshow("erode1",erode1);  
#将两幅图像相减获得边，第一个参数是膨胀后的图像，第二个参数是腐蚀后的图像    
result = cv2.absdiff(dilate,erode);    
cv2.imshow("result",result);     
#上面得到的结果是灰度图，将其二值化以便更清楚的观察结果    
#retval, result = cv2.threshold(result, 40, 255, cv2.THRESH_BINARY);  
retval, result = cv2.threshold(result,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)   
#反色，即对二值图每个像素取反    
result = cv2.bitwise_not(result);     
#显示图像    


blur = cv2.GaussianBlur(image,(3,3),0)
cv2.imshow("blur",blur); 
canny = cv2.Canny(blur,0.5,20,3)
cv2.imshow("canny",canny); 
blur = cv2.GaussianBlur(canny,(3,3),0)
cv2.imshow("blur2",blur);
result = cv2.GaussianBlur(result,(3,3),0)

contours, hierarchy = cv2.findContours(result,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)  
print (len(contours))
ic = 0

 

emptyImage = np.zeros(image.shape, np.uint8)  
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
#cv2.imshow("edges", img) 

cv2.waitKey(0)    
cv2.destroyAllWindows() 
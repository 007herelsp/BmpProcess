# encoding: utf-8  
#Harris角点检测算法代码
import cv2
import numpy as np
import matplotlib
import math
from matplotlib import pyplot as plt  

kernel=np.uint8(np.zeros((5,5)))  
for x in range(5):  
    kernel[x,2]=1;  
    kernel[2,x]=1;

#根据一阶锐化算子，求x，y的梯度，显示锐化图像
#读取图片
filename = '../images/Target17.bmp'
img = cv2.imread(filename)
img = cv2.bilateralFilter(img,9,75,75)
cv2.imshow("bilateralFilter", img)
emptyImage = np.zeros(img.shape, np.uint8)  
(B,G,R) = cv2.split(img)
#gray = cv2.GaussianBlur(img,(9,9),0)
gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
gray =G
canny = cv2.Canny(img,0.1,3,3)
eroded=cv2.erode(img,kernel); 
dilate = cv2.dilate(img,kernel) 
 


y = dilate - eroded
 
cv2.normalize(y,y,255,0,cv2.NORM_MINMAX)
cv2.imshow("diff", y)
cv2.imshow("canny", canny)
cv2.imwrite("canny.bmp",canny);
 
g1 = cv2.GaussianBlur(gray,(3,3),0)
g2 = cv2.GaussianBlur(g1,(3,3),0)

#GaussianBlur(img,img_G0,Size(3,3),0);    
#GaussianBlur(img_G0,img_G1,Size(3,3),0);    
img_DoG = g1 - g2;    
cv2.normalize(img_DoG,img_DoG,255,0,cv2.NORM_MINMAX)
#img_DoG = cv2.GaussianBlur(img_DoG,(3,3),0)
eroded=cv2.erode(img_DoG,kernel); 
dilate = cv2.dilate(img_DoG,kernel) 

d = dilate - eroded
cv2.imshow("dog1", img_DoG)
img = cv2.imread(filename,0)

gray_lap = cv2.Laplacian(img,cv2.CV_16S,ksize = 3)    
dst = cv2.convertScaleAbs(gray_lap)
#lap = np.uint8(np.absolute(dst))##对lap去绝对值 
cv2.imshow("dst", dst) 
canny = dst
contours, hierarchy = cv2.findContours(canny,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)  
print (len(contours))
ic = 0


lines = cv2.HoughLinesP(canny,1,np.pi/180,30,minLineLength=40,maxLineGap=30)
#lines = cv2.HoughLines(canny,1,np.pi/180,160)
lines1 = lines[:,0,:]#
for x1,y1,x2,y2 in lines1[:]: 
    cv2.line(emptyImage,(x1,y1),(x2,y2),(255,0,0),1)
cv2.imshow("emptyImage", emptyImage) 


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
	if area < 200   :
		continue
	if lc== 4 :
		rect = cv2.minAreaRect(cnt)
		x,y,w,h = cv2.boundingRect(cnt)

		#if (abs(w-h) <1) :
		#	continue
		
		print x,y,w,h
	 	cv2.imroi( img, rect );
		cv2.imshow("tem image",img)
 
		box = cv2.cv.BoxPoints(rect)
		box = np.int0(box)
		 
	#x,y,w,h =cv2.boundingRect(element)
  
#	if abs(w-h)>3 :
#		w=0;  
#		h=0;  
#		x = x + 640;  
#		y = y + 480;  
  
  
     
    #cvRectangle(dst, pt1, pt2, CV_RGB(255,0,0), 1, CV_AA, 0);   
		cv2.rectangle(emptyImage,(x,y),(x+w,y+h),(0,0,255),2)
		cv2.drawContours(emptyImage,[box],0,(255,255,255),2)
		#cv2.drawContours(emptyImage,element,-1,(0,255,0),2)
		#cv2.rectangle(emptyImage,(x,y),(x+w,y+h),(0,255,0),2)
	#rect = cv2.minAreaRect(element)
	#box = cv2.cv.BoxPoints(rect)
	#box = np.int0(box)
		ic = ic +1
	#cv2.drawContours(emptyImage, [box], 0, (0, 0, 255), 2)
	#cv2.imwrite('contours.png', img)
	#cv2.drawContours(img, element, 0,(255,0,0),  2); 
	#cv2.rectangle(emptyImage,(384,0),(510,128),(0,255,0),3)
	#cv2.imshow("img", emptyImage)
	#break
	#print M
print ic


#cv2.drawContours(emptyImage2,contours,-1,(255,255,255),4)  
  
cv2.imshow("emptyImage2", emptyImage) 


cv2.waitKey (0)  
cv2.destroyAllWindows() 
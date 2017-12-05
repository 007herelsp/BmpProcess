# coding=UTF-8
import cv2  
import cv2.cv as cv
import numpy as np  
  
#
def gamma_trans(img, gamma):

	gamma_table = [np.power(x/255.0, gamma)*255.0 for x in range(256)]
	gamma_table = np.round(np.array(gamma_table)).astype(np.uint8)

	return cv2.LUT(img, gamma_table)


  
img = cv2.imread("Target14.bmp")  
#cv2.imshow("img", img)

image = cv.LoadImage("Target14.bmp")
new = cv.CreateImage(cv.GetSize(image), image.depth, 1) 
for i in range(image.height):
		for j in range(image.width):
			new[i,j] = max(image[i,j][0], image[i,j][1], image[i,j][2])
cv.ShowImage('a_window', new)
cv.SaveImage("t.bmp", new)
img3 = cv2.imread("t.bmp")

#cv2.imshow("img", img)
#image = cv2.pyrMeanShiftFiltering(img, 25, 10)
#cv2.imshow("image", image)
#img = image
emptyImage = np.zeros(img.shape, np.uint8)  
  
emptyImage2 = img.copy()  
  
emptyImage3=cv2.cvtColor(img,cv2.COLOR_BGR2GRAY) 
cv2.imwrite("imgg.bmp",emptyImage3);
#cv2.imshow("emptyImage3", emptyImage3)
ret, threshold = cv2.threshold(emptyImage3,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
#cv2.imshow("threshold", threshold)
emptyImage3 = gamma_trans(emptyImage3, 0.5)
ret, thr1 = cv2.threshold(emptyImage3,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
#cv2.imshow("thr1", emptyImage3)

(B,G,R) = cv2.split(img)
cv2.imwrite("colorB.bmp",B);
cv2.imwrite("colorG.bmp",G);
cv2.imwrite("colorR.bmp",R);
 
bgrequalizeHist1 = np.hstack([B,
                     G,
                     R
                     ])
#cv2.imshow("equalizeHist1",bgrequalizeHist1)


B1 = cv2.equalizeHist(B)
G1 = cv2.equalizeHist(G)
R1 = cv2.equalizeHist(R)
img2 = cv2.merge( [B1, G1, R1])   

bgrequalizeHist = np.hstack([B1,
                     G1,
                     R1
                     ])
#cv2.imshow("equalizeHist",bgrequalizeHist)
#cv2.imwrite("img2.bmp",img2);
#img = gamma_trans(cv2.cvtColor(img,cv2.COLOR_BGR2GRAY), 0.1)

blur = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)  
#cv2.imshow("COLOR_BGR2HSV", img)
img = cv2.equalizeHist(cv2.cvtColor(img,cv2.COLOR_BGR2GRAY))
imgAdapt = cv2.adaptiveThreshold(img,255,cv2.ADAPTIVE_THRESH_GAUSSIAN_C,cv2.THRESH_BINARY,11,2) 
cv2.imshow("imgAdapt",imgAdapt) 
# Otsu's thresholding after Gaussian filtering
blur = cv2.GaussianBlur(img,(3,3),0)
img2 = cv2.imread('Target14.bmp',0) #直接读为灰度图像

kernel = np.ones((5,5),np.uint8)
erosion = cv2.erode(img,kernel,1)
 
#cv2.imshow("erosion",erosion)

#cv2.imshow("img", img)
#blur = cv2.GaussianBlur(img,(3,3),0)
img2 = cv2.imread('ct.bmp') #直接读为灰度图像
canny = cv2.Canny(img2,30,90,1)
#canny = cv2.medianBlur(canny,3)
canny = cv2.GaussianBlur(canny,(3,3),0)
 
#中值滤波
blurred = np.hstack([cv2.medianBlur(img,3),
                     cv2.medianBlur(img,5),
                     cv2.medianBlur(img,7)
                     ])
#cv2.imshow("Median",blurred)

#双边滤波
blurred = np.hstack([cv2.bilateralFilter(img,5,21,21),
                     cv2.bilateralFilter(img,7, 31, 31),
                     cv2.bilateralFilter(img,9, 41, 41)
                     ])
#cv2.imshow("Bilateral",blurred)

#cv2.imshow("canny", canny)
ret3,b = cv2.threshold(R,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
cv2.imwrite("BR.bmp",b);
#cv2.imshow("R", th3)
ret3,g = cv2.threshold(G,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
cv2.imwrite("BG.bmp",g);
#cv2.imshow("G", th3)
ret3,r = cv2.threshold(B,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
cv2.imwrite("BB.bmp",r);
#cv2.imshow("B", th3)


bgr = np.hstack([b,g,r
                     ])
cv2.imshow("bgr",bgr)
 
#ret3,th3 = cv2.threshold(blur,0,255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
#canny = thr1 
#cv2.imshow("EmptyImage3", thr1)  
#cv2.imshow("th3", edges)

#RETR_CCOMP,RETR_TREE, RETR_EXTERNAL
#blur = cv2.GaussianBlur(img,(3,3),0)
#gray = cv2.cvtColor(blur,cv2.COLOR_BGR2GRAY) 
#cv2.imshow("gray", gray)
#canny = cv2.threshold(gray,50,200, cv2.THRESH_BINARY)

cv2.imshow("canny", canny)
#blur = cv2.GaussianBlur(canny,(3,3),0)
#cv2.imshow("blur", blur)
kernel=np.ones((5,5),np.uint8)
dst3=cv2.morphologyEx(imgAdapt,cv2.MORPH_CLOSE,kernel)
cv2.imshow("morphologyEx_Open", dst3)
##
element = cv2.getStructuringElement(cv2.MORPH_RECT,(3, 3))    
dilate = cv2.dilate(img, element)    
erode = cv2.erode(img, element)    
    
#将两幅图像相减获得边，第一个参数是膨胀后的图像，第二个参数是腐蚀后的图像    
result = cv2.absdiff(dilate,erode);    
    
#上面得到的结果是灰度图，将其二值化以便更清楚的观察结果    
retval, result = cv2.threshold(result, 40, 255, cv2.THRESH_BINARY);     
#反色，即对二值图每个像素取反    
result = cv2.bitwise_not(result);     
#显示图像    
cv2.imshow("result",result);

#
dst1=cv2.erode(img,kernel,iterations=1)
#cv2.imshow("erode", dst1) 
#
dst2=cv2.dilate(canny,kernel,iterations=1)
#cv2.imshow("dilate", dst2) 

#
dst3=cv2.morphologyEx(canny,cv2.MORPH_OPEN,kernel)
#cv2.imshow("morphologyEx_Open", dst3) 

dst4=cv2.morphologyEx(canny,cv2.MORPH_CLOSE,kernel)
#cv2.imshow("morphologyEx_Close", dst4) 

blur = cv2.GaussianBlur(img,(3,3),0)
gray_lap = cv2.Laplacian(img,cv2.CV_16S,ksize = 3)  
dst = cv2.convertScaleAbs(gray_lap)  
dst = cv2.GaussianBlur(dst,(3,3),0)
cv2.imshow('laplacian',dst)
contours, hierarchy = cv2.findContours(b,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)  
print (len(contours))
ic = 0


lines = cv2.HoughLinesP(gray,1,np.pi/180,60,30,  10)
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
#cv2.imshow("edges", img) 



cv2.waitKey (0)  
cv2.destroyAllWindows()  
# coding=UTF-8
import cv2  
import cv2.cv as cv
import numpy as np  
  
#
def gamma_trans(img, gamma):

	gamma_table = [np.power(x/255.0, gamma)*255.0 for x in range(256)]
	gamma_table = np.round(np.array(gamma_table)).astype(np.uint8)

	return cv2.LUT(img, gamma_table)

def bint(img, val):
	rows,cols = img.shape
	print val;
	for i in range(rows):
		for j in range(cols):
			#print "["+str(i)+","+str(j)+"]:" + str(img[i,j])
			if img[i,j] != val:
				img[i,j] = 0
			else:
				img[i,j] =255
				#print 'xx';
				#new[i,j] = max(image[i,j][0], image[i,j][1], image[i,j][2])

  
img = cv2.imread("Target15.bmp")  
#cv2.imshow("img", img)

 

#img = cv2.equalizeHist(cv2.cvtColor(img,cv2.COLOR_BGR2GRAY))

gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)



#canny = cv2.Canny(img,30,90,3)
 
#canny = cv2.GaussianBlur(canny,(3,3),0)
emptyImage = np.zeros(img.shape, np.uint8)   
for i in range(255):
	#gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
	(B,G,R) = cv2.split(img)
	gray = G
	bint(gray, i)
	threshold = gray
	#ret, threshold = cv2.threshold(gray,i,255, cv2.THRESH_BINARY) 
	threshold = cv2.Canny(threshold,10,90,5)
	contours, hierarchy = cv2.findContours(threshold,cv2.RETR_CCOMP,cv2.CHAIN_APPROX_SIMPLE)  
	print (len(contours))
	ic = 0
	cv2.imwrite("t"+str(i)+".bmp", gray)

	#lines = cv2.HoughLinesP(threshold,1,np.pi/180,20, maxLineGap=20)
	#lines1 = lines[:,0,:]#
	#for x1,y1,x2,y2 in lines1[:]: 
	#    cv2.line(emptyImage,(x1,y1),(x2,y2),(255,0,0),1)
	#cv2.imshow("emptyImage", emptyImage) 


	for element in contours:
	    #cv2.drawContours(emptyImage,element,-1,(0,255,0),2)
	    #continue
		epsilon = 0.1 * cv2.arcLength(element, True)
		cnt = cv2.approxPolyDP(element, epsilon, closed=True)
		#########
		x,y,w,h =cv2.boundingRect(element)
		area = cv2.contourArea(cnt)
		if area <100 :
			continue

	#########

		lc = len(cnt)
		if lc>=3 :
			rect = cv2.minAreaRect(cnt)
			x,y,w,h = cv2.boundingRect(cnt)
		
	    	#print x,y,w,h
			box = cv2.cv.BoxPoints(rect)
			box = np.int0(box)
			cv2.rectangle(emptyImage,(x,y),(x+w,y+h),(0,0,255),2)
			cv2.drawContours(emptyImage,[box],0,(255,255,255),2)
			ic = ic +1
	print ic
  
cv2.imshow("emptyImage2", emptyImage) 

cv2.waitKey (0)  
cv2.destroyAllWindows()  
# coding=UTF-8
import cv2
import numpy as np

img = cv2.imread('Target14.bmp') 
cv2.imshow("img", img)
emptyImage = np.zeros(img.shape, np.uint8)  
gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)#灰度图像 
#open to see how to use: cv2.Canny
#http://blog.csdn.net/on2way/article/details/46851451 
edges = cv2.Canny(gray,50,200)
cv2.imshow("edges", edges)
#hough transform
lines = cv2.HoughLinesP(edges,1,np.pi/180,30,minLineLength=60,maxLineGap=10)
lines1 = lines[:,0,:]#提取为为二维
result = img.copy() 
for line in lines[0]:   
    rho = line[0] #第一个元素是距离rho    
    theta= line[1] #第二个元素是角度theta    
    print rho    
    print theta    
    
    if  (theta < (np.pi/4. )) or (theta > (3.*np.pi/4.0)): #垂直直线    
                #该直线与第一行的交点    
        pt1 = (int(rho/np.cos(theta)),0)    
        #该直线与最后一行的焦点    
        pt2 = (int((rho-result.shape[0]*np.sin(theta))/np.cos(theta)),result.shape[0])    
        #绘制一条白线    
        cv2.line( result, pt1, pt2, (255))    
    else: #水平直线    
        # 该直线与第一列的交点    
        pt1 = (0,int(rho/np.sin(theta)))    
        #该直线与最后一列的交点    
        pt2 = (result.shape[1], int((rho-result.shape[1]*np.cos(theta))/np.sin(theta)))    
        #绘制一条直线    
        cv2.line(result, pt1, pt2, (255), 1)    
  

cv2.imshow("image", result)
cv2.waitKey (0)  
cv2.destroyAllWindows()  
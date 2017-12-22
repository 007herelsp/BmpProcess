import cv2  
  
img = cv2.imread('D:/opensrc/BmpProcess/images/Target18.bmp')  
gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)  
canny = cv2.Canny(img,5,15,3)
#ret, binary = cv2.threshold(gray,127,255,cv2.THRESH_BINARY)  
cv2.imshow("canny", canny)    
contours, hierarchy = cv2.findContours(canny,cv2.RETR_LIST,cv2.CHAIN_APPROX_TC89_KCOS)  
cv2.drawContours(img,contours,-1,(0,0,255),3)  

cv2.imshow("img", img)  
cv2.waitKey(0) 
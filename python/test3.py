import cv2  
import numpy as np  
  
def CannyThreshold(lowThreshold):  
    ret, threshold = cv2.threshold(gray,lowThreshold,255, cv2.THRESH_BINARY)
    cv2.imshow('canny demo',threshold)  
  
lowThreshold = 0  
max_lowThreshold = 255  
ratio = 3  
kernel_size = 3  
  
img = cv2.imread('Target1.bmp')  
gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)  
  
cv2.namedWindow('canny demo')  
  
cv2.createTrackbar('Min threshold','canny demo',lowThreshold, max_lowThreshold, CannyThreshold)  
  
CannyThreshold(0)  # initialization  
if cv2.waitKey(0) == 27:  
    cv2.destroyAllWindows()  
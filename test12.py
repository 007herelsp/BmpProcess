# coding=UTF-8
"""
Created on Thu Jun 29 14:23:56 2017

@author: Administrator
"""

import cv2
import numpy as np
import os

CANNY_LOW_THRESHOLD = 50                # canny算法低阈值
CANNY_HIGH_THRESHOLD = 150              #canny算法高阈值
HOUGH_DELTARHO = 1                      #hough检测步长
HOUGH_DELTA_THETA = 100
TESTIMAGESDIR = 'D:\\opensrc\\BmpProcess'

class RotateByHough(object):
    def __init__(self,image):
        self._srcimage = image

#获取图像背景像素值        
    def get_background_pix(self,):
        width = self._srcimage.shape[1]
        height = self._srcimage.shape[0]
        left_coners = self._srcimage[0:9,0:9]
        right_coners = self._srcimage[height-10:height-1,width-10:width-1]
        left_array = np.array(left_coners).reshape(81,1,3)
        left_point = np.mean(left_array,0)        
        right_point = np.mean(np.array(right_coners).reshape(81,1,3),0)
#        print(left_point)
        final = np.array((left_point[0],right_point[0]),np.float32).reshape(2,1,3)        
        final_point = np.mean(final,0)
        
        self._point_scale = (int(final_point[0][0]),int(final_point[0][1]),int(final_point[0][2]))
#        print(self._point_scale)

#       输入矫正图像的宽和高        
    def detect_hough_line(self):
        _grayimage = cv2.cvtColor(self._srcimage,cv2.COLOR_RGB2GRAY)
        _cannyimage = cv2.Canny(_grayimage,CANNY_LOW_THRESHOLD, CANNY_HIGH_THRESHOLD, apertureSize=3)
        lines = cv2.HoughLinesP(_cannyimage,1,np.pi/180,160,minLineLength=200, maxLineGap=180)
        
#        寻找长度最长的线
        distance = []
        for line in lines:
            x1,y1,x2,y2 = line[0]
            dis = np.sqrt(pow((x2-x1),2)+pow((y2-y1),2))
            distance.append(dis)
        max_dis_index = distance.index(max(distance))        
        max_line = lines[max_dis_index]
        x1,y1,x2,y2 = max_line[0]

#       获取旋转角度
        angle = cv2.fastAtan2((y2-y1),(x2-x1))
        centerpoint = (self._srcimage.shape[1]/2,self._srcimage.shape[0]/2)
        rotate_mat = cv2.getRotationMatrix2D(centerpoint,angle,1.0)         #获取旋转矩阵
        correct_image = cv2.warpAffine(self._srcimage,rotate_mat,(self._srcimage.shape[1],self._srcimage.shape[0]),borderValue =(255,255,255) )        
        return correct_image

def get_files(images_dir):
    images = []
    for _file in os.listdir(images_dir):
        filename = os.path.join(images_dir, _file)
        images.append(filename)
        
    return images

#==============================================================================
# def key_board_event(event):
# #    监听键盘输入值
# 
#     print("Key :", event.Key)
#     if event.Key == 'Down':
#         print('next file')        
#         i = i + 1
#         if i > len(image_files):
#             i = len(image_files)        
#         print(i)
#         print(image_files[i])
#     if event.Key == 'Up':
#         print('pre file')
#         i = i -1
#         if i < 0:
#             i = 0     
#     
#==============================================================================    
def main():
    image_files = get_files(TESTIMAGESDIR)
    print image_files
    files_num = len(image_files)
    while True:
        num = int(input("Enter your input:"))        
        if num > 50:
            break;
        if num < 0 or num > files_num:
            print('resume load')
        else:
            file_name = image_files[num]
            print(file_name)
            image = cv2.imread(file_name)
            roteimage_ins = RotateByHough(image)
            roteimage = roteimage_ins.detect_hough_line()
            width = roteimage.shape[1]
            height = roteimage.shape[0]

            for i in range(0, height,  50):
                for j in range(0, width, 50):
                    cv2.line(roteimage, (0,i),(width, i),(255, 0, 255), 1)                    
            cv2.imshow('test', roteimage)
            cv2.waitKey(0)
#==============================================================================
#     hm = pyHook.HookManager()             #设置一个钩子管理对象
#     hm.KeyDown = key_board_event          #监听所有键盘事件
#     hm.HookKeyboard()                     #设置键盘钩子
#==============================================================================    
    
if __name__ == "__main__":
    main()
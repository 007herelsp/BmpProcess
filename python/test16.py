# encoding: utf-8  
#Harris角点检测算法代码
import cv2
import numpy as np
import matplotlib
import math
from matplotlib import pyplot as plt  

#根据一阶锐化算子，求x，y的梯度，显示锐化图像
#读取图片
filename = 'Target14.bmp'
tu = cv2.imread(filename)

#转换为灰度图
gray = cv2.cvtColor(tu, cv2.COLOR_RGB2GRAY)

#获取图像属性
print '获取图像大小: '
print gray.shape
print '\n'

#打印数组gray
print '灰度图像数组：\n %s \n \n' % (gray)

#输出n*n的数组
#print gray[:2,:2]

#转换为矩阵
m = np.matrix(gray)

#计算x方向的梯度的函数（水平方向锐化算子）
delta_h = m
def grad_x(h):
    a = int(h.shape[0])
    b = int(h.shape[1])

    for i in range(a):
        for j in range(b):
            if i-1>=0 and i+1<a and j-1>=0 and j+1<b:
                c = abs(int(h[i-1,j-1]) - int(h[i+1,j-1]) + 2*(int(h[i-1,j]) - int(h[i+1,j])) + int(h[i-1,j+1]) - int(h[i+1,j+1]))
#                print c
                if c>255:
#                    print c
                    c = 255
                delta_h[i,j] = c
            else:
                delta_h[i,j] = 0
    print 'x方向的梯度：\n %s \n' %delta_h
    return delta_h

##计算y方向的梯度的函数（水平方向锐化算子）
def grad_y(h):
    a = int(h.shape[0])
    b = int(h.shape[1])

    for i in range(a):
        for j in range(b):
            if i-1>=0 and i+1<a and j-1>=0 and j+1<b:
                c = abs(int(h[i-1,j-1]) - int(h[i-1,j+1]) + 2*(int(h[i,j-1]) - int(h[i,j+1])) + (int(h[i+1,j-1]) - int(h[i+1,j+1])))  #注意像素不能直接计算，需要转化为整型
#                print c
                if c > 255:
                    c = 255
                delta_h[i,j] = c
            else:
                delta_h[i,j] = 0
    print 'y方向的梯度：\n %s \n' %delta_h
    return delta_h

# Laplace 算子  
img_laplace = cv2.Laplacian(gray, cv2.CV_64F, ksize=3)  

dx = np.array(grad_x(gray))
dy = np.array(grad_y(gray))

#dxy = dx + dy
#print 'dxy1:'
#print dxy

A = dx * dx
B = dy * dy 
C = dx * dy

print A
print B
print C

A1 = A
B1 = B
C1 = C

A1 = cv2.GaussianBlur(A1,(3,3),1.5)
B1 = cv2.GaussianBlur(B1,(3,3),1.5)
C1 = cv2.GaussianBlur(C1,(3,3),1.5)

print A1
print B1
print C1

a = int(gray.shape[0])
b = int(gray.shape[1])

R = np.zeros(gray.shape)
for i in range(a):
    for j in range(b):
        M = [[A1[i,j],C1[i,j]],[C1[i,j],B1[i,j]]]

        R[i,j] = np.linalg.det(M) - 0.06 * (np.trace(M)) * (np.trace(M))

print R

cv2.namedWindow('R',cv2.WINDOW_NORMAL)
cv2.imshow('R',R)

cv2.waitKey(0)
cv2.destroyAllWindows()
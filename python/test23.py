import cv2
import numpy as np
from matplotlib import pyplot as plt

img = cv2.imread('../images/out.bmp')
median = cv2.medianBlur(img,5)
cv2.imwrite('../images/out1.bmp',median)
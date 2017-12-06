# coding=UTF-8
import cv2  
import cv2.cv as cv
import numpy as np  
import sys
import argparse
parser = argparse.ArgumentParser(description='manual to this script')
parser.add_argument('--file', type=str, default = None)
args = parser.parse_args()
#print args.file
 
  
#
def gamma_trans(img, gamma):
	gamma_table = [np.power(x/255.0, gamma)*255.0 for x in range(256)]
	gamma_table = np.round(np.array(gamma_table)).astype(np.uint8)
	return cv2.LUT(img, gamma_table)


def gamma_process(file):
    img = cv2.imread(file)  
    emptyImage = np.zeros(img.shape, np.uint8)  
    emptyImage2 = img.copy()  
    emptyImage3=cv2.cvtColor(img,cv2.COLOR_BGR2GRAY) 
    emptyImage3 = gamma_trans(emptyImage3, 0.5)

    cv2.imwrite(file+"_gamma.bmp",emptyImage3);
    cv2.imshow(file+"_gamma.bmp", emptyImage3)
    cv2.waitKey (0)  
    cv2.destroyAllWindows()  


def main():
    gamma_process(args.file)

if __name__=="__main__":
    main()
    

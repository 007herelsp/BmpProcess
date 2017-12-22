#ifndef _CV_IMG_PROC_H_
#define _CV_IMG_PROC_H_

#undef   CV_CALC_MIN
#define  CV_CALC_MIN(a, b) if((a) > (b)) (a) = (b)

#undef   CV_CALC_MAX
#define  CV_CALC_MAX(a, b) if((a) < (b)) (a) = (b)

#define CV_MORPH_ALIGN  4

#define CV_WHOLE   0
#define CV_START   1
#define CV_END     2
#define CV_MIDDLE  4

#define ICV_WARP_SHIFT          10
#define ICV_WARP_MASK           ((1 << ICV_WARP_SHIFT) - 1)




#endif /*_CV_INTERNAL_H_*/

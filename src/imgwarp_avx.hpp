 

#ifndef _CV_IMGWARP_AVX_H_
#define _CV_IMGWARP_AVX_H_

int VResizeLinearVec_32f_avx(const uchar** _src, uchar* _dst, const uchar* _beta, int width );

int VResizeCubicVec_32f_avx(const uchar** _src, uchar* _dst, const uchar* _beta, int width );

#endif

/* End of file. */

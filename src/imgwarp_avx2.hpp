 

#ifndef _CV_IMGWARP_AVX2_H_
#define _CV_IMGWARP_AVX2_H_

int VResizeLinearVec_32s8u_avx2(const uchar** _src, uchar* dst, const uchar* _beta, int width );

template<int shiftval>
int VResizeLinearVec_32f16_avx2(const uchar** _src, uchar* _dst, const uchar* _beta, int width );

int VResizeCubicVec_32s8u_avx2(const uchar** _src, uchar* dst, const uchar* _beta, int width );

template<int shiftval>
int VResizeCubicVec_32f16_avx2(const uchar** _src, uchar* _dst, const uchar* _beta, int width );

#endif

/* End of file. */

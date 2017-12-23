/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef _CV_IPP_H_
#define _CV_IPP_H_



/****************************************************************************************\
*                                Geometric Transformations                               *
\****************************************************************************************/


#define IPCV_REMAP( flavor, cn )                                        \
IPCVAPI_EX( CvStatus, icvRemap_##flavor##_C##cn##R,                     \
    "ippiRemap_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),     \
    ( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,      \
      const float* xmap, int xmapstep, const float* ymap, int ymapstep, \
      void* dst, int dststep, CvSize dstsize, int interpolation )) 

IPCV_REMAP( 8u, 1 )
IPCV_REMAP( 8u, 3 )
IPCV_REMAP( 8u, 4 )

IPCV_REMAP( 32f, 1 )
IPCV_REMAP( 32f, 3 )
IPCV_REMAP( 32f, 4 )

#undef IPCV_REMAP

/****************************************************************************************\
*                                      Morphology                                        *
\****************************************************************************************/

#define IPCV_MORPHOLOGY( minmaxtype, morphtype, flavor, cn )                    \
IPCVAPI_EX( CvStatus, icv##morphtype##Rect_##flavor##_C##cn##R,                 \
    "ippiFilter" #minmaxtype "BorderReplicate_" #flavor "_C" #cn "R",           \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( const void* src, int srcstep, void* dst,    \
    int dststep, CvSize roi, CvSize esize, CvPoint anchor, void* buffer ))      \
IPCVAPI_EX( CvStatus, icv##morphtype##Rect_GetBufSize_##flavor##_C##cn##R,      \
    "ippiFilter" #minmaxtype "GetBufferSize_" #flavor "_C" #cn "R",             \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( int width, CvSize esize, int* bufsize ))    \
                                                                                \
IPCVAPI_EX( CvStatus, icv##morphtype##_##flavor##_C##cn##R,                     \
    "ippi" #morphtype "BorderReplicate_" #flavor "_C" #cn "R",                  \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( const void* src, int srcstep,               \
    void* dst, int dststep, CvSize roi, int bordertype, void* morphstate ))

IPCV_MORPHOLOGY( Min, Erode, 8u, 1 )
IPCV_MORPHOLOGY( Min, Erode, 8u, 3 )
IPCV_MORPHOLOGY( Min, Erode, 8u, 4 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 1 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 3 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 4 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 1 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 3 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 4 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 1 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 3 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 4 )

#undef IPCV_MORPHOLOGY

#define IPCV_MORPHOLOGY_INIT_ALLOC( flavor, cn )                            \
IPCVAPI_EX( CvStatus, icvMorphInitAlloc_##flavor##_C##cn##R,                \
    "ippiMorphologyInitAlloc_" #flavor "_C" #cn "R",                        \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( int width, const uchar* element,        \
    CvSize esize, CvPoint anchor, void** morphstate ))

IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 1 )
IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 3 )
IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 4 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 1 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 3 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 4 )

#undef IPCV_MORPHOLOGY_INIT_ALLOC

IPCVAPI_EX( CvStatus, icvMorphFree, "ippiMorphologyFree",
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( void* morphstate ))


/****************************************************************************************\
*                                 Derivative Filters                                     *
\****************************************************************************************/

#define IPCV_FILTER_SOBEL_BORDER( suffix, flavor, srctype )                             \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##GetBufSize_##flavor##_C1R,                \
    "ippiFilterSobel" #suffix "GetBufferSize_" #flavor "_C1R",                          \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int masksize, int* buffersize ))        \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##Border_##flavor##_C1R,                    \
    "ippiFilterSobel" #suffix "Border_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),   \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int masksize,   \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_SOBEL_BORDER( NegVert, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( Horiz, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( VertSecond, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( HorizSecond, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( Cross, 8u16s, uchar )

IPCV_FILTER_SOBEL_BORDER( NegVert, 32f, float )
IPCV_FILTER_SOBEL_BORDER( Horiz, 32f, float )
IPCV_FILTER_SOBEL_BORDER( VertSecond, 32f, float )
IPCV_FILTER_SOBEL_BORDER( HorizSecond, 32f, float )
IPCV_FILTER_SOBEL_BORDER( Cross, 32f, float )

#undef IPCV_FILTER_SOBEL_BORDER

#define IPCV_FILTER_SCHARR_BORDER( suffix, flavor, srctype )                            \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##GetBufSize_##flavor##_C1R,               \
    "ippiFilterScharr" #suffix "GetBufferSize_" #flavor "_C1R",                         \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int* buffersize ))                      \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##Border_##flavor##_C1R,                   \
    "ippiFilterScharr" #suffix "Border_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),  \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi,                 \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_SCHARR_BORDER( Vert, 8u16s, uchar )
IPCV_FILTER_SCHARR_BORDER( Horiz, 8u16s, uchar )

IPCV_FILTER_SCHARR_BORDER( Vert, 32f, float )
IPCV_FILTER_SCHARR_BORDER( Horiz, 32f, float )

#undef IPCV_FILTER_SCHARR_BORDER


#define IPCV_FILTER_LAPLACIAN_BORDER( flavor, srctype )                                 \
IPCVAPI_EX( CvStatus, icvFilterLaplacianGetBufSize_##flavor##_C1R,                      \
    "ippiFilterLaplacianGetBufferSize_" #flavor "_C1R",                                 \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int masksize, int* buffersize ))        \
IPCVAPI_EX( CvStatus, icvFilterLaplacianBorder_##flavor##_C1R,                          \
    "ippiFilterLaplacianBorder_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),          \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int masksize,   \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_LAPLACIAN_BORDER( 8u16s, uchar )
IPCV_FILTER_LAPLACIAN_BORDER( 32f, float )

#undef IPCV_FILTER_LAPLACIAN_BORDER


/////////////////////////////////////////////////////////////////////////////////////////

#define IPCV_FILTER_SOBEL( suffix, ipp_suffix, flavor )                             \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##_##flavor##_C1R,                      \
    "ippiFilterSobel" #ipp_suffix "_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),  \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int aperture ))

IPCV_FILTER_SOBEL( Vert, Vert, 8u16s )
IPCV_FILTER_SOBEL( Horiz, Horiz, 8u16s )
IPCV_FILTER_SOBEL( VertSecond, VertSecond, 8u16s )
IPCV_FILTER_SOBEL( HorizSecond, HorizSecond, 8u16s )
IPCV_FILTER_SOBEL( Cross, Cross, 8u16s )

IPCV_FILTER_SOBEL( Vert, VertMask, 32f )
IPCV_FILTER_SOBEL( Horiz, HorizMask, 32f )
IPCV_FILTER_SOBEL( VertSecond, VertSecond, 32f )
IPCV_FILTER_SOBEL( HorizSecond, HorizSecond, 32f )
IPCV_FILTER_SOBEL( Cross, Cross, 32f )

#undef IPCV_FILTER_SOBEL

#define IPCV_FILTER_SCHARR( suffix, ipp_suffix, flavor )                            \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##_##flavor##_C1R,                     \
    "ippiFilterScharr" #ipp_suffix "_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI), \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi ))

IPCV_FILTER_SCHARR( Vert, Vert, 8u16s )
IPCV_FILTER_SCHARR( Horiz, Horiz, 8u16s )
IPCV_FILTER_SCHARR( Vert, Vert, 32f )
IPCV_FILTER_SCHARR( Horiz, Horiz, 32f )

#undef IPCV_FILTER_SCHARR

/****************************************************************************************\
*                                   Generic Filters                                      *
\****************************************************************************************/

#define IPCV_FILTER( suffix, ipp_suffix, cn, ksizetype, anchortype )                    \
IPCVAPI_EX( CvStatus, icvFilter##suffix##_C##cn##R,                                     \
            "ippiFilter" #ipp_suffix "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
            ( const void* src, int srcstep, void* dst, int dststep, CvSize size,        \
              const float* kernel, ksizetype ksize, anchortype anchor ))

IPCV_FILTER( _8u, 32f_8u, 1, CvSize, CvPoint )
IPCV_FILTER( _8u, 32f_8u, 3, CvSize, CvPoint )
IPCV_FILTER( _8u, 32f_8u, 4, CvSize, CvPoint )

IPCV_FILTER( _16s, 32f_16s, 1, CvSize, CvPoint )
IPCV_FILTER( _16s, 32f_16s, 3, CvSize, CvPoint )
IPCV_FILTER( _16s, 32f_16s, 4, CvSize, CvPoint )

IPCV_FILTER( _32f, _32f, 1, CvSize, CvPoint )
IPCV_FILTER( _32f, _32f, 3, CvSize, CvPoint )
IPCV_FILTER( _32f, _32f, 4, CvSize, CvPoint )

IPCV_FILTER( Column_8u, Column32f_8u, 1, int, int )
IPCV_FILTER( Column_8u, Column32f_8u, 3, int, int )
IPCV_FILTER( Column_8u, Column32f_8u, 4, int, int )

IPCV_FILTER( Column_16s, Column32f_16s, 1, int, int )
IPCV_FILTER( Column_16s, Column32f_16s, 3, int, int )
IPCV_FILTER( Column_16s, Column32f_16s, 4, int, int )

IPCV_FILTER( Column_32f, Column_32f, 1, int, int )
IPCV_FILTER( Column_32f, Column_32f, 3, int, int )
IPCV_FILTER( Column_32f, Column_32f, 4, int, int )

IPCV_FILTER( Row_8u, Row32f_8u, 1, int, int )
IPCV_FILTER( Row_8u, Row32f_8u, 3, int, int )
IPCV_FILTER( Row_8u, Row32f_8u, 4, int, int )

IPCV_FILTER( Row_16s, Row32f_16s, 1, int, int )
IPCV_FILTER( Row_16s, Row32f_16s, 3, int, int )
IPCV_FILTER( Row_16s, Row32f_16s, 4, int, int )

IPCV_FILTER( Row_32f, Row_32f, 1, int, int )
IPCV_FILTER( Row_32f, Row_32f, 3, int, int )
IPCV_FILTER( Row_32f, Row_32f, 4, int, int )

#undef IPCV_FILTER


/****************************************************************************************\
*                               Thresholding functions                                   *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvCompareC_8u_C1R_cv,
            "ippiCompareC_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* src1, int srcstep1, uchar scalar,
              uchar* dst, int dststep, CvSize size, int cmp_op ))
IPCVAPI_EX( CvStatus, icvAndC_8u_C1R,
            "ippiAndC_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* src1, int srcstep1, uchar scalar,
              uchar* dst, int dststep, CvSize size ))
IPCVAPI_EX( CvStatus, icvThreshold_GTVal_8u_C1R,
            "ippiThreshold_GTVal_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* pSrc, int srcstep, uchar* pDst, int dststep,
              CvSize size, uchar threshold, uchar value ))
IPCVAPI_EX( CvStatus, icvThreshold_GTVal_32f_C1R,
            "ippiThreshold_GTVal_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const float* pSrc, int srcstep, float* pDst, int dststep,
              CvSize size, float threshold, float value ))
IPCVAPI_EX( CvStatus, icvThreshold_LTVal_8u_C1R,
            "ippiThreshold_LTVal_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* pSrc, int srcstep, uchar* pDst, int dststep,
              CvSize size, uchar threshold, uchar value ))
IPCVAPI_EX( CvStatus, icvThreshold_LTVal_32f_C1R,
            "ippiThreshold_LTVal_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const float* pSrc, int srcstep, float* pDst, int dststep,
              CvSize size, float threshold, float value ))


#endif /*_CV_IPP_H_*/


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

#include "_cv.h"

static CvStatus CV_STDCALL
icvThresh_8u_C1R( const uchar* src, int src_step, uchar* dst, int dst_step,
                  CvSize roi, uchar thresh, uchar maxval, int type )
{
    int i, j;
    uchar tab[256];

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = maxval;
        break;
    case CV_THRESH_BINARY_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = maxval;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    case CV_THRESH_TRUNC:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = thresh;
        break;
    case CV_THRESH_TOZERO:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = (uchar)i;
        break;
    case CV_THRESH_TOZERO_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    default:
        return CV_BADFLAG_ERR;
    }

    for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
    {
        for( j = 0; j <= roi.width - 4; j += 4 )
        {
            uchar t0 = tab[src[j]];
            uchar t1 = tab[src[j+1]];

            dst[j] = t0;
            dst[j+1] = t1;

            t0 = tab[src[j+2]];
            t1 = tab[src[j+3]];

            dst[j+2] = t0;
            dst[j+3] = t1;
        }

        for( ; j < roi.width; j++ )
            dst[j] = tab[src[j]];
    }

    return CV_NO_ERR;
}


static CvStatus CV_STDCALL
icvThresh_32f_C1R( const float *src, int src_step, float *dst, int dst_step,
                   CvSize roi, float thresh, float maxval, int type )
{
    int i, j;
    const int* isrc = (const int*)src;
    int* idst = (int*)dst;
    Cv32suf v;
    int iThresh, iMax;

    v.f = thresh; iThresh = CV_TOGGLE_FLT(v.i);
    v.f = maxval; iMax = v.i;

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) <= iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_BINARY_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) > iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_TRUNC:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                float temp = src[j];

                if( temp > thresh )
                    temp = thresh;
                dst[j] = temp;
            }
        }
        break;

    case CV_THRESH_TOZERO:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) <= iThresh) - 1) & temp;
            }
        }
        break;

    case CV_THRESH_TOZERO_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) > iThresh) - 1) & temp;
            }
        }
        break;

    default:
        return CV_BADFLAG_ERR;
    }

    return CV_OK;
}


CV_IMPL void
cvThreshold( const void* srcarr, void* dstarr, double thresh, double maxval, int type )
{
    CvHistogram* hist = 0;

    CV_FUNCNAME( "cvThreshold" );

    __BEGIN__;

    CvSize roi;
    int src_step, dst_step;
    CvMat src_stub, *src = (CvMat*)srcarr;
    CvMat dst_stub, *dst = (CvMat*)dstarr;
    CvMat src0, dst0;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval, cn;
    bool use_otsu;

    CV_CALL( src = cvGetMat( src, &src_stub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dst_stub, &coi2 ));

    if( coi1 + coi2 )
        CV_ERROR( CV_BadCOI, "COI is not supported by the function" );

    if( !CV_ARE_CNS_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedFormats, "Both arrays must have equal number of channels" );

    cn = CV_MAT_CN(src->type);
    if( cn > 1 )
    {
        src = cvReshape( src, &src0, 1 );
        dst = cvReshape( dst, &dst0, 1 );
    }

    use_otsu = (type & ~CV_THRESH_MASK) == CV_THRESH_OTSU;
    type &= CV_THRESH_MASK;

    if( use_otsu )
    {
        assert("herelsp remove" && 0);
    }

    if( !CV_ARE_DEPTHS_EQ( src, dst ) )
    {
       CV_ERROR( CV_BadDepth, cvUnsupportedFormat );
    }

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    roi = cvGetMatSize( src );
    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }
    else
    {
        src_step = src->step;
        dst_step = dst->step;
    }

    switch( CV_MAT_DEPTH(src->type) )
    {
    case CV_8U:

        ithresh = cvFloor(thresh);
        imaxval = cvRound(maxval);
        if( type == CV_THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = CV_CAST_8U(imaxval);

        icvThresh_8u_C1R( src->data.ptr, src_step,
                          dst->data.ptr, dst_step, roi,
                          (uchar)ithresh, (uchar)imaxval, type );
        break;
    case CV_32F:


        icvThresh_32f_C1R( src->data.fl, src_step, dst->data.fl, dst_step, roi,
                           (float)thresh, (float)maxval, type );
        break;
    default:
        CV_ERROR( CV_BadDepth, cvUnsupportedFormat );
    }

    __END__;


}

/* End of file. */

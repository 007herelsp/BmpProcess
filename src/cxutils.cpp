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

#include "_cxcore.h"

CV_IMPL void cvNormalize( const CvArr* src, CvArr* dst,
                          double a, double b, int norm_type, const CvArr* mask )
{
    CvMat* tmp = 0;

    CV_FUNCNAME( "cvNormalize" );

    __BEGIN__;

    double scale, shift;
    
    if( norm_type == CV_MINMAX )
    {
        double smin = 0, smax = 0;
        double dmin = MIN( a, b ), dmax = MAX( a, b );
        cvMinMaxLoc( src, &smin, &smax, 0, 0, mask );
        scale = (dmax - dmin)*(smax - smin > DBL_EPSILON ? 1./(smax - smin) : 0);
        shift = dmin - smin*scale;
    }
    else if( norm_type == CV_L2 || norm_type == CV_L1 || norm_type == CV_C )
    {
        CvMat *s = (CvMat*)src, *d = (CvMat*)dst;
        
        if( CV_IS_MAT(s) && CV_IS_MAT(d) && CV_IS_MAT_CONT(s->type & d->type) &&
            CV_ARE_TYPES_EQ(s,d) && CV_ARE_SIZES_EQ(s,d) && !mask &&
            s->cols*s->rows <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
        {
            int i, len = s->cols*s->rows;
            double norm = 0, v;

            if( CV_MAT_TYPE(s->type) == CV_32FC1 )
            {
                const float* sptr = s->data.fl;
                float* dptr = d->data.fl;
                
                if( norm_type == CV_L2 )
                {
                    for( i = 0; i < len; i++ )
                    {
                        v = sptr[i];
                        norm += v*v;
                    }
                    norm = sqrt(norm);
                }
                else if( norm_type == CV_L1 )
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs((double)sptr[i]);
                        norm += v;
                    }
                else
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs((double)sptr[i]);
                        norm = MAX(norm,v);
                    }

                norm = norm > DBL_EPSILON ? 1./norm : 0.;
                for( i = 0; i < len; i++ )
                    dptr[i] = (float)(sptr[i]*norm);
                EXIT;
            }

            if( CV_MAT_TYPE(s->type) == CV_64FC1 )
            {
                const double* sptr = s->data.db;
                double* dptr = d->data.db;
                
                if( norm_type == CV_L2 )
                {
                    for( i = 0; i < len; i++ )
                    {
                        v = sptr[i];
                        norm += v*v;
                    }
                    norm = sqrt(norm);
                }
                else if( norm_type == CV_L1 )
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs(sptr[i]);
                        norm += v;
                    }
                else
                    for( i = 0; i < len; i++ )
                    {
                        v = fabs(sptr[i]);
                        norm = MAX(norm,v);
                    }

                norm = norm > DBL_EPSILON ? 1./norm : 0.;
                for( i = 0; i < len; i++ )
                    dptr[i] = sptr[i]*norm;
                EXIT;
            }
        }
        
        scale = cvNorm( src, 0, norm_type, mask );
        scale = scale > DBL_EPSILON ? 1./scale : 0.;
        shift = 0;
    }
    else
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported norm type" );
    
    if( !mask )
        cvConvertScale( src, dst, scale, shift );
    else
    {
        CvMat stub, *dmat;
        CV_CALL( dmat = cvGetMat(dst, &stub));
        CV_CALL( tmp = cvCreateMat(dmat->rows, dmat->cols, dmat->type) );
        cvConvertScale( src, tmp, scale, shift );
        cvCopy( tmp, dst, mask );
    }

    __END__;

    if( tmp )
        cvReleaseMat( &tmp );
}


CV_IMPL void cvRandShuffle( CvArr* arr, CvRNG* rng, double iter_factor )
{
    CV_FUNCNAME( "cvRandShuffle" );

    __BEGIN__;

    const int sizeof_int = (int)sizeof(int);
    CvMat stub, *mat = (CvMat*)arr;
    int i, j, k, iters, delta = 0;
    int cont_flag, arr_size, elem_size, cols, step;
    const int pair_buf_sz = 100;
    int* pair_buf = (int*)cvStackAlloc( pair_buf_sz*sizeof(pair_buf[0])*2 );
    CvMat _pair_buf = cvMat( 1, pair_buf_sz*2, CV_32S, pair_buf );
    CvRNG _rng = cvRNG(-1);
    uchar* data = 0;
    int* idata = 0;
    
    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !rng )
        rng = &_rng;

    cols = mat->cols;
    step = mat->step;
    arr_size = cols*mat->rows;
    iters = cvRound(iter_factor*arr_size)*2;
    cont_flag = CV_IS_MAT_CONT(mat->type);
    elem_size = CV_ELEM_SIZE(mat->type);
    if( elem_size % sizeof_int == 0 && (cont_flag || step % sizeof_int == 0) )
    {
        idata = mat->data.i;
        step /= sizeof_int;
        elem_size /= sizeof_int;
    }
    else
        data = mat->data.ptr;

    for( i = 0; i < iters; i += delta )
    {
        delta = MIN( iters - i, pair_buf_sz*2 );
        _pair_buf.cols = delta;
        cvRandArr( rng, &_pair_buf, CV_RAND_UNI, cvRealScalar(0), cvRealScalar(arr_size) );
        
        if( cont_flag )
        {
            if( idata )
                for( j = 0; j < delta; j += 2 )
                {
                    int* p = idata + pair_buf[j]*elem_size, *q = idata + pair_buf[j+1]*elem_size, t;
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
            else
                for( j = 0; j < delta; j += 2 )
                {
                    uchar* p = data + pair_buf[j]*elem_size, *q = data + pair_buf[j+1]*elem_size, t;
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
        }
        else
        {
            if( idata )
                for( j = 0; j < delta; j += 2 )
                {
                    int idx1 = pair_buf[j], idx2 = pair_buf[j+1], row1, row2;
                    int* p, *q, t;
                    row1 = idx1/step; row2 = idx2/step;
                    p = idata + row1*step + (idx1 - row1*cols)*elem_size;
                    q = idata + row2*step + (idx2 - row2*cols)*elem_size;
                    
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
            else
                for( j = 0; j < delta; j += 2 )
                {
                    int idx1 = pair_buf[j], idx2 = pair_buf[j+1], row1, row2;
                    uchar* p, *q, t;
                    row1 = idx1/step; row2 = idx2/step;
                    p = data + row1*step + (idx1 - row1*cols)*elem_size;
                    q = data + row2*step + (idx2 - row2*cols)*elem_size;
                    
                    for( k = 0; k < elem_size; k++ )
                        CV_SWAP( p[k], q[k], t );
                }
        }
    }

    __END__;
}


CV_IMPL CvArr*
cvRange( CvArr* arr, double start, double end )
{
    int ok = 0;
    
    CV_FUNCNAME( "cvRange" );

    __BEGIN__;
    
    CvMat stub, *mat = (CvMat*)arr;
    double delta;
    int type, step;
    double val = start;
    int i, j;
    int rows, cols;
    
    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub) );

    rows = mat->rows;
    cols = mat->cols;
    type = CV_MAT_TYPE(mat->type);
    delta = (end-start)/(rows*cols);

    if( CV_IS_MAT_CONT(mat->type) )
    {
        cols *= rows;
        rows = 1;
        step = 1;
    }
    else
        step = mat->step / CV_ELEM_SIZE(type);

    if( type == CV_32SC1 )
    {
        int* idata = mat->data.i;
        int ival = cvRound(val), idelta = cvRound(delta);

        if( fabs(val - ival) < DBL_EPSILON &&
            fabs(delta - idelta) < DBL_EPSILON )
        {
            for( i = 0; i < rows; i++, idata += step )
                for( j = 0; j < cols; j++, ival += idelta )
                    idata[j] = ival;
        }
        else
        {
            for( i = 0; i < rows; i++, idata += step )
                for( j = 0; j < cols; j++, val += delta )
                    idata[j] = cvRound(val);
        }
    }
    else if( type == CV_32FC1 )
    {
        float* fdata = mat->data.fl;
        for( i = 0; i < rows; i++, fdata += step )
            for( j = 0; j < cols; j++, val += delta )
                fdata[j] = (float)val;
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "The function only supports 32sC1 and 32fC1 datatypes" );

    ok = 1;

    __END__;

    return ok ? arr : 0;
}

/* End of file. */

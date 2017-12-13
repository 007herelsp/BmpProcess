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
#include "imgproc.precomp.hpp"

/* calculates length of a curve (e.g. contour perimeter) */
CV_IMPL  double
cvArcLength( const void *array, CvSlice slice, int is_closed )
{
    double perimeter = 0;

    int i, j = 0, count;
    const int N = 16;
    float buf[N];
    CvMat buffer = cvMat( 1, N, CV_32F, buf );
    CvSeqReader reader;
    CvContour contour_header;
    CvSeq* contour = 0;
    CvSeqBlock block;

    if( CV_IS_SEQ( array ))
    {
        contour = (CvSeq*)array;
        if( !CV_IS_SEQ_POLYLINE( contour ))
            CV_Error( CV_StsBadArg, "Unsupported sequence type" );
        if( is_closed < 0 )
            is_closed = CV_IS_SEQ_CLOSED( contour );
    }
    else
    {
        is_closed = is_closed > 0;
        contour = cvPointSeqFromMat(
            CV_SEQ_KIND_CURVE | (is_closed ? CV_SEQ_FLAG_CLOSED : 0),
            array, &contour_header, &block );
    }

    if( contour->total > 1 )
    {
        int is_float = CV_SEQ_ELTYPE( contour ) == CV_32FC2;

        cvStartReadSeq( contour, &reader, 0 );
        cvSetSeqReaderPos( &reader, slice.start_index );
        count = cvSliceLength( slice, contour );

        count -= !is_closed && count == contour->total;

        /* scroll the reader by 1 point */
        reader.prev_elem = reader.ptr;
        CV_NEXT_SEQ_ELEM( sizeof(CvPoint), reader );

        for( i = 0; i < count; i++ )
        {
            float dx, dy;

            if( !is_float )
            {
                CvPoint* pt = (CvPoint*)reader.ptr;
                CvPoint* prev_pt = (CvPoint*)reader.prev_elem;

                dx = (float)pt->x - (float)prev_pt->x;
                dy = (float)pt->y - (float)prev_pt->y;
            }
            else
            {
                CvPoint2D32f* pt = (CvPoint2D32f*)reader.ptr;
                CvPoint2D32f* prev_pt = (CvPoint2D32f*)reader.prev_elem;

                dx = pt->x - prev_pt->x;
                dy = pt->y - prev_pt->y;
            }

            reader.prev_elem = reader.ptr;
            CV_NEXT_SEQ_ELEM( contour->elem_size, reader );
            // Bugfix by Axel at rubico.com 2010-03-22, affects closed slices only
            // wraparound not handled by CV_NEXT_SEQ_ELEM
            if( is_closed && i == count - 2 )
                cvSetSeqReaderPos( &reader, slice.start_index );

            buffer.data.fl[j] = dx * dx + dy * dy;
            if( ++j == N || i == count - 1 )
            {
                buffer.cols = j;
                cvPow( &buffer, &buffer, 0.5 );
                for( ; j > 0; j-- )
                    perimeter += buffer.data.fl[j-1];
            }
        }
    }

    return perimeter;
}


static CvStatus
icvFindCircle( CvPoint2D32f pt0, CvPoint2D32f pt1,
               CvPoint2D32f pt2, CvPoint2D32f * center, float *radius )
{
    double x1 = (pt0.x + pt1.x) * 0.5;
    double dy1 = pt0.x - pt1.x;
    double x2 = (pt1.x + pt2.x) * 0.5;
    double dy2 = pt1.x - pt2.x;
    double y1 = (pt0.y + pt1.y) * 0.5;
    double dx1 = pt1.y - pt0.y;
    double y2 = (pt1.y + pt2.y) * 0.5;
    double dx2 = pt2.y - pt1.y;
    double t = 0;

    CvStatus result = CV_OK;

    if( icvIntersectLines( x1, dx1, y1, dy1, x2, dx2, y2, dy2, &t ) >= 0 )
    {
        center->x = (float) (x2 + dx2 * t);
        center->y = (float) (y2 + dy2 * t);
        *radius = (float) icvDistanceL2_32f( *center, pt0 );
    }
    else
    {
        center->x = center->y = 0.f;
        radius = 0;
        result = CV_NOTDEFINED_ERR;
    }

    return result;
}


CV_INLINE double icvIsPtInCircle( CvPoint2D32f pt, CvPoint2D32f center, float radius )
{
    double dx = pt.x - center.x;
    double dy = pt.y - center.y;
    return (double)radius*radius - dx*dx - dy*dy;
}


static int
icvFindEnslosingCicle4pts_32f( CvPoint2D32f * pts, CvPoint2D32f * _center, float *_radius )
{
    int shuffles[4][4] = { {0, 1, 2, 3}, {0, 1, 3, 2}, {2, 3, 0, 1}, {2, 3, 1, 0} };

    int idxs[4] = { 0, 1, 2, 3 };
    int i, j, k = 1, mi = 0;
    float max_dist = 0;
    CvPoint2D32f center;
    CvPoint2D32f min_center;
    float radius, min_radius = FLT_MAX;
    CvPoint2D32f res_pts[4];

    center = min_center = pts[0];
    radius = 1.f;

    for( i = 0; i < 4; i++ )
        for( j = i + 1; j < 4; j++ )
        {
            float dist = icvDistanceL2_32f( pts[i], pts[j] );

            if( max_dist < dist )
            {
                max_dist = dist;
                idxs[0] = i;
                idxs[1] = j;
            }
        }

    if( max_dist == 0 )
        goto function_exit;

    k = 2;
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < k; j++ )
            if( i == idxs[j] )
                break;
        if( j == k )
            idxs[k++] = i;
    }

    center = cvPoint2D32f( (pts[idxs[0]].x + pts[idxs[1]].x)*0.5f,
                           (pts[idxs[0]].y + pts[idxs[1]].y)*0.5f );
    radius = (float)(icvDistanceL2_32f( pts[idxs[0]], center )*1.03);
    if( radius < 1.f )
        radius = 1.f;

    if( icvIsPtInCircle( pts[idxs[2]], center, radius ) >= 0 &&
        icvIsPtInCircle( pts[idxs[3]], center, radius ) >= 0 )
    {
        k = 2; //rand()%2+2;
    }
    else
    {
        mi = -1;
        for( i = 0; i < 4; i++ )
        {
            if( icvFindCircle( pts[shuffles[i][0]], pts[shuffles[i][1]],
                               pts[shuffles[i][2]], &center, &radius ) >= 0 )
            {
                radius *= 1.03f;
                if( radius < 2.f )
                    radius = 2.f;

                if( icvIsPtInCircle( pts[shuffles[i][3]], center, radius ) >= 0 &&
                    min_radius > radius )
                {
                    min_radius = radius;
                    min_center = center;
                    mi = i;
                }
            }
        }
        assert( mi >= 0 );
        if( mi < 0 )
            mi = 0;
        k = 3;
        center = min_center;
        radius = min_radius;
        for( i = 0; i < 4; i++ )
            idxs[i] = shuffles[mi][i];
    }

  function_exit:

    *_center = center;
    *_radius = radius;

    /* reorder output points */
    for( i = 0; i < 4; i++ )
        res_pts[i] = pts[idxs[i]];

    for( i = 0; i < 4; i++ )
    {
        pts[i] = res_pts[i];
        assert( icvIsPtInCircle( pts[i], center, radius ) >= 0 );
    }

    return k;
}



/* area of a whole sequence */
static CvStatus
icvContourArea( const CvSeq* contour, double *area )
{
    if( contour->total )
    {
        CvSeqReader reader;
        int lpt = contour->total;
        double a00 = 0, xi_1, yi_1;
        int is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;

        cvStartReadSeq( contour, &reader, 0 );

        if( !is_float )
        {
            xi_1 = ((CvPoint*)(reader.ptr))->x;
            yi_1 = ((CvPoint*)(reader.ptr))->y;
        }
        else
        {
            xi_1 = ((CvPoint2D32f*)(reader.ptr))->x;
            yi_1 = ((CvPoint2D32f*)(reader.ptr))->y;
        }
        CV_NEXT_SEQ_ELEM( contour->elem_size, reader );

        while( lpt-- > 0 )
        {
            double dxy, xi, yi;

            if( !is_float )
            {
                xi = ((CvPoint*)(reader.ptr))->x;
                yi = ((CvPoint*)(reader.ptr))->y;
            }
            else
            {
                xi = ((CvPoint2D32f*)(reader.ptr))->x;
                yi = ((CvPoint2D32f*)(reader.ptr))->y;
            }
            CV_NEXT_SEQ_ELEM( contour->elem_size, reader );

            dxy = xi_1 * yi - xi * yi_1;
            a00 += dxy;
            xi_1 = xi;
            yi_1 = yi;
        }

        *area = a00 * 0.5;
    }
    else
        *area = 0;

    return CV_OK;
}


/****************************************************************************************\

 copy data from one buffer to other buffer

\****************************************************************************************/

static CvStatus
icvMemCopy( double **buf1, double **buf2, double **buf3, int *b_max )
{
    int bb;

    if( (*buf1 == NULL && *buf2 == NULL) || *buf3 == NULL )
        return CV_NULLPTR_ERR;

    bb = *b_max;
    if( *buf2 == NULL )
    {
        *b_max = 2 * (*b_max);
        *buf2 = (double *)cvAlloc( (*b_max) * sizeof( double ));

        if( *buf2 == NULL )
            return CV_OUTOFMEM_ERR;

        memcpy( *buf2, *buf3, bb * sizeof( double ));

        *buf3 = *buf2;
        cvFree( buf1 );
        *buf1 = NULL;
    }
    else
    {
        *b_max = 2 * (*b_max);
        *buf1 = (double *) cvAlloc( (*b_max) * sizeof( double ));

        if( *buf1 == NULL )
            return CV_OUTOFMEM_ERR;

        memcpy( *buf1, *buf3, bb * sizeof( double ));

        *buf3 = *buf1;
        cvFree( buf2 );
        *buf2 = NULL;
    }
    return CV_OK;
}


/* area of a contour sector */
static CvStatus icvContourSecArea( CvSeq * contour, CvSlice slice, double *area )
{
    CvPoint pt;                 /*  pointer to points   */
    CvPoint pt_s, pt_e;         /*  first and last points  */
    CvSeqReader reader;         /*  points reader of contour   */

    int p_max = 2, p_ind;
    int lpt, flag, i;
    double a00;                 /* unnormalized moments m00    */
    double xi, yi, xi_1, yi_1, x0, y0, dxy, sk, sk1, t;
    double x_s, y_s, nx, ny, dx, dy, du, dv;
    double eps = 1.e-5;
    double *p_are1, *p_are2, *p_are;

    assert( contour != NULL );

    if( contour == NULL )
        return CV_NULLPTR_ERR;

    if( !CV_IS_SEQ_POINT_SET( contour ))
        return CV_BADFLAG_ERR;

    lpt = cvSliceLength( slice, contour );
    /*if( n2 >= n1 )
        lpt = n2 - n1 + 1;
    else
        lpt = contour->total - n1 + n2 + 1;*/

    if( contour->total && lpt > 2 )
    {
        a00 = x0 = y0 = xi_1 = yi_1 = 0;
        sk1 = 0;
        flag = 0;
        dxy = 0;
        p_are1 = (double *) cvAlloc( p_max * sizeof( double ));

        if( p_are1 == NULL )
            return CV_OUTOFMEM_ERR;

        p_are = p_are1;
        p_are2 = NULL;

        cvStartReadSeq( contour, &reader, 0 );
        cvSetSeqReaderPos( &reader, slice.start_index );
        CV_READ_SEQ_ELEM( pt_s, reader );
        p_ind = 0;
        cvSetSeqReaderPos( &reader, slice.end_index );
        CV_READ_SEQ_ELEM( pt_e, reader );

/*    normal coefficients    */
        nx = pt_s.y - pt_e.y;
        ny = pt_e.x - pt_s.x;
        cvSetSeqReaderPos( &reader, slice.start_index );

        while( lpt-- > 0 )
        {
            CV_READ_SEQ_ELEM( pt, reader );

            if( flag == 0 )
            {
                xi_1 = (double) pt.x;
                yi_1 = (double) pt.y;
                x0 = xi_1;
                y0 = yi_1;
                sk1 = 0;
                flag = 1;
            }
            else
            {
                xi = (double) pt.x;
                yi = (double) pt.y;

/****************   edges intersection examination   **************************/
                sk = nx * (xi - pt_s.x) + ny * (yi - pt_s.y);
                if( (fabs( sk ) < eps && lpt > 0) || sk * sk1 < -eps )
                {
                    if( fabs( sk ) < eps )
                    {
                        dxy = xi_1 * yi - xi * yi_1;
                        a00 = a00 + dxy;
                        dxy = xi * y0 - x0 * yi;
                        a00 = a00 + dxy;

                        if( p_ind >= p_max )
                            icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

                        p_are[p_ind] = a00 / 2.;
                        p_ind++;
                        a00 = 0;
                        sk1 = 0;
                        x0 = xi;
                        y0 = yi;
                        dxy = 0;
                    }
                    else
                    {
/*  define intersection point    */
                        dv = yi - yi_1;
                        du = xi - xi_1;
                        dx = ny;
                        dy = -nx;
                        if( fabs( du ) > eps )
                            t = ((yi_1 - pt_s.y) * du + dv * (pt_s.x - xi_1)) /
                                (du * dy - dx * dv);
                        else
                            t = (xi_1 - pt_s.x) / dx;
                        if( t > eps && t < 1 - eps )
                        {
                            x_s = pt_s.x + t * dx;
                            y_s = pt_s.y + t * dy;
                            dxy = xi_1 * y_s - x_s * yi_1;
                            a00 += dxy;
                            dxy = x_s * y0 - x0 * y_s;
                            a00 += dxy;
                            if( p_ind >= p_max )
                                icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

                            p_are[p_ind] = a00 / 2.;
                            p_ind++;

                            a00 = 0;
                            sk1 = 0;
                            x0 = x_s;
                            y0 = y_s;
                            dxy = x_s * yi - xi * y_s;
                        }
                    }
                }
                else
                    dxy = xi_1 * yi - xi * yi_1;

                a00 += dxy;
                xi_1 = xi;
                yi_1 = yi;
                sk1 = sk;

            }
        }

        xi = x0;
        yi = y0;
        dxy = xi_1 * yi - xi * yi_1;

        a00 += dxy;

        if( p_ind >= p_max )
            icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

        p_are[p_ind] = a00 / 2.;
        p_ind++;

/*     common area calculation    */
        *area = 0;
        for( i = 0; i < p_ind; i++ )
            (*area) += fabs( p_are[i] );

        if( p_are1 != NULL )
            cvFree( &p_are1 );
        else if( p_are2 != NULL )
            cvFree( &p_are2 );

        return CV_OK;
    }
    else
        return CV_BADSIZE_ERR;
}


/* external contour area function */
CV_IMPL double
cvContourArea( const void *array, CvSlice slice, int oriented )
{
    double area = 0;

    CvContour contour_header;
    CvSeq* contour = 0;
    CvSeqBlock block;

    if( CV_IS_SEQ( array ))
    {
        contour = (CvSeq*)array;
        if( !CV_IS_SEQ_POLYLINE( contour ))
            CV_Error( CV_StsBadArg, "Unsupported sequence type" );
    }
    else
    {
        contour = cvPointSeqFromMat( CV_SEQ_KIND_CURVE, array, &contour_header, &block );
    }

    if( cvSliceLength( slice, contour ) == contour->total )
    {
        IPPI_CALL( icvContourArea( contour, &area ));
    }
    else
    {
        if( CV_SEQ_ELTYPE( contour ) != CV_32SC2 )
            CV_Error( CV_StsUnsupportedFormat,
            "Only curves with integer coordinates are supported in case of contour slice" );
        IPPI_CALL( icvContourSecArea( contour, slice, &area ));
    }

    return oriented ? area : fabs(area);
}




/* Calculates bounding rectagnle of a point set or retrieves already calculated */
CV_IMPL  CvRect
cvBoundingRect( CvArr* array, int update )
{
    CvSeqReader reader;
    CvRect  rect = { 0, 0, 0, 0 };
    CvContour contour_header;
    CvSeq* ptseq = 0;
    CvSeqBlock block;

    CvMat stub, *mat = 0;
    int  xmin = 0, ymin = 0, xmax = -1, ymax = -1, i, j, k;
    int calculate = update;

    if( CV_IS_SEQ( array ))
    {
        ptseq = (CvSeq*)array;
        if( !CV_IS_SEQ_POINT_SET( ptseq ))
            CV_Error( CV_StsBadArg, "Unsupported sequence type" );

        if( ptseq->header_size < (int)sizeof(CvContour))
        {
            update = 0;
            calculate = 1;
        }
    }
    else
    {
        mat = cvGetMat( array, &stub );
        if( CV_MAT_TYPE(mat->type) == CV_32SC2 ||
            CV_MAT_TYPE(mat->type) == CV_32FC2 )
        {
            ptseq = cvPointSeqFromMat(CV_SEQ_KIND_GENERIC, mat, &contour_header, &block);
            mat = 0;
        }
        else if( CV_MAT_TYPE(mat->type) != CV_8UC1 &&
                CV_MAT_TYPE(mat->type) != CV_8SC1 )
            CV_Error( CV_StsUnsupportedFormat,
                "The image/matrix format is not supported by the function" );
        update = 0;
        calculate = 1;
    }

    if( !calculate )
        return ((CvContour*)ptseq)->rect;

    if( mat )
    {
        CvSize size = cvGetMatSize(mat);
        xmin = size.width;
        ymin = -1;

        for( i = 0; i < size.height; i++ )
        {
            uchar* _ptr = mat->data.ptr + i*mat->step;
            uchar* ptr = (uchar*)cvAlignPtr(_ptr, 4);
            int have_nz = 0, k_min, offset = (int)(ptr - _ptr);
            j = 0;
            offset = MIN(offset, size.width);
            for( ; j < offset; j++ )
                if( _ptr[j] )
                {
                    have_nz = 1;
                    break;
                }
            if( j < offset )
            {
                if( j < xmin )
                    xmin = j;
                if( j > xmax )
                    xmax = j;
            }
            if( offset < size.width )
            {
                xmin -= offset;
                xmax -= offset;
                size.width -= offset;
                j = 0;
                for( ; j <= xmin - 4; j += 4 )
                    if( *((int*)(ptr+j)) )
                        break;
                for( ; j < xmin; j++ )
                    if( ptr[j] )
                    {
                        xmin = j;
                        if( j > xmax )
                            xmax = j;
                        have_nz = 1;
                        break;
                    }
                k_min = MAX(j-1, xmax);
                k = size.width - 1;
                for( ; k > k_min && (k&3) != 3; k-- )
                    if( ptr[k] )
                        break;
                if( k > k_min && (k&3) == 3 )
                {
                    for( ; k > k_min+3; k -= 4 )
                        if( *((int*)(ptr+k-3)) )
                            break;
                }
                for( ; k > k_min; k-- )
                    if( ptr[k] )
                    {
                        xmax = k;
                        have_nz = 1;
                        break;
                    }
                if( !have_nz )
                {
                    j &= ~3;
                    for( ; j <= k - 3; j += 4 )
                        if( *((int*)(ptr+j)) )
                            break;
                    for( ; j <= k; j++ )
                        if( ptr[j] )
                        {
                            have_nz = 1;
                            break;
                        }
                }
                xmin += offset;
                xmax += offset;
                size.width += offset;
            }
            if( have_nz )
            {
                if( ymin < 0 )
                    ymin = i;
                ymax = i;
            }
        }

        if( xmin >= size.width )
            xmin = ymin = 0;
    }
    else if( ptseq->total )
    {
        int  is_float = CV_SEQ_ELTYPE(ptseq) == CV_32FC2;
        cvStartReadSeq( ptseq, &reader, 0 );
        CvPoint pt;
        CV_READ_SEQ_ELEM( pt, reader );
    #if CV_SSE4_2
        if(cv::checkHardwareSupport(CV_CPU_SSE4_2))
        {
            if( !is_float )
            {
                __m128i minval, maxval;
                minval = maxval = _mm_loadl_epi64((const __m128i*)(&pt)); //min[0]=pt.x, min[1]=pt.y

                for( i = 1; i < ptseq->total; i++)
                {
                    __m128i ptXY = _mm_loadl_epi64((const __m128i*)(reader.ptr));
                    CV_NEXT_SEQ_ELEM(sizeof(pt), reader);
                    minval = _mm_min_epi32(ptXY, minval);
                    maxval = _mm_max_epi32(ptXY, maxval);
                }
                xmin = _mm_cvtsi128_si32(minval);
                ymin = _mm_cvtsi128_si32(_mm_srli_si128(minval, 4));
                xmax = _mm_cvtsi128_si32(maxval);
                ymax = _mm_cvtsi128_si32(_mm_srli_si128(maxval, 4));
            }
            else
            {
                __m128 minvalf, maxvalf, z = _mm_setzero_ps(), ptXY = _mm_setzero_ps();
                minvalf = maxvalf = _mm_loadl_pi(z, (const __m64*)(&pt));

                for( i = 1; i < ptseq->total; i++ )
                {
                    ptXY = _mm_loadl_pi(ptXY, (const __m64*)reader.ptr);
                    CV_NEXT_SEQ_ELEM(sizeof(pt), reader);

                    minvalf = _mm_min_ps(minvalf, ptXY);
                    maxvalf = _mm_max_ps(maxvalf, ptXY);
                }

                float xyminf[2], xymaxf[2];
                _mm_storel_pi((__m64*)xyminf, minvalf);
                _mm_storel_pi((__m64*)xymaxf, maxvalf);
                xmin = cvFloor(xyminf[0]);
                ymin = cvFloor(xyminf[1]);
                xmax = cvFloor(xymaxf[0]);
                ymax = cvFloor(xymaxf[1]);
            }
        }
        else
    #endif
        {
            if( !is_float )
            {
                xmin = xmax = pt.x;
                ymin = ymax = pt.y;

                for( i = 1; i < ptseq->total; i++ )
                {
                    CV_READ_SEQ_ELEM( pt, reader );

                    if( xmin > pt.x )
                        xmin = pt.x;

                    if( xmax < pt.x )
                        xmax = pt.x;

                    if( ymin > pt.y )
                        ymin = pt.y;

                    if( ymax < pt.y )
                        ymax = pt.y;
                }
            }
            else
            {
                Cv32suf v;
                // init values
                xmin = xmax = CV_TOGGLE_FLT(pt.x);
                ymin = ymax = CV_TOGGLE_FLT(pt.y);

                for( i = 1; i < ptseq->total; i++ )
                {
                    CV_READ_SEQ_ELEM( pt, reader );
                    pt.x = CV_TOGGLE_FLT(pt.x);
                    pt.y = CV_TOGGLE_FLT(pt.y);

                    if( xmin > pt.x )
                        xmin = pt.x;

                    if( xmax < pt.x )
                        xmax = pt.x;

                    if( ymin > pt.y )
                        ymin = pt.y;

                    if( ymax < pt.y )
                        ymax = pt.y;
                }

                v.i = CV_TOGGLE_FLT(xmin); xmin = cvFloor(v.f);
                v.i = CV_TOGGLE_FLT(ymin); ymin = cvFloor(v.f);
                // because right and bottom sides of the bounding rectangle are not inclusive
                // (note +1 in width and height calculation below), cvFloor is used here instead of cvCeil
                v.i = CV_TOGGLE_FLT(xmax); xmax = cvFloor(v.f);
                v.i = CV_TOGGLE_FLT(ymax); ymax = cvFloor(v.f);
            }
        }
        rect.x = xmin;
        rect.y = ymin;
        rect.width = xmax - xmin + 1;
        rect.height = ymax - ymin + 1;
    }
    if( update )
        ((CvContour*)ptseq)->rect = rect;
    return rect;
}

/* End of file. */

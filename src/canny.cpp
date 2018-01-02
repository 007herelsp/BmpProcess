

#include "_cv.h"

static void
cvSobel( const void* srcarr, void* dstarr, int dx, int dy, int aperture_size )
{
    CvSepFilter filter;
    void* buffer = 0;
    int local_alloc = 0;

    VOS_FUNCNAME( "cvSobel" );

    __BEGIN__;

    int origin = 0;
    int src_type, dst_type;
    Mat srcstub, *src = (Mat*)srcarr;
    Mat dststub, *dst = (Mat*)dstarr;

    if( !VOS_IS_MAT(src) )
        VOS_CALL( src = GetMat( src, &srcstub ));
    if( !VOS_IS_MAT(dst) )
        VOS_CALL( dst = GetMat( dst, &dststub ));

    if( VOS_IS_IMAGE_HDR( srcarr ))
        origin = ((IplImage*)srcarr)->origin;

    src_type = VOS_MAT_TYPE( src->type );
    dst_type = VOS_MAT_TYPE( dst->type );

    if( !VOS_ARE_SIZES_EQ( src, dst ))
        VOS_ERROR( VOS_StsBadArg, "src and dst have different sizes" );

    VOS_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    VOS_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}



 void
Canny( const void* srcarr, void* dstarr,
         double low_thresh, double high_thresh, int aperture_size )
{
    Mat *dx = 0, *dy = 0;
    void *buffer = 0;
    uchar **stack_top, **stack_bottom = 0;

    VOS_FUNCNAME( "Canny" );

    __BEGIN__;

    Mat srcstub, *src = (Mat*)srcarr;
    Mat dststub, *dst = (Mat*)dstarr;
    Size size;
    int flags = aperture_size;
    int low, high;
    int* mag_buf[3];
    uchar* map;
    int mapstep, maxsize;
    int i, j;
    Mat mag_row;

    VOS_CALL( src = GetMat( src, &srcstub ));
    VOS_CALL( dst = GetMat( dst, &dststub ));

    if( VOS_MAT_TYPE( src->type ) != VOS_8UC1 ||
        VOS_MAT_TYPE( dst->type ) != VOS_8UC1 )
        VOS_ERROR( VOS_StsUnsupportedFormat, "" );

    if( !VOS_ARE_SIZES_EQ( src, dst ))
        VOS_ERROR( VOS_StsUnmatchedSizes, "" );

    if( low_thresh > high_thresh )
    {
        double t;
        VOS_SWAP( low_thresh, high_thresh, t );
    }

    aperture_size &= INT_MAX;
    if( (aperture_size & 1) == 0 || aperture_size < 3 || aperture_size > 7 )
        VOS_ERROR( VOS_StsBadFlag, "" );

    size = cvGetMatSize( src );

    dx = CreateMat( size.height, size.width, VOS_16SC1 );
    dy = CreateMat( size.height, size.width, VOS_16SC1 );
    cvSobel( src, dx, 1, 0, aperture_size );
    cvSobel( src, dy, 0, 1, aperture_size );


    if( flags & VOS_CANNY_L2_GRADIENT )
    {
        Cv32suf ul, uh;
        ul.f = (float)low_thresh;
        uh.f = (float)high_thresh;

        low = ul.i;
        high = uh.i;
    }
    else
    {
        low = cvFloor( low_thresh );
        high = cvFloor( high_thresh );
    }

    VOS_CALL( buffer = cvAlloc( (size.width+2)*(size.height+2) +
                                (size.width+2)*3*sizeof(int)) );

    mag_buf[0] = (int*)buffer;
    mag_buf[1] = mag_buf[0] + size.width + 2;
    mag_buf[2] = mag_buf[1] + size.width + 2;
    map = (uchar*)(mag_buf[2] + size.width + 2);
    mapstep = size.width + 2;

    maxsize = MAX( 1 << 10, size.width*size.height/10 );
    VOS_CALL( stack_top = stack_bottom = (uchar**)cvAlloc( maxsize*sizeof(stack_top[0]) ));

    memset( mag_buf[0], 0, (size.width+2)*sizeof(int) );
    memset( map, 1, mapstep );
    memset( map + mapstep*(size.height + 1), 1, mapstep );

    /* sector numbers
       (Top-Left Origin)

        1   2   3
         *  *  *
          * * *
        0*******0
          * * *
         *  *  *
        3   2   1
    */

    #define CANNY_PUSH(d)    *(d) = (uchar)2, *stack_top++ = (d)
    #define CANNY_POP(d)     (d) = *--stack_top

    mag_row = cvMat( 1, size.width, VOS_32F );

    // calculate magnitude and angle of gradient, perform non-maxima supression.
    // fill the map with one of the following values:
    //   0 - the pixel might belong to an edge
    //   1 - the pixel can not belong to an edge
    //   2 - the pixel does belong to an edge
    for( i = 0; i <= size.height; i++ )
    {
        int* _mag = mag_buf[(i > 0) + 1] + 1;
        float* _magf = (float*)_mag;
        const short* _dx = (short*)(dx->data.ptr + dx->step*i);
        const short* _dy = (short*)(dy->data.ptr + dy->step*i);
        uchar* _map;
        int x, y;
        int magstep1, magstep2;
        int prev_flag = 0;

        if( i < size.height )
        {
            _mag[-1] = _mag[size.width] = 0;

            if( !(flags & VOS_CANNY_L2_GRADIENT) )
                for( j = 0; j < size.width; j++ )
                    _mag[j] = abs(_dx[j]) + abs(_dy[j]);
            else if( 0 != 0 ) // check for IPP
            {
                // use vectorized sqrt
                mag_row.data.fl = _magf;
                for( j = 0; j < size.width; j++ )
                {
                    x = _dx[j]; y = _dy[j];
                    _magf[j] = (float)((double)x*x + (double)y*y);
                }
                cvPow( &mag_row, &mag_row, 0.5 );
            }
            else
            {
                for( j = 0; j < size.width; j++ )
                {
                    x = _dx[j]; y = _dy[j];
                    _magf[j] = (float)sqrt((double)x*x + (double)y*y);
                }
            }
        }
        else
            memset( _mag-1, 0, (size.width + 2)*sizeof(int) );

        // at the very beginning we do not have a complete ring
        // buffer of 3 magnitude rows for non-maxima suppression
        if( i == 0 )
            continue;

        _map = map + mapstep*i + 1;
        _map[-1] = _map[size.width] = 1;

        _mag = mag_buf[1] + 1; // take the central row
        _dx = (short*)(dx->data.ptr + dx->step*(i-1));
        _dy = (short*)(dy->data.ptr + dy->step*(i-1));

        magstep1 = (int)(mag_buf[2] - mag_buf[1]);
        magstep2 = (int)(mag_buf[0] - mag_buf[1]);

        if( (stack_top - stack_bottom) + size.width > maxsize )
        {
            uchar** new_stack_bottom;
            maxsize = MAX( maxsize * 3/2, maxsize + size.width );
            VOS_CALL( new_stack_bottom = (uchar**)cvAlloc( maxsize * sizeof(stack_top[0])) );
            memcpy( new_stack_bottom, stack_bottom, (stack_top - stack_bottom)*sizeof(stack_top[0]) );
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            cvFree( &stack_bottom );
            stack_bottom = new_stack_bottom;
        }

        for( j = 0; j < size.width; j++ )
        {
            #define CANNY_SHIFT 15
            #define TG22  (int)(0.4142135623730950488016887242097*(1<<CANNY_SHIFT) + 0.5)

            x = _dx[j];
            y = _dy[j];
            int s = x ^ y;
            int m = _mag[j];

            x = abs(x);
            y = abs(y);
            if( m > low )
            {
                int tg22x = x * TG22;
                int tg67x = tg22x + ((x + x) << CANNY_SHIFT);

                y <<= CANNY_SHIFT;

                if( y < tg22x )
                {
                    if( m > _mag[j-1] && m >= _mag[j+1] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else if( y > tg67x )
                {
                    if( m > _mag[j+magstep2] && m >= _mag[j+magstep1] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else
                {
                    s = s < 0 ? -1 : 1;
                    if( m > _mag[j+magstep2-s] && m > _mag[j+magstep1+s] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
            }
            prev_flag = 0;
            _map[j] = (uchar)1;
        }

        // scroll the ring buffer
        _mag = mag_buf[0];
        mag_buf[0] = mag_buf[1];
        mag_buf[1] = mag_buf[2];
        mag_buf[2] = _mag;
    }

    // now track the edges (hysteresis thresholding)
    while( stack_top > stack_bottom )
    {
        uchar* m;
        if( (stack_top - stack_bottom) + 8 > maxsize )
        {
            uchar** new_stack_bottom;
            maxsize = MAX( maxsize * 3/2, maxsize + 8 );
            VOS_CALL( new_stack_bottom = (uchar**)cvAlloc( maxsize * sizeof(stack_top[0])) );
            memcpy( new_stack_bottom, stack_bottom, (stack_top - stack_bottom)*sizeof(stack_top[0]) );
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            cvFree( &stack_bottom );
            stack_bottom = new_stack_bottom;
        }

        CANNY_POP(m);

        if( !m[-1] )
            CANNY_PUSH( m - 1 );
        if( !m[1] )
            CANNY_PUSH( m + 1 );
        if( !m[-mapstep-1] )
            CANNY_PUSH( m - mapstep - 1 );
        if( !m[-mapstep] )
            CANNY_PUSH( m - mapstep );
        if( !m[-mapstep+1] )
            CANNY_PUSH( m - mapstep + 1 );
        if( !m[mapstep-1] )
            CANNY_PUSH( m + mapstep - 1 );
        if( !m[mapstep] )
            CANNY_PUSH( m + mapstep );
        if( !m[mapstep+1] )
            CANNY_PUSH( m + mapstep + 1 );
    }

    // the final pass, form the final image
    for( i = 0; i < size.height; i++ )
    {
        const uchar* _map = map + mapstep*(i+1) + 1;
        uchar* _dst = dst->data.ptr + dst->step*i;

        for( j = 0; j < size.width; j++ )
            _dst[j] = (uchar)-(_map[j] >> 1);
    }

    __END__;

    ReleaseMat( &dx );
    ReleaseMat( &dy );
    cvFree( &buffer );
    cvFree( &stack_bottom );
}

/* End of file. */
#include "cv.h"
#include "misc.h"

#define VOS_CALIPERS_MAXHEIGHT      0
#define VOS_CALIPERS_MINAREARECT    1


static void
iRotatingCalipers( Point2D32f* points, int n, int mode, float* out )
{
    float minarea = FLT_MAX;
    float max_dist = 0;
    char buffer[32];
    int i, k;
    Point2D32f* vect = (Point2D32f*)SysAlloc( n * sizeof(vect[0]) );
    float* inv_vect_length = (float*)SysAlloc( n * sizeof(inv_vect_length[0]) );
    int left = 0, bottom = 0, right = 0, top = 0;
    int seq[4] = { -1, -1, -1, -1 };

    float orientation = 0;
    float base_a;
    float base_b = 0;

    float left_x, right_x, top_y, bottom_y;
    Point2D32f pt0 = points[0];

    left_x = right_x = pt0.x;
    top_y = bottom_y = pt0.y;

    for( i = 0; i < n; i++ )
    {
        double dx, dy;

        if( pt0.x < left_x )
            left_x = pt0.x, left = i;

        if( pt0.x > right_x )
            right_x = pt0.x, right = i;

        if( pt0.y > top_y )
            top_y = pt0.y, top = i;

        if( pt0.y < bottom_y )
            bottom_y = pt0.y, bottom = i;

        Point2D32f pt = points[(i+1) & (i+1 < n ? -1 : 0)];

        dx = pt.x - pt0.x;
        dy = pt.y - pt0.y;

        vect[i].x = (float)dx;
        vect[i].y = (float)dy;
        inv_vect_length[i] = (float)(1./sqrt(dx*dx + dy*dy));

        pt0 = pt;
    }

    /* find convex hull orientation */
    {
        double ax = vect[n-1].x;
        double ay = vect[n-1].y;

        for( i = 0; i < n; i++ )
        {
            double bx = vect[i].x;
            double by = vect[i].y;

            double convexity = ax * by - ay * bx;

            if( convexity != 0 )
            {
                orientation = (convexity > 0) ? 1.0f : (-1.0f);
                break;
            }
            ax = bx;
            ay = by;
        }
        assert( 0 != orientation);
    }
    base_a = orientation;

    /*****************************************************************************************/
    /*                         init calipers position                                        */
    seq[0] = bottom;
    seq[1] = right;
    seq[2] = top;
    seq[3] = left;
    /*****************************************************************************************/
    /*                         Main loop - evaluate angles and rotate calipers               */

    /* all of edges will be checked while rotating calipers by 90 degrees */
    for( k = 0; k < n; k++ )
    {
        float dp0 = base_a * vect[seq[0]].x + base_b * vect[seq[0]].y;
        float dp1 = -base_b * vect[seq[1]].x + base_a * vect[seq[1]].y;
        float dp2 = -base_a * vect[seq[2]].x - base_b * vect[seq[2]].y;
        float dp3 = base_b * vect[seq[3]].x - base_a * vect[seq[3]].y;

        float cosalpha = dp0 * inv_vect_length[seq[0]];
        float maxcos = cosalpha;

        /* number of calipers edges, that has minimal angle with edge */
        int main_element = 0;

        /* choose minimal angle */
        cosalpha = dp1 * inv_vect_length[seq[1]];
        maxcos = (cosalpha > maxcos) ? (main_element = 1, cosalpha) : maxcos;
        cosalpha = dp2 * inv_vect_length[seq[2]];
        maxcos = (cosalpha > maxcos) ? (main_element = 2, cosalpha) : maxcos;
        cosalpha = dp3 * inv_vect_length[seq[3]];
        maxcos = (cosalpha > maxcos) ? (main_element = 3, cosalpha) : maxcos;

        /*rotate calipers*/
        {
            //get next base
            int pindex = seq[main_element];
            float lead_x = vect[pindex].x*inv_vect_length[pindex];
            float lead_y = vect[pindex].y*inv_vect_length[pindex];
            switch( main_element )
            {
            case 0:
                base_a = lead_x;
                base_b = lead_y;
                break;
            case 1:
                base_a = lead_y;
                base_b = -lead_x;
                break;
            case 2:
                base_a = -lead_x;
                base_b = -lead_y;
                break;
            case 3:
                base_a = -lead_y;
                base_b = lead_x;
                break;
            default: assert(0);
            }
        }
        /* change base point of main edge */
        seq[main_element] += 1;
        seq[main_element] = (seq[main_element] == n) ? 0 : seq[main_element];


        switch (mode)
        {
        case VOS_CALIPERS_MAXHEIGHT:
        {
            int opposite_el = main_element ^ 2;

            float dx = points[seq[opposite_el]].x - points[seq[main_element]].x;
            float dy = points[seq[opposite_el]].y - points[seq[main_element]].y;
            float dist;

            if( main_element & 1 )
                dist = (float)fabs(dx * base_a + dy * base_b);
            else
                dist = (float)fabs(dx * (-base_b) + dy * base_a);

            if( dist > max_dist )
                max_dist = dist;

            break;
        }
        case VOS_CALIPERS_MINAREARECT:
            /* find area of rectangle */
        {
            float height;
            float area;

            /* find vector left-right */
            float dx = points[seq[1]].x - points[seq[3]].x;
            float dy = points[seq[1]].y - points[seq[3]].y;

            /* dotproduct */
            float width = dx * base_a + dy * base_b;

            /* find vector left-right */
            dx = points[seq[2]].x - points[seq[0]].x;
            dy = points[seq[2]].y - points[seq[0]].y;

            /* dotproduct */
            height = -dx * base_b + dy * base_a;

            area = width * height;
            if( area <= minarea )
            {
                float *buf = (float *) buffer;

                minarea = area;
                /* leftist point */
                ((int *) buf)[0] = seq[3];
                buf[1] = base_a;
                buf[2] = width;
                buf[3] = base_b;
                buf[4] = height;
                /* bottom point */
                ((int *) buf)[5] = seq[0];
                buf[6] = area;
            }
            break;
        }
        }                       /*switch */
    }                           /* for */

    switch (mode)
    {
    case VOS_CALIPERS_MINAREARECT:
    {
        float *buf = (float *) buffer;

        float A1 = buf[1];
        float B1 = buf[3];

        float A2 = -buf[3];
        float B2 = buf[1];

        float C1 = A1 * points[((int *) buf)[0]].x + points[((int *) buf)[0]].y * B1;
        float C2 = A2 * points[((int *) buf)[5]].x + points[((int *) buf)[5]].y * B2;

        float idet = 1.f / (A1 * B2 - A2 * B1);

        float px = (C1 * B2 - C2 * B1) * idet;
        float py = (A1 * C2 - A2 * C1) * idet;

        out[0] = px;
        out[1] = py;

        out[2] = A1 * buf[2];
        out[3] = B1 * buf[2];

        out[4] = A2 * buf[4];
        out[5] = B2 * buf[4];
    }
        break;
    case VOS_CALIPERS_MAXHEIGHT:
    {
        out[0] = max_dist;
    }
        break;
    }

    SYS_FREE( &vect );
    SYS_FREE( &inv_vect_length );
}


Box2D
MinAreaRect2( const Seq* array, MemStorage* storage )
{
    MemStorage* temp_storage = NULL;
    Box2D box;
    Point2D32f* points = NULL;

    VOS_FUNCNAME( "MinAreaRect2" );
    Seq* ptseq = (Seq*)array;
    VOS_MEMSET(&box, 0, sizeof(box));

    __BEGIN__;

    int i, n;
    SeqReader reader;
    Point2D32f out[3];

    if( !storage )
        storage = ptseq->storage;

    if( storage )
    {
        VOS_CALL( temp_storage = CreateChildMemStorage( storage ));
    }
    else
    {
        VOS_CALL( temp_storage = CreateMemStorage(1 << 10));
    }

    if( !VOS_IS_SEQ_CONVEX( ptseq ))
    {
        VOS_CALL( ptseq = ConvexHull2( ptseq, temp_storage, VOS_CLOCKWISE, 1 ));
    }

    n = ptseq->total;

    VOS_CALL( points = (Point2D32f*)SysAlloc( n*sizeof(points[0]) ));
    StartReadSeq( ptseq, &reader );

    if( VOS_SEQ_ELTYPE( ptseq ) == VOS_32SC2 )
    {
        for( i = 0; i < n; i++ )
        {
            Point pt;
            VOS_READ_SEQ_ELEM( pt, reader );
            points[i].x = (float)pt.x;
            points[i].y = (float)pt.y;
        }
    }
    else
    {
        for( i = 0; i < n; i++ )
        {
            VOS_READ_SEQ_ELEM( points[i], reader );
        }
    }

    if( n > 2 )
    {
        iRotatingCalipers( points, n, VOS_CALIPERS_MINAREARECT, (float*)out );
        box.center.x = out[0].x + (out[1].x + out[2].x)*0.5f;
        box.center.y = out[0].y + (out[1].y + out[2].y)*0.5f;
        box.size.height = (float)sqrt((double)out[1].x*out[1].x + (double)out[1].y*out[1].y);
        box.size.width = (float)sqrt((double)out[2].x*out[2].x + (double)out[2].y*out[2].y);
        box.angle = (float)atan2( -(double)out[1].y, (double)out[1].x );
    }
    else if( 2 == n )
    {
        box.center.x = (points[0].x + points[1].x)*0.5f;
        box.center.y = (points[0].y + points[1].y)*0.5f;
        double dx = points[1].x - points[0].x;
        double dy = points[1].y - points[0].y;
        box.size.height = (float)sqrt(dx*dx + dy*dy);
        box.size.width = 0;
        box.angle = (float)atan2( -dy, dx );
    }
    else
    {
        if( 1 == n )
            box.center = points[0];
    }

    box.angle = (float)(box.angle*180/VOS_PI);

    __END__;

    ReleaseMemStorage( &temp_storage );
    SYS_FREE( &points );

    return box;
}


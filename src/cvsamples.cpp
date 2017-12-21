
#include <cv.h>
#include <highgui.h>

/* Calculates coefficients of perspective transformation
 * which maps <quad> into rectangle ((0,0), (w,0), (w,h), (h,0)):
 *
 *      c00*xi + c01*yi + c02
 * ui = ---------------------
 *      c20*xi + c21*yi + c22
 *
 *      c10*xi + c11*yi + c12
 * vi = ---------------------
 *      c20*xi + c21*yi + c22
 *
 * Coefficients are calculated by solving linear system:
 * / x0 y0  1  0  0  0 -x0*u0 -y0*u0 \ /c00\ /u0\
 * | x1 y1  1  0  0  0 -x1*u1 -y1*u1 | |c01| |u1|
 * | x2 y2  1  0  0  0 -x2*u2 -y2*u2 | |c02| |u2|
 * | x3 y3  1  0  0  0 -x3*u3 -y3*u3 |.|c10|=|u3|,
 * |  0  0  0 x0 y0  1 -x0*v0 -y0*v0 | |c11| |v0|
 * |  0  0  0 x1 y1  1 -x1*v1 -y1*v1 | |c12| |v1|
 * |  0  0  0 x2 y2  1 -x2*v2 -y2*v2 | |c20| |v2|
 * \  0  0  0 x3 y3  1 -x3*v3 -y3*v3 / \c21/ \v3/
 *
 * where:
 *   (xi, yi) = (quad[i][0], quad[i][1])
 *        cij - coeffs[i][j], coeffs[2][2] = 1
 *   (ui, vi) - rectangle vertices
 */
void cvGetPerspectiveTransform( CvSize src_size, double quad[4][2],
                                double coeffs[3][3] )
{
    CV_FUNCNAME( "cvWarpPerspective" );

    __BEGIN__;

    double a[8][8];
    double b[8];

    CvMat A = cvMat( 8, 8, CV_64FC1, a );
    CvMat B = cvMat( 8, 1, CV_64FC1, b );
    CvMat X = cvMat( 8, 1, CV_64FC1, coeffs );

    int i;
    for( i = 0; i < 4; ++i )
    {
        a[i][0] = quad[i][0]; a[i][1] = quad[i][1]; a[i][2] = 1;
        a[i][3] = a[i][4] = a[i][5] = a[i][6] = a[i][7] = 0;
        b[i] = 0;
    }
    for( i = 4; i < 8; ++i )
    {
        a[i][3] = quad[i-4][0]; a[i][4] = quad[i-4][1]; a[i][5] = 1;
        a[i][0] = a[i][1] = a[i][2] = a[i][6] = a[i][7] = 0;
        b[i] = 0;
    }

    int u = src_size.width - 1;
    int v = src_size.height - 1;

    a[1][6] = -quad[1][0] * u; a[1][7] = -quad[1][1] * u;
    a[2][6] = -quad[2][0] * u; a[2][7] = -quad[2][1] * u;
    b[1] = b[2] = u;

    a[6][6] = -quad[2][0] * v; a[6][7] = -quad[2][1] * v;
    a[7][6] = -quad[3][0] * v; a[7][7] = -quad[3][1] * v;
    b[6] = b[7] = v;

    cvSolve( &A, &B, &X );

    coeffs[2][2] = 1;

    __END__;
}


/* End of file. */

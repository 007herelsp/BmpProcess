

#ifndef _CV_GEOM_H_
#define _CV_GEOM_H_

/* Finds distance between two points */
CV_INLINE  float  icvDistanceL2_32f( CvPoint2D32f pt1, CvPoint2D32f pt2 )
{
    float dx = pt2.x - pt1.x;
    float dy = pt2.y - pt1.y;

    return std::sqrt( dx*dx + dy*dy );
}


int  icvIntersectLines( double x1, double dx1, double y1, double dy1,
                        double x2, double dx2, double y2, double dy2,
                        double* t2 );



#endif /*_IPCVGEOM_H_*/

/* End of file. */



#include "core.precomp.hpp"


namespace cv
{
    ParallelLoopBody::~ParallelLoopBody() {}
}



/* ================================   parallel_for_  ================================ */

void cv::parallel_for_(const cv::Range& range, const cv::ParallelLoopBody& body, double nstripes)
{

        (void)nstripes;
        body(range);
 
}







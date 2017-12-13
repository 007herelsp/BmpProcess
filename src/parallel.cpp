

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


const char* cv::currentParallelFramework() {
#ifdef CV_PARALLEL_FRAMEWORK
    return CV_PARALLEL_FRAMEWORK;
#else
    return NULL;
#endif
}





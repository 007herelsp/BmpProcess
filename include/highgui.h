#ifndef _HIGH_GUI_
#define _HIGH_GUI_

#ifndef SKIP_INCLUDES

  #include "cxcore.h"

#else // SKIP_INCLUDES


    #define CV_CDECL
    #define CV_STDCALL



  #ifndef CV_INLINE
    #if defined __cplusplus
      #define CV_INLINE inline
    #elif (defined WIN32 || defined WIN64) && !defined __GNUC__
      #define CV_INLINE __inline
    #else
      #define CV_INLINE static
    #endif
  #endif /* CV_INLINE */


    #define CV_EXPORTS


  #ifndef CVAPI
    #define CVAPI(rettype) CV_EXTERN_C CV_EXPORTS rettype CV_CDECL
  #endif

#endif // SKIP_INCLUDES



#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

/****************************************************************************************\
*                                  Basic GUI functions                                   *
\****************************************************************************************/

/* 8bit, color or not */
/* 8bit, gray */
/* ?, color */
#define CV_LOAD_IMAGE_COLOR       1
/* any depth, ? */ 
#define CV_LOAD_IMAGE_ANYDEPTH    2
/* ?, any color */
#define CV_LOAD_IMAGE_ANYCOLOR    4


CVAPI(IplImage*) cvLoadImage( const char* filename, int iscolor CV_DEFAULT(CV_LOAD_IMAGE_COLOR));

/* save image to file */
CVAPI(int) cvSaveImage( const char* filename, const CvArr* image );


#ifdef __cplusplus
  	}
#endif /* __cplusplus */



#endif /* _HIGH_GUI_ */

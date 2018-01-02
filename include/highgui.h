#ifndef _HIGH_GUI_
#define _HIGH_GUI_

#ifndef SKIP_INCLUDES

  #include "cxcore.h"

#else // SKIP_INCLUDES


    #define VOS_CDECL
    #define VOS_STDCALL



  #ifndef VOS_INLINE
    #if defined __cplusplus
      #define VOS_INLINE inline
    #elif (defined WIN32 || defined WIN64) && !defined __GNUC__
      #define VOS_INLINE __inline
    #else
      #define VOS_INLINE static
    #endif
  #endif /* VOS_INLINE */


    #define


#endif // SKIP_INCLUDES

#define VOS_LOAD_IMAGE_COLOR       1
/* any depth, ? */ 
/* ?, any color */

IplImage* cvLoadImage( const char* filename, int iscolor VOS_DEFAULT(VOS_LOAD_IMAGE_COLOR));

/* save image to file */
int cvSaveImage( const char* filename, const CvArr* image );


#endif /* _HIGH_GUI_ */

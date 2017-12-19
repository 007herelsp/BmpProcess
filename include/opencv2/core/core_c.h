#ifndef __OPENCV_CORE_C_H__
#define __OPENCV_CORE_C_H__

#include "opencv2/core/types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************\
*          Array allocation, deallocation, initialization and access to elements         *
\****************************************************************************************/

/* <malloc> wrapper.
   If there is no enough memory, the function
   (as well as other OpenCV functions that call cvAlloc)
   raises an error. */
CVAPI(void*)  cvAlloc( size_t size );

/* <free> wrapper.
   Here and further all the memory releasing functions
   (that all call cvFree) take double pointer in order to
   to clear pointer to the data after releasing it.
   Passing pointer to NULL pointer is Ok: nothing happens in this case
*/
CVAPI(void)   cvFree_( void* ptr );
#define cvFree(ptr) (cvFree_(*(ptr)), *(ptr)=0)

/* Allocates and initializes IplImage header */
CVAPI(IplImage*)  cvCreateImageHeader( CvSize size, int depth, int channels );

/* Inializes IplImage header */
CVAPI(IplImage*) cvInitImageHeader( IplImage* image, CvSize size, int depth,
                                   int channels, int origin CV_DEFAULT(0),
                                   int align CV_DEFAULT(4));

/* Creates IPL image (header and data) */
CVAPI(IplImage*)  cvCreateImage( CvSize size, int depth, int channels );

/* Releases (i.e. deallocates) IPL image header */
CVAPI(void)  cvReleaseImageHeader( IplImage** image );

/* Releases IPL image header and data */
CVAPI(void)  cvReleaseImage( IplImage** image );

/* Creates a copy of IPL image (widthStep may differ) */
CVAPI(IplImage*) cvCloneImage( const IplImage* image );

/* Sets a Channel Of Interest (only a few functions support COI) -
   use cvCopy to extract the selected channel and/or put it back */
CVAPI(void)  cvSetImageCOI( IplImage* image, int coi );

/* Retrieves image Channel Of Interest */
CVAPI(int)  cvGetImageCOI( const IplImage* image );

/* Sets image ROI (region of interest) (COI is not changed) */
CVAPI(void)  cvSetImageROI( IplImage* image, CvRect rect );

/* Resets image ROI and COI */
CVAPI(void)  cvResetImageROI( IplImage* image );

/* Retrieves image ROI */
CVAPI(CvRect) cvGetImageROI( const IplImage* image );

/* Allocates and initializes CvMat header */
CVAPI(CvMat*)  cvCreateMatHeader( int rows, int cols, int type );

#define CV_AUTOSTEP  0x7fffffff

/* Initializes CvMat header */
CVAPI(CvMat*) cvInitMatHeader( CvMat* mat, int rows, int cols,
                              int type, void* data CV_DEFAULT(NULL),
                              int step CV_DEFAULT(CV_AUTOSTEP) );

/* Allocates and initializes CvMat header and allocates data */
CVAPI(CvMat*)  cvCreateMat( int rows, int cols, int type );

/* Releases CvMat header and deallocates matrix data
   (reference counting is used for data) */
CVAPI(void)  cvReleaseMat( CvMat** mat );

/* Decrements CvMat data reference counter and deallocates the data if
   it reaches 0 */
CV_INLINE  void  cvDecRefData( CvArr* arr )
{
    if( CV_IS_MAT( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        mat->data.ptr = NULL;
        if( mat->refcount != NULL && --*mat->refcount == 0 )
            cvFree( &mat->refcount );
        mat->refcount = NULL;
    }
	else
		{
		assert("herelsp remove");
		}
}




/* Creates an exact copy of the input matrix (except, may be, step value) */
CVAPI(CvMat*) cvCloneMat( const CvMat* mat );


/**************** matrix iterator: used for n-ary operations on dense arrays *********/

#define CV_MAX_ARR 10

#define CV_NO_DEPTH_CHECK     1
#define CV_NO_CN_CHECK        2
#define CV_NO_SIZE_CHECK      4




/* Converts CvArr (IplImage or CvMat,...) to CvMat.
   If the last parameter is non-zero, function can
   convert multi(>2)-dimensional array to CvMat as long as
   the last array's dimension is continous. The resultant
   matrix will be have appropriate (a huge) number of rows */
CVAPI(CvMat*) cvGetMat( const CvArr* arr, CvMat* header,
                       int* coi CV_DEFAULT(NULL),
                       int allowND CV_DEFAULT(0));

/* Converts CvArr (IplImage or CvMat) to IplImage */
CVAPI(IplImage*) cvGetImage( const CvArr* arr, IplImage* image_header );







/* Allocates array data */
CVAPI(void)  cvCreateData( CvArr* arr );

/* Releases array data */
CVAPI(void)  cvReleaseData( CvArr* arr );

/* Attaches user data to the array header. The step is reffered to
   the pre-last dimension. That is, all the planes of the array
   must be joint (w/o gaps) */
CVAPI(void)  cvSetData( CvArr* arr, void* data, int step );


/* Returns width and height of array in elements */
CVAPI(CvSize) cvGetSize( const CvArr* arr );

/* Copies source array to destination array */
CVAPI(void)  cvCopy( const CvArr* src, CvArr* dst,
                     const CvArr* mask CV_DEFAULT(NULL) );




/* Splits a multi-channel array into the set of single-channel arrays or
   extracts particular [color] plane */
CVAPI(void)  cvSplit( const CvArr* src, CvArr* dst0, CvArr* dst1,
                      CvArr* dst2, CvArr* dst3 );







#define CV_CMP_EQ   0
#define CV_CMP_GT   1
#define CV_CMP_GE   2
#define CV_CMP_LT   3
#define CV_CMP_LE   4
#define CV_CMP_NE   5


/****************************************************************************************\
*                                Math operations                                         *
\****************************************************************************************/



/* Does powering: dst(idx) = src(idx)^power */
CVAPI(void)  cvPow( const CvArr* src, CvArr* dst, double power );






/* Checks array values for NaNs, Infs or simply for too large numbers
   (if CV_CHECK_RANGE is set). If CV_CHECK_QUIET is set,
   no runtime errors is raised (function returns zero value in case of "bad" values).
   Otherwise cvError is called */
#define  CV_CHECK_RANGE    1
#define  CV_CHECK_QUIET    2

#define CV_RAND_UNI      0
#define CV_RAND_NORMAL   1


#define CV_SORT_EVERY_ROW 0
#define CV_SORT_EVERY_COLUMN 1
#define CV_SORT_ASCENDING 0
#define CV_SORT_DESCENDING 16



/****************************************************************************************\
*                                Matrix operations                                       *
\****************************************************************************************/

#define CV_GEMM_A_T 1
#define CV_GEMM_B_T 2
#define CV_GEMM_C_T 4


/* Transforms each element of source array and stores
   resultant vectors in destination array */
CVAPI(void)  cvTransform( const CvArr* src, CvArr* dst,
                          const CvMat* transmat,
                          const CvMat* shiftvec CV_DEFAULT(NULL));
#define cvMatMulAddS cvTransform

/* Does perspective transform on every element of input array */
CVAPI(void)  cvPerspectiveTransform( const CvArr* src, CvArr* dst,
                                     const CvMat* mat );



/* types of array norm */
#define CV_C            1
#define CV_L1           2
#define CV_L2           4
#define CV_NORM_MASK    7
#define CV_RELATIVE     8
#define CV_DIFF         16
#define CV_MINMAX       32

#define CV_DIFF_C       (CV_DIFF | CV_C)
#define CV_DIFF_L1      (CV_DIFF | CV_L1)
#define CV_DIFF_L2      (CV_DIFF | CV_L2)
#define CV_RELATIVE_C   (CV_RELATIVE | CV_C)
#define CV_RELATIVE_L1  (CV_RELATIVE | CV_L1)
#define CV_RELATIVE_L2  (CV_RELATIVE | CV_L2)





#define CV_REDUCE_SUM 0
#define CV_REDUCE_AVG 1
#define CV_REDUCE_MAX 2
#define CV_REDUCE_MIN 3


/****************************************************************************************\
*                              Dynamic data structures                                   *
\****************************************************************************************/

/* Calculates length of sequence slice (with support of negative indices). */
CVAPI(int) cvSliceLength( CvSlice slice, const CvSeq* seq );


/* Creates new memory storage.
   block_size == 0 means that default,
   somewhat optimal size, is used (currently, it is 64K) */
CVAPI(CvMemStorage*)  cvCreateMemStorage( int block_size CV_DEFAULT(0));


/* Creates a memory storage that will borrow memory blocks from parent storage */
CVAPI(CvMemStorage*)  cvCreateChildMemStorage( CvMemStorage* parent );


/* Releases memory storage. All the children of a parent must be released before
   the parent. A child storage returns all the blocks to parent when it is released */
CVAPI(void)  cvReleaseMemStorage( CvMemStorage** storage );


/* Clears memory storage. This is the only way(!!!) (besides cvRestoreMemStoragePos)
   to reuse memory allocated for the storage - cvClearSeq,cvClearSet ...
   do not free any memory.
   A child storage returns all the blocks to the parent when it is cleared */
CVAPI(void)  cvClearMemStorage( CvMemStorage* storage );

/* Remember a storage "free memory" position */
CVAPI(void)  cvSaveMemStoragePos( const CvMemStorage* storage, CvMemStoragePos* pos );

/* Restore a storage "free memory" position */
CVAPI(void)  cvRestoreMemStoragePos( CvMemStorage* storage, CvMemStoragePos* pos );

/* Allocates continuous buffer of the specified size in the storage */
CVAPI(void*) cvMemStorageAlloc( CvMemStorage* storage, size_t size );


/* Creates new empty sequence that will reside in the specified storage */
CVAPI(CvSeq*)  cvCreateSeq( int seq_flags, size_t header_size,
                            size_t elem_size, CvMemStorage* storage );

/* Changes default size (granularity) of sequence blocks.
   The default size is ~1Kbyte */
CVAPI(void)  cvSetSeqBlockSize( CvSeq* seq, int delta_elems );


/* Adds new element to the end of sequence. Returns pointer to the element */
CVAPI(schar*)  cvSeqPush( CvSeq* seq, const void* element CV_DEFAULT(NULL));


/* Adds new element to the beginning of sequence. Returns pointer to it */
CVAPI(schar*)  cvSeqPushFront( CvSeq* seq, const void* element CV_DEFAULT(NULL));


/* Removes the last element from sequence and optionally saves it */
CVAPI(void)  cvSeqPop( CvSeq* seq, void* element CV_DEFAULT(NULL));


/* Removes the first element from sequence and optioanally saves it */
CVAPI(void)  cvSeqPopFront( CvSeq* seq, void* element CV_DEFAULT(NULL));


#define CV_FRONT 1
#define CV_BACK 0
/* Adds several new elements to the end of sequence */
CVAPI(void)  cvSeqPushMulti( CvSeq* seq, const void* elements,
                             int count, int in_front CV_DEFAULT(0) );

/* Removes several elements from the end of sequence and optionally saves them */
CVAPI(void)  cvSeqPopMulti( CvSeq* seq, void* elements,
                            int count, int in_front CV_DEFAULT(0) );

/* Inserts a new element in the middle of sequence.
   cvSeqInsert(seq,0,elem) == cvSeqPushFront(seq,elem) */
CVAPI(schar*)  cvSeqInsert( CvSeq* seq, int before_index,
                            const void* element CV_DEFAULT(NULL));

/* Removes specified sequence element */
CVAPI(void)  cvSeqRemove( CvSeq* seq, int index );


/* Removes all the elements from the sequence. The freed memory
   can be reused later only by the same sequence unless cvClearMemStorage
   or cvRestoreMemStoragePos is called */
CVAPI(void)  cvClearSeq( CvSeq* seq );


/* Retrieves pointer to specified sequence element.
   Negative indices are supported and mean counting from the end
   (e.g -1 means the last sequence element) */
CVAPI(schar*)  cvGetSeqElem( const CvSeq* seq, int index );

/* Calculates index of the specified sequence element.
   Returns -1 if element does not belong to the sequence */
CVAPI(int)  cvSeqElemIdx( const CvSeq* seq, const void* element,
                         CvSeqBlock** block CV_DEFAULT(NULL) );

/* Initializes sequence writer. The new elements will be added to the end of sequence */
CVAPI(void)  cvStartAppendToSeq( CvSeq* seq, CvSeqWriter* writer );


/* Combination of cvCreateSeq and cvStartAppendToSeq */
CVAPI(void)  cvStartWriteSeq( int seq_flags, int header_size,
                              int elem_size, CvMemStorage* storage,
                              CvSeqWriter* writer );

/* Closes sequence writer, updates sequence header and returns pointer
   to the resultant sequence
   (which may be useful if the sequence was created using cvStartWriteSeq))
*/
CVAPI(CvSeq*)  cvEndWriteSeq( CvSeqWriter* writer );


/* Updates sequence header. May be useful to get access to some of previously
   written elements via cvGetSeqElem or sequence reader */
CVAPI(void)   cvFlushSeqWriter( CvSeqWriter* writer );


/* Initializes sequence reader.
   The sequence can be read in forward or backward direction */
CVAPI(void) cvStartReadSeq( const CvSeq* seq, CvSeqReader* reader,
                           int reverse CV_DEFAULT(0) );


/* Returns current sequence reader position (currently observed sequence element) */
CVAPI(int)  cvGetSeqReaderPos( CvSeqReader* reader );


/* Changes sequence reader position. It may seek to an absolute or
   to relative to the current position */
CVAPI(void)   cvSetSeqReaderPos( CvSeqReader* reader, int index,
                                 int is_relative CV_DEFAULT(0));

/* Copies sequence content to a continuous piece of memory */
CVAPI(void*)  cvCvtSeqToArray( const CvSeq* seq, void* elements,
                               CvSlice slice CV_DEFAULT(CV_WHOLE_SEQ) );

/* Creates sequence header for array.
   After that all the operations on sequences that do not alter the content
   can be applied to the resultant sequence */
CVAPI(CvSeq*) cvMakeSeqHeaderForArray( int seq_type, int header_size,
                                       int elem_size, void* elements, int total,
                                       CvSeq* seq, CvSeqBlock* block );

/* Extracts sequence slice (with or without copying sequence elements) */
CVAPI(CvSeq*) cvSeqSlice( const CvSeq* seq, CvSlice slice,
                         CvMemStorage* storage CV_DEFAULT(NULL),
                         int copy_data CV_DEFAULT(0));

CV_INLINE CvSeq* cvCloneSeq( const CvSeq* seq, CvMemStorage* storage CV_DEFAULT(NULL))
{
    return cvSeqSlice( seq, CV_WHOLE_SEQ, storage, 1 );
}

/* Removes sequence slice */
CVAPI(void)  cvSeqRemoveSlice( CvSeq* seq, CvSlice slice );

/* Inserts a sequence or array into another sequence */
CVAPI(void)  cvSeqInsertSlice( CvSeq* seq, int before_index, const CvArr* from_arr );

/* a < b ? -1 : a > b ? 1 : 0 */
typedef int (CV_CDECL* CvCmpFunc)(const void* a, const void* b, void* userdata );


/* Reverses order of sequence elements in-place */
CVAPI(void) cvSeqInvert( CvSeq* seq );


/************ Internal sequence functions ************/
CVAPI(void)  cvChangeSeqBlock( void* reader, int direction );
CVAPI(void)  cvCreateSeqBlock( CvSeqWriter* writer );


/* Creates a new set */
CVAPI(CvSet*)  cvCreateSet( int set_flags, int header_size,
                            int elem_size, CvMemStorage* storage );

/* Adds new element to the set and returns pointer to it */
CVAPI(int)  cvSetAdd( CvSet* set_header, CvSetElem* elem CV_DEFAULT(NULL),
                      CvSetElem** inserted_elem CV_DEFAULT(NULL) );

/* Fast variant of cvSetAdd */
CV_INLINE  CvSetElem* cvSetNew( CvSet* set_header )
{
    CvSetElem* elem = set_header->free_elems;
    if( elem )
    {
        set_header->free_elems = elem->next_free;
        elem->flags = elem->flags & CV_SET_ELEM_IDX_MASK;
        set_header->active_count++;
    }
    else
        cvSetAdd( set_header, NULL, &elem );
    return elem;
}

/* Removes set element given its pointer */
CV_INLINE  void cvSetRemoveByPtr( CvSet* set_header, void* elem )
{
    CvSetElem* _elem = (CvSetElem*)elem;
    assert( _elem->flags >= 0 /*&& (elem->flags & CV_SET_ELEM_IDX_MASK) < set_header->total*/ );
    _elem->next_free = set_header->free_elems;
    _elem->flags = (_elem->flags & CV_SET_ELEM_IDX_MASK) | CV_SET_ELEM_FREE_FLAG;
    set_header->free_elems = _elem;
    set_header->active_count--;
}

/* Removes element from the set by its index  */
CVAPI(void)   cvSetRemove( CvSet* set_header, int index );

/* Returns a set element by index. If the element doesn't belong to the set,
   NULL is returned */
CV_INLINE CvSetElem* cvGetSetElem( const CvSet* set_header, int idx )
{
    CvSetElem* elem = (CvSetElem*)(void *)cvGetSeqElem( (CvSeq*)set_header, idx );
    return elem && CV_IS_SET_ELEM( elem ) ? elem : 0;
}

/* Removes all the elements from the set */
CVAPI(void)  cvClearSet( CvSet* set_header );



#define  CV_GRAPH_VERTEX        1
#define  CV_GRAPH_TREE_EDGE     2
#define  CV_GRAPH_BACK_EDGE     4
#define  CV_GRAPH_FORWARD_EDGE  8
#define  CV_GRAPH_CROSS_EDGE    16
#define  CV_GRAPH_ANY_EDGE      30
#define  CV_GRAPH_NEW_TREE      32
#define  CV_GRAPH_BACKTRACKING  64
#define  CV_GRAPH_OVER          -1

#define  CV_GRAPH_ALL_ITEMS    -1

/* flags for graph vertices and edges */
#define  CV_GRAPH_ITEM_VISITED_FLAG  (1 << 30)
#define  CV_IS_GRAPH_VERTEX_VISITED(vtx) \
    (((CvGraphVtx*)(vtx))->flags & CV_GRAPH_ITEM_VISITED_FLAG)
#define  CV_IS_GRAPH_EDGE_VISITED(edge) \
    (((CvGraphEdge*)(edge))->flags & CV_GRAPH_ITEM_VISITED_FLAG)
#define  CV_GRAPH_SEARCH_TREE_NODE_FLAG   (1 << 29)
#define  CV_GRAPH_FORWARD_EDGE_FLAG       (1 << 28)



/****************************************************************************************\
*                                     Drawing                                            *
\****************************************************************************************/

/****************************************************************************************\
*       Drawing functions work with images/matrices of arbitrary type.                   *
*       For color images the channel order is BGR[A]                                     *
*       Antialiasing is supported only for 8-bit image now.                              *
*       All the functions include parameter color that means rgb value (that may be      *
*       constructed with CV_RGB macro) for color images and brightness                   *
*       for grayscale images.                                                            *
*       If a drawn figure is partially or completely outside of the image, it is clipped.*
\****************************************************************************************/

#define CV_RGB( r, g, b )  cvScalar( (b), (g), (r), 0 )
#define CV_FILLED -1

#define CV_AA 16



/* Moves iterator to the next line point */
#define CV_NEXT_LINE_POINT( line_iterator )                     \
{                                                               \
    int _line_iterator_mask = (line_iterator).err < 0 ? -1 : 0; \
    (line_iterator).err += (line_iterator).minus_delta +        \
        ((line_iterator).plus_delta & _line_iterator_mask);     \
    (line_iterator).ptr += (line_iterator).minus_step +         \
        ((line_iterator).plus_step & _line_iterator_mask);      \
}






/******************* Iteration through the sequence tree *****************/


/* Inserts sequence into tree with specified "parent" sequence.
   If parent is equal to frame (e.g. the most external contour),
   then added contour will have null pointer to parent. */
CVAPI(void) cvInsertNodeIntoTree( void* node, void* parent, void* frame );



/****************************************************************************************\
*                                    System functions                                    *
\****************************************************************************************/



/* Loads optimized functions from IPP, MKL etc. or switches back to pure C code */
CVAPI(int)  cvUseOptimized( int on_off );



typedef void* (CV_CDECL *CvAllocFunc)(size_t size, void* userdata);
typedef int (CV_CDECL *CvFreeFunc)(void* pptr, void* userdata);




typedef IplImage* (CV_STDCALL* Cv_iplCreateImageHeader)
                            (int,int,int,char*,char*,int,int,int,int,int,
                            IplROI*,IplImage*,void*,IplTileInfo*);
typedef void (CV_STDCALL* Cv_iplAllocateImageData)(IplImage*,int,int);
typedef void (CV_STDCALL* Cv_iplDeallocate)(IplImage*,int);
typedef IplROI* (CV_STDCALL* Cv_iplCreateROI)(int,int,int,int,int);
typedef IplImage* (CV_STDCALL* Cv_iplCloneImage)(const IplImage*);



/*********************************** Adding own types ***********************************/

CVAPI(void) cvUnregisterType( const char* type_name );

/* universal functions */
CVAPI(void) cvRelease( void** struct_ptr );
CVAPI(void*) cvClone( const void* struct_ptr );



/*********************************** Measuring Execution Time ***************************/

/* helper functions for RNG initialization and accurate time measurement:
   uses internal clock counter on x86 */
CVAPI(double) cvGetTickFrequency( void );

/*********************************** CPU capabilities ***********************************/

#define CV_CPU_NONE    0
#define CV_CPU_MMX     1
#define CV_CPU_SSE     2
#define CV_CPU_SSE2    3
#define CV_CPU_SSE3    4
#define CV_CPU_SSSE3   5
#define CV_CPU_SSE4_1  6
#define CV_CPU_SSE4_2  7
#define CV_CPU_POPCNT  8
#define CV_CPU_AVX    10
#define CV_CPU_AVX2   11
#define CV_HARDWARE_MAX_FEATURE 255


/*********************************** Multi-Threading ************************************/

/* retrieve/set the number of threads used in OpenMP implementations */
CVAPI(int)  cvGetNumThreads( void );
CVAPI(void) cvSetNumThreads( int threads CV_DEFAULT(0) );
/* get index of the thread being executed */
CVAPI(int)  cvGetThreadNum( void );


/********************************** Error Handling **************************************/

/* Get current OpenCV error status */
CVAPI(int) cvGetErrStatus( void );

/* Sets error status silently */
CVAPI(void) cvSetErrStatus( int status );

#define CV_ErrModeLeaf     0   /* Print error and exit program */
#define CV_ErrModeParent   1   /* Print error and continue */
#define CV_ErrModeSilent   2   /* Don't print and continue */

/* Retrives current error processing mode */
CVAPI(int)  cvGetErrMode( void );

/* Sets error processing mode, returns previously used mode */
CVAPI(int) cvSetErrMode( int mode );

/* Sets error status and performs some additonal actions (displaying message box,
 writing message to stderr, terminating application etc.)
 depending on the current error mode */
CVAPI(void) cvError( int status, const char* func_name,
                    const char* err_msg, const char* file_name, int line );

/* Retrieves textual description of the error given its code */
CVAPI(const char*) cvErrorStr( int status );



typedef int (CV_CDECL *CvErrorCallback)( int status, const char* func_name,
                                        const char* err_msg, const char* file_name, int line, void* userdata );





#define OPENCV_ERROR(status,func,context)                           \
cvError((status),(func),(context),__FILE__,__LINE__)

#define OPENCV_ERRCHK(func,context)                                 \
{if (cvGetErrStatus() >= 0)                         \
{OPENCV_ERROR(CV_StsBackTrace,(func),(context));}}

#define OPENCV_ASSERT(expr,func,context)                            \
{if (! (expr))                                      \
{OPENCV_ERROR(CV_StsInternal,(func),(context));}}

#define OPENCV_RSTERR() (cvSetErrStatus(CV_StsOk))

#define OPENCV_CALL( Func )                                         \
{                                                                   \
Func;                                                           \
}


/* CV_FUNCNAME macro defines icvFuncName constant which is used by CV_ERROR macro */
#ifdef CV_NO_FUNC_NAMES
#define CV_FUNCNAME( Name )
#define cvFuncName ""
#else
#define CV_FUNCNAME( Name )  \
static char cvFuncName[] = Name
#endif


/*
 CV_ERROR macro unconditionally raises error with passed code and message.
 After raising error, control will be transferred to the exit label.
 */
#define CV_ERROR( Code, Msg )                                       \
{                                                                   \
    cvError( (Code), cvFuncName, Msg, __FILE__, __LINE__ );        \
    __CV_EXIT__;                                                   \
}

/* Simplified form of CV_ERROR */
#define CV_ERROR_FROM_CODE( code )   \
    CV_ERROR( code, "" )

/*
 CV_CHECK macro checks error status after CV (or IPL)
 function call. If error detected, control will be transferred to the exit
 label.
 */
#define CV_CHECK()                                                  \
{                                                                   \
    if( cvGetErrStatus() < 0 )                                      \
        CV_ERROR( CV_StsBackTrace, "Inner function failed." );      \
}


/*
 CV_CALL macro calls CV (or IPL) function, checks error status and
 signals a error if the function failed. Useful in "parent node"
 error procesing mode
 */
#define CV_CALL( Func )                                             \
{                                                                   \
    Func;                                                           \
    CV_CHECK();                                                     \
}


/* Runtime assertion macro */
#define CV_ASSERT( Condition )                                          \
{                                                                       \
    if( !(Condition) )                                                  \
        CV_ERROR( CV_StsInternal, "Assertion: " #Condition " failed" ); \
}

#define __CV_BEGIN__       {
#define __CV_END__         goto exit; exit: ; }
#define __CV_EXIT__        goto exit

#ifdef __cplusplus
}





#endif

#endif

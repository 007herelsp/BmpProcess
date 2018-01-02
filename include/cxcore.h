#ifndef _CXCORE_H_
#define _CXCORE_H_

#include "cxtypes.h"
#include "cxerror.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************\
*          Array allocation, deallocation, initialization and access to elements         *
\****************************************************************************************/

/* <malloc> wrapper.
   If there is no enough memory, the function
   (as well as other   functions that call cvAlloc)
   raises an error. */
CVAPI(void *)
cvAlloc(size_t size);

/* <free> wrapper.
   Here and further all the memory releasing functions
   (that all call cvFree) take double pointer in order to
   to clear pointer to the data after releasing it.
   Passing pointer to NULL pointer is Ok: nothing happens in this case
*/
CVAPI(void)
cvFree_(void *ptr);
#define cvFree(ptr) (cvFree_(*(ptr)), *(ptr) = 0)

/* Allocates and initializes IplImage header */
CVAPI(IplImage *)
cvCreateImageHeader(CvSize size, int depth, int channels);

/* Inializes IplImage header */
CVAPI(IplImage *)
cvInitImageHeader(IplImage *image, CvSize size, int depth,
                  int channels, int origin VOS_DEFAULT(0),
                  int align VOS_DEFAULT(4));

/* Creates IPL image (header and data) */
CVAPI(IplImage *)
cvCreateImage(CvSize size, int depth, int channels);

CVAPI(void)
cvReleaseImageHeader(IplImage **image);

/* Releases IPL image header and data */
CVAPI(void)
cvReleaseImage(IplImage **image);

/* Creates a copy of IPL image (widthStep may differ) */
CVAPI(IplImage *)
cvCloneImage(const IplImage *image);

/* Allocates and initalizes CvMat header */
CVAPI(CvMat *)
cvCreateMatHeader(int rows, int cols, int type);

#define VOS_AUTOSTEP 0x7fffffff

/* Initializes CvMat header */
CVAPI(CvMat *)
cvInitMatHeader(CvMat *mat, int rows, int cols,
                int type, void *data VOS_DEFAULT(NULL),
                int step VOS_DEFAULT(VOS_AUTOSTEP));

/* Allocates and initializes CvMat header and allocates data */
CVAPI(CvMat *)
cvCreateMat(int rows, int cols, int type);

/* Releases CvMat header and deallocates matrix data
   (reference counting is used for data) */
CVAPI(void)
cvReleaseMat(CvMat **mat);

/* Decrements CvMat data reference counter and deallocates the data if
   it reaches 0 */
VOS_INLINE void cvDecRefData(CvArr *arr)
{
    if (VOS_IS_MAT(arr))
    {
        CvMat *mat = (CvMat *)arr;
        mat->data.ptr = NULL;
        if (mat->refcount != NULL && --*mat->refcount == 0)
            cvFree(&mat->refcount);
        mat->refcount = NULL;
    }
    else
    {
        assert("herelsp remove" && 0);
    }
}

/* low-level scalar <-> raw data conversion functions */
CVAPI(void)
cvScalarToRawData(const CvScalar *scalar, void *data, int type);

/**************** matrix iterator: used for n-ary operations on dense arrays *********/

CVAPI(int)
cvGetElemType(const CvArr *arr);

CVAPI(CvMat *)
cvGetMat(const CvArr *arr, CvMat *header,
         int *coi VOS_DEFAULT(NULL),
         int allowND VOS_DEFAULT(0));

CVAPI(CvMat *)
cvReshape(const CvArr *arr, CvMat *header,
          int new_cn, int new_rows VOS_DEFAULT(0));

/* Allocates array data */
CVAPI(void)
cvCreateData(CvArr *arr);

/* Releases array data */
CVAPI(void)
cvReleaseData(CvArr *arr);

/* Attaches user data to the array header. The step is reffered to
   the pre-last dimension. That is, all the planes of the array
   must be joint (w/o gaps) */
CVAPI(void)
cvSetData(CvArr *arr, void *data, int step);

/* Returns width and height of array in elements */
CVAPI(CvSize)
cvGetSize(const CvArr *arr);

/* Copies source array to destination array */
CVAPI(void)
cvCopy(const CvArr *src, CvArr *dst,
       const CvArr *mask VOS_DEFAULT(NULL));

/* Performs linear transformation on every source array element:
   dst(x,y,c) = scale*src(x,y,c)+shift.
   Arbitrary combination of input and output array depths are allowed
   (number of channels must be the same), thus the function can be used
   for type conversion */
CVAPI(void)
cvConvertScale(const CvArr *src, CvArr *dst,
               double scale VOS_DEFAULT(1),
               double shift VOS_DEFAULT(0));
#define cvConvert(src, dst) cvConvertScale((src), (dst), 1, 0)

/****************************************************************************************\
*                                Math operations                                         *
\****************************************************************************************/

/* Does powering: dst(idx) = src(idx)^power */
CVAPI(void)
cvPow(const CvArr *src, CvArr *dst, double power);

/****************************************************************************************\
*                                Matrix operations                                       *
\****************************************************************************************/

#define VOS_SVD_MODIFY_A 1
#define VOS_SVD_U_T 2
#define VOS_SVD_V_T 4

/* Performs Singular Value Decomposition of a matrix */
CVAPI(void)
cvSVD(CvArr *A, CvArr *W, CvArr *U VOS_DEFAULT(NULL),
      CvArr *V VOS_DEFAULT(NULL), int flags VOS_DEFAULT(0));

/* Performs Singular Value Back Substitution (solves A*X = B):
   flags must be the same as in cvSVD */
CVAPI(void)
cvSVBkSb(const CvArr *W, const CvArr *U,
         const CvArr *V, const CvArr *B,
         CvArr *X, int flags);

#define VOS_LU 0
#define VOS_SVD 1
#define VOS_SVD_SYM 2


/****************************************************************************************\
*                              Dynamic data structures                                   *
\****************************************************************************************/

/* Calculates length of sequence slice (with support of negative indices). */
CVAPI(int)
cvSliceLength(CvSlice slice, const CvSeq *seq);

/* Creates new memory storage.
   block_size == 0 means that default,
   somewhat optimal size, is used (currently, it is 64K) */
CVAPI(CvMemStorage *)
cvCreateMemStorage(int block_size VOS_DEFAULT(0));

/* Creates a memory storage that will borrow memory blocks from parent storage */
CVAPI(CvMemStorage *)
cvCreateChildMemStorage(CvMemStorage *parent);

/* Releases memory storage. All the children of a parent must be released before
   the parent. A child storage returns all the blocks to parent when it is released */
CVAPI(void)
cvReleaseMemStorage(CvMemStorage **storage);

/* Clears memory storage. This is the only way(!!!) (besides cvRestoreMemStoragePos)
   to reuse memory allocated for the storage - cvClearSeq,cvClearSet ...
   do not free any memory.
   A child storage returns all the blocks to the parent when it is cleared */
CVAPI(void)
cvClearMemStorage(CvMemStorage *storage);

/* Remember a storage "free memory" position */
CVAPI(void)
cvSaveMemStoragePos(const CvMemStorage *storage, CvMemStoragePos *pos);

/* Restore a storage "free memory" position */
CVAPI(void)
cvRestoreMemStoragePos(CvMemStorage *storage, CvMemStoragePos *pos);

/* Allocates continuous buffer of the specified size in the storage */
CVAPI(void *)
cvMemStorageAlloc(CvMemStorage *storage, size_t size);

/* Creates new empty sequence that will reside in the specified storage */
CVAPI(CvSeq *)
cvCreateSeq(int seq_flags, int header_size,
            int elem_size, CvMemStorage *storage);

/* Changes default size (granularity) of sequence blocks.
   The default size is ~1Kbyte */
CVAPI(void)
cvSetSeqBlockSize(CvSeq *seq, int delta_elems);

/* Adds new element to the end of sequence. Returns pointer to the element */
CVAPI(char *)
cvSeqPush(CvSeq *seq, void *element VOS_DEFAULT(NULL));

/* Removes the last element from sequence and optionally saves it */
CVAPI(void)
cvSeqPop(CvSeq *seq, void *element VOS_DEFAULT(NULL));

#define VOS_FRONT 1
#define VOS_BACK 0

/* Removes several elements from the end of sequence and optionally saves them */
CVAPI(void)
cvSeqPopMulti(CvSeq *seq, void *elements,
              int count, int in_front VOS_DEFAULT(0));

/* Removes all the elements from the sequence. The freed memory
   can be reused later only by the same sequence unless cvClearMemStorage
   or cvRestoreMemStoragePos is called */
CVAPI(void)
cvClearSeq(CvSeq *seq);

/* Retrives pointer to specified sequence element.
   Negative indices are supported and mean counting from the end
   (e.g -1 means the last sequence element) */
CVAPI(char *)
cvGetSeqElem(const CvSeq *seq, int index);

/* Initializes sequence writer. The new elements will be added to the end of sequence */
CVAPI(void)
cvStartAppendToSeq(CvSeq *seq, CvSeqWriter *writer);

/* Combination of cvCreateSeq and cvStartAppendToSeq */
CVAPI(void)
cvStartWriteSeq(int seq_flags, int header_size,
                int elem_size, CvMemStorage *storage,
                CvSeqWriter *writer);

/* Closes sequence writer, updates sequence header and returns pointer
   to the resultant sequence
   (which may be useful if the sequence was created using cvStartWriteSeq))
*/
CVAPI(CvSeq *)
cvEndWriteSeq(CvSeqWriter *writer);

/* Updates sequence header. May be useful to get access to some of previously
   written elements via cvGetSeqElem or sequence reader */
CVAPI(void)
cvFlushSeqWriter(CvSeqWriter *writer);

/* Initializes sequence reader.
   The sequence can be read in forward or backward direction */
CVAPI(void)
cvStartReadSeq(const CvSeq *seq, CvSeqReader *reader,
               int reverse VOS_DEFAULT(0));

/* Returns current sequence reader position (currently observed sequence element) */
CVAPI(int)
cvGetSeqReaderPos(CvSeqReader *reader);

/* Changes sequence reader position. It may seek to an absolute or
   to relative to the current position */
CVAPI(void)
SetSeqReaderPos(CvSeqReader *reader, int index,
                  int is_relative VOS_DEFAULT(0));

/************ Internal sequence functions ************/
CVAPI(void)
cvChangeSeqBlock(void *reader, int direction);
CVAPI(void)
cvCreateSeqBlock(CvSeqWriter *writer);

/* Creates a new set */
CVAPI(CvSet *)
cvCreateSet(int set_flags, int header_size,
            int elem_size, CvMemStorage *storage);

/* Adds new element to the set and returns pointer to it */
CVAPI(int)
cvSetAdd(CvSet *set_header, CvSetElem *elem VOS_DEFAULT(NULL),
         CvSetElem **inserted_elem VOS_DEFAULT(NULL));

CVAPI(int)
cvStdErrReport(int status, const char *func_name, const char *err_msg,
               const char *file_name, int line, void *userdata);

/* Inserts sequence into tree with specified "parent" sequence.
   If parent is equal to frame (e.g. the most external contour),
   then added contour will have null pointer to parent. */
CVAPI(void)
cvInsertNodeIntoTree(void *node, void *parent, void *frame);

/****************************************************************************************\
*                                    System functions                                    *
\****************************************************************************************/

/* Get current   error status */
CVAPI(int)
cvGetErrStatus(void);

/* Sets error status silently */
CVAPI(void)
cvSetErrStatus(int status);

#define VOS_ErrModeLeaf 0   /* Print error and exit program */
#define VOS_ErrModeParent 1 /* Print error and continue */
#define VOS_ErrModeSilent 2 /* Don't print and continue */

/* Retrives current error processing mode */
CVAPI(int)
cvGetErrMode(void);

/* Sets error processing mode, returns previously used mode */
CVAPI(int)
cvSetErrMode(int mode);

/* Sets error status and performs some additonal actions (displaying message box,
   writing message to stderr, terminating application etc.)
   depending on the current error mode */
CVAPI(void)
cvError(int status, const char *func_name,
        const char *err_msg, const char *file_name, int line);

/* Retrieves textual description of the error given its code */
CVAPI(const char *)
cvErrorStr(int status);

CVAPI(int)
cvErrorFromStatus(int ipp_status);

typedef int(VOS_CDECL *CvErrorCallback)(int status, const char *func_name,
                                        const char *err_msg, const char *file_name, int line, void *userdata);


#ifdef __cplusplus
}

#include "cxcore.hpp"
#endif

#endif /*_CXCORE_H_*/

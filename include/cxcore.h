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
void *
cvAlloc(size_t size);

/* <free> wrapper.
   Here and further all the memory releasing functions
   (that all call cvFree) take double pointer in order to
   to clear pointer to the data after releasing it.
   Passing pointer to NULL pointer is Ok: nothing happens in this case
*/
void
cvFree_(void *ptr);
#define cvFree(ptr) (cvFree_(*(ptr)), *(ptr) = 0)

/* Allocates and initializes IplImage header */
IplImage *
CreateImageHeader(Size size, int depth, int channels);

/* Inializes IplImage header */
IplImage *
InitImageHeader(IplImage *image, Size size, int depth,
                  int channels, int origin VOS_DEFAULT(0),
                  int align VOS_DEFAULT(4));

/* Creates IPL image (header and data) */
IplImage *
cvCreateImage(Size size, int depth, int channels);

void
ReleaseImageHeader(IplImage **image);

/* Releases IPL image header and data */
void
ReleaseImage(IplImage **image);

/* Creates a copy of IPL image (widthStep may differ) */
IplImage *
CloneImage(const IplImage *image);

/* Allocates and initalizes Mat header */
Mat *
CreateMatHeader(int rows, int cols, int type);

#define VOS_AUTOSTEP 0x7fffffff

/* Initializes Mat header */
Mat *
InitMatHeader(Mat *mat, int rows, int cols,
                int type, void *data VOS_DEFAULT(NULL),
                int step VOS_DEFAULT(VOS_AUTOSTEP));

/* Allocates and initializes Mat header and allocates data */
Mat *
CreateMat(int rows, int cols, int type);

/* Releases Mat header and deallocates matrix data
   (reference counting is used for data) */
void
ReleaseMat(Mat **mat);

/* Decrements Mat data reference counter and deallocates the data if
   it reaches 0 */
VOS_INLINE void cvDecRefData(CvArr *arr)
{
    if (VOS_IS_MAT(arr))
    {
        Mat *mat = (Mat *)arr;
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
void
ScalarToRawData(const Scalar *scalar, void *data, int type);

/**************** matrix iterator: used for n-ary operations on dense arrays *********/

int
GetElemType(const CvArr *arr);

Mat *
GetMat(const CvArr *arr, Mat *header,
         int *coi VOS_DEFAULT(NULL),
         int allowND VOS_DEFAULT(0));

Mat *
Reshape(const CvArr *arr, Mat *header,
          int new_cn, int new_rows VOS_DEFAULT(0));

/* Allocates array data */
void
CreateData(CvArr *arr);

/* Releases array data */
void
ReleaseData(CvArr *arr);

/* Attaches user data to the array header. The step is reffered to
   the pre-last dimension. That is, all the planes of the array
   must be joint (w/o gaps) */
void
SetData(CvArr *arr, void *data, int step);

/* Returns width and height of array in elements */
Size
GetSize(const CvArr *arr);

/* Copies source array to destination array */
void
cvCopy(const CvArr *src, CvArr *dst,
       const CvArr *mask VOS_DEFAULT(NULL));

/* Performs linear transformation on every source array element:
   dst(x,y,c) = scale*src(x,y,c)+shift.
   Arbitrary combination of input and output array depths are allowed
   (number of channels must be the same), thus the function can be used
   for type conversion */
void
cvConvertScale(const CvArr *src, CvArr *dst,
               double scale VOS_DEFAULT(1),
               double shift VOS_DEFAULT(0));
#define cvConvert(src, dst) cvConvertScale((src), (dst), 1, 0)

/****************************************************************************************\
*                                Math operations                                         *
\****************************************************************************************/

/* Does powering: dst(idx) = src(idx)^power */
void
cvPow(const CvArr *src, CvArr *dst, double power);

/****************************************************************************************\
*                                Matrix operations                                       *
\****************************************************************************************/

#define VOS_SVD_MODIFY_A 1
#define VOS_SVD_U_T 2
#define VOS_SVD_V_T 4

/* Performs Singular Value Decomposition of a matrix */
void
cvSVD(CvArr *A, CvArr *W, CvArr *U VOS_DEFAULT(NULL),
      CvArr *V VOS_DEFAULT(NULL), int flags VOS_DEFAULT(0));

/* Performs Singular Value Back Substitution (solves A*X = B):
   flags must be the same as in cvSVD */
void
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
int
SliceLength(Slice slice, const Seq_t *seq);

/* Creates new memory storage.
   block_size == 0 means that default,
   somewhat optimal size, is used (currently, it is 64K) */
MemStorage *
CreateMemStorage(int block_size VOS_DEFAULT(0));

/* Creates a memory storage that will borrow memory blocks from parent storage */
MemStorage *
CreateChildMemStorage(MemStorage *parent);

/* Releases memory storage. All the children of a parent must be released before
   the parent. A child storage returns all the blocks to parent when it is released */
void
ReleaseMemStorage(MemStorage **storage);

/* Clears memory storage. This is the only way(!!!) (besides RestoreMemStoragePos)
   to reuse memory allocated for the storage - cvClearSeq,cvClearSet ...
   do not free any memory.
   A child storage returns all the blocks to the parent when it is cleared */
void
ClearMemStorage(MemStorage *storage);

/* Remember a storage "free memory" position */
void
SaveMemStoragePos(const MemStorage *storage, MemStoragePos *pos);

/* Restore a storage "free memory" position */
void
RestoreMemStoragePos(MemStorage *storage, MemStoragePos *pos);

/* Allocates continuous buffer of the specified size in the storage */
void *
MemStorageAlloc(MemStorage *storage, size_t size);

/* Creates new empty sequence that will reside in the specified storage */
Seq_t *
CreateSeq(int seq_flags, int header_size,
            int elem_size, MemStorage *storage);

/* Changes default size (granularity) of sequence blocks.
   The default size is ~1Kbyte */
void
SetSeqBlockSize(Seq_t *seq, int delta_elems);

/* Adds new element to the end of sequence. Returns pointer to the element */
char *
cvSeqPush(Seq_t *seq, void *element VOS_DEFAULT(NULL));

/* Removes the last element from sequence and optionally saves it */
void
cvSeqPop(Seq_t *seq, void *element VOS_DEFAULT(NULL));

#define VOS_FRONT 1
#define VOS_BACK 0

/* Removes several elements from the end of sequence and optionally saves them */
void
cvSeqPopMulti(Seq_t *seq, void *elements,
              int count, int in_front VOS_DEFAULT(0));


void
cvClearSeq(Seq_t *seq);


char *
GetSeqElem(const Seq_t *seq, int index);

/* Initializes sequence writer. The new elements will be added to the end of sequence */
void
StartAppendToSeq(Seq_t *seq, CvSeqWriter *writer);

/* Combination of CreateSeq and StartAppendToSeq */
void
StartWriteSeq(int seq_flags, int header_size,
                int elem_size, MemStorage *storage,
                CvSeqWriter *writer);


Seq_t *
EndWriteSeq(CvSeqWriter *writer);

/* Updates sequence header. May be useful to get access to some of previously
   written elements via GetSeqElem or sequence reader */
void
FlushSeqWriter(CvSeqWriter *writer);

/* Initializes sequence reader.
   The sequence can be read in forward or backward direction */
void
StartReadSeq(const Seq_t *seq, CvSeqReader *reader,
               int reverse VOS_DEFAULT(0));

/* Returns current sequence reader position (currently observed sequence element) */
int
cvGetSeqReaderPos(CvSeqReader *reader);

/* Changes sequence reader position. It may seek to an absolute or
   to relative to the current position */
void
SetSeqReaderPos(CvSeqReader *reader, int index,
                  int is_relative VOS_DEFAULT(0));

/************ Internal sequence functions ************/
void
cvChangeSeqBlock(void *reader, int direction);
void
CreateSeqBlock(CvSeqWriter *writer);

/* Creates a new set */
Set *
CreateSet(int set_flags, int header_size,
            int elem_size, MemStorage *storage);

/* Adds new element to the set and returns pointer to it */
int
cvSetAdd(Set *set_header, SetElem_t *elem VOS_DEFAULT(NULL),
         SetElem_t **inserted_elem VOS_DEFAULT(NULL));

int
cvStdErrReport(int status, const char *func_name, const char *err_msg,
               const char *file_name, int line, void *userdata);

/* Inserts sequence into tree with specified "parent" sequence.
   If parent is equal to frame (e.g. the most external contour),
   then added contour will have null pointer to parent. */
void
cvInsertNodeIntoTree(void *node, void *parent, void *frame);

/****************************************************************************************\
*                                    System functions                                    *
\****************************************************************************************/

/* Get current   error status */
int
cvGetErrStatus(void);

/* Sets error status silently */
void
cvSetErrStatus(int status);

#define VOS_ErrModeLeaf 0   /* Print error and exit program */
#define VOS_ErrModeParent 1 /* Print error and continue */
#define VOS_ErrModeSilent 2 /* Don't print and continue */

/* Retrives current error processing mode */
int
cvGetErrMode(void);

/* Sets error processing mode, returns previously used mode */
int
cvSetErrMode(int mode);

/* Sets error status and performs some additonal actions (displaying message box,
   writing message to stderr, terminating application etc.)
   depending on the current error mode */
void
cvError(int status, const char *func_name,
        const char *err_msg, const char *file_name, int line);

/* Retrieves textual description of the error given its code */
const char *
cvErrorStr(int status);

int
cvErrorFromStatus(int ipp_status);

typedef int(VOS_CDECL *CvErrorCallback)(int status, const char *func_name,
                                        const char *err_msg, const char *file_name, int line, void *userdata);


#ifdef __cplusplus
}

#include "cxcore.hpp"
#endif

#endif /*_CXCORE_H_*/

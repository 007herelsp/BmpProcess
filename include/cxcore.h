#ifndef _CXCORE_H_
#define _CXCORE_H_

#include "cxtypes.h"
#include "cxerror.h"

#ifdef __cplusplus
extern "C" {
#endif

void *
SysAlloc(size_t size);

void
SysFree_(void *ptr);
#define SYS_FREE(ptr) (SysFree_(*(ptr)), *(ptr) = 0)

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
CreateImage(Size size, int depth, int channels);

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
VOS_INLINE void DecRefData(CvArr *arr)
{
    if (VOS_IS_MAT(arr))
    {
        Mat *mat = (Mat *)arr;
        mat->data.ptr = NULL;
        if (mat->refcount != NULL && --*mat->refcount == 0)
            SYS_FREE(&mat->refcount);
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

void
SetData(CvArr *arr, void *data, int step);

/* Returns width and height of array in elements */
Size
GetSize(const CvArr *arr);

/* Copies source array to destination array */
void
Copy(const CvArr *src, CvArr *dst,
       const CvArr *mask VOS_DEFAULT(NULL));

void
ConvertScale(const CvArr *src, CvArr *dst,
               double scale VOS_DEFAULT(1),
               double shift VOS_DEFAULT(0));
#define Convert(src, dst) ConvertScale((src), (dst), 1, 0)

/* Does powering: dst(idx) = src(idx)^power */
void
SysPow(const CvArr *src, CvArr *dst, double power);

#define VOS_SVD_MODIFY_A 1
#define VOS_SVD_U_T 2
#define VOS_SVD_V_T 4

/* Performs Singular Value Decomposition of a matrix */
void
SVD(CvArr *A, CvArr *W, CvArr *U VOS_DEFAULT(NULL),
      CvArr *V VOS_DEFAULT(NULL), int flags VOS_DEFAULT(0));

/* Performs Singular Value Back Substitution (solves A*X = B):
   flags must be the same as in SVD */
void
SVBkSb(const CvArr *W, const CvArr *U,
         const CvArr *V, const CvArr *B,
         CvArr *X, int flags);

#define VOS_LU 0
#define VOS_SVD 1
#define VOS_SVD_SYM 2

int
SliceLength(Slice slice, const Seq *seq);

MemStorage *
CreateMemStorage(int block_size VOS_DEFAULT(0));

/* Creates a memory storage that will borrow memory blocks from parent storage */
MemStorage *
CreateChildMemStorage(MemStorage *parent);

void
ReleaseMemStorage(MemStorage **storage);

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
Seq *
CreateSeq(int seq_flags, int header_size,
            int elem_size, MemStorage *storage);


void
SetSeqBlockSize(Seq *seq, int delta_elems);

/* Adds new element to the end of sequence. Returns pointer to the element */
char *
SeqPush(Seq *seq, void *element VOS_DEFAULT(NULL));

/* Removes the last element from sequence and optionally saves it */
void
SeqPop(Seq *seq, void *element VOS_DEFAULT(NULL));

#define VOS_FRONT 1
#define VOS_BACK 0

/* Removes several elements from the end of sequence and optionally saves them */
void
SeqPopMulti(Seq *seq, void *elements,
              int count, int in_front VOS_DEFAULT(0));


void
ClearSeq(Seq *seq);


char *
GetSeqElem(const Seq *seq, int index);

/* Initializes sequence writer. The new elements will be added to the end of sequence */
void
StartAppendToSeq(Seq *seq, SeqWriter *writer);

/* Combination of CreateSeq and StartAppendToSeq */
void
StartWriteSeq(int seq_flags, int header_size,
                int elem_size, MemStorage *storage,
                SeqWriter *writer);


Seq *
EndWriteSeq(SeqWriter *writer);

void
FlushSeqWriter(SeqWriter *writer);

void
StartReadSeq(const Seq *seq, SeqReader *reader,
               int reverse VOS_DEFAULT(0));

/* Returns current sequence reader position (currently observed sequence element) */
int
GetSeqReaderPos(SeqReader *reader);

/* Changes sequence reader position. It may seek to an absolute or
   to relative to the current position */
void
SetSeqReaderPos(SeqReader *reader, int index,
                  int is_relative VOS_DEFAULT(0));

/************ Internal sequence functions ************/
void
ChangeSeqBlock(void *reader, int direction);
void
CreateSeqBlock(SeqWriter *writer);

/* Creates a new set */
Set *
CreateSet(int set_flags, int header_size,
            int elem_size, MemStorage *storage);

/* Adds new element to the set and returns pointer to it */
int
SetAdd(Set *set_header, SetElem *elem VOS_DEFAULT(NULL),
         SetElem **inserted_elem VOS_DEFAULT(NULL));

int
StdErrReport(int status, const char *func_name, const char *err_msg,
               const char *file_name, int line, void *userdata);

void
InsertNodeIntoTree(void *node, void *parent, void *frame);

int
GetErrStatus(void);

/* Sets error status silently */
void
SetErrStatus(int status);

#define VOS_ErrModeLeaf 0   /* Print error and exit program */
#define VOS_ErrModeParent 1 /* Print error and continue */
#define VOS_ErrModeSilent 2 /* Don't print and continue */

/* Retrives current error processing mode */
int
GetErrMode(void);

/* Sets error processing mode, returns previously used mode */
int
SetErrMode(int mode);

void
SysError(int status, const char *func_name,
        const char *err_msg, const char *file_name, int line);

/* Retrieves textual description of the error given its code */
const char *
SysErrorStr(int status);

int
SysErrorFromStatus(int ipp_status);

typedef int(*SysErrorCallback)(int status, const char *func_name,
                                        const char *err_msg, const char *file_name, int line, void *userdata);

#ifdef __cplusplus
}

#endif

#endif /*_CXCORE_H_*/

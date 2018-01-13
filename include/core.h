#ifndef _CXCORE_H_
#define _CXCORE_H_

#include "types.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

void *
SysAlloc(size_t size);

void
SysFree_(void *ptr);
#define SYS_FREE(ptr) (SysFree_(*(ptr)), *(ptr) = 0)

IplImage *
CreateImageHeader(Size size, int depth, int channels);

IplImage *
InitImageHeader(IplImage *image, Size size, int depth,
                  int channels, int origin VOS_DEFAULT(0),
                  int align VOS_DEFAULT(4));

IplImage *
CreateImage(Size size, int depth, int channels);

void
ReleaseImageHeader(IplImage **image);

void
ReleaseImage(IplImage **image);

IplImage *
CloneImage(const IplImage *image);

Mat *
CreateMatHeader(int rows, int cols, int type);

#define VOS_AUTOSTEP 0x7fffffff

Mat *
InitMatHeader(Mat *mat, int rows, int cols,
                int type, void *data VOS_DEFAULT(NULL),
                int step VOS_DEFAULT(VOS_AUTOSTEP));

Mat *
CreateMat(int rows, int cols, int type);

void
ReleaseMat(Mat **mat);

VOS_INLINE void DecRefData(Mat *arr)
{
        Mat *mat = (Mat *)arr;
        mat->data.ptr = NULL;
        if (mat->refcount != NULL && --*mat->refcount == 0)
            SYS_FREE(&mat->refcount);
        mat->refcount = NULL;
}

void
ScalarToRawData(const Scalar *scalar, void *data, int type);

Mat *
GetMat(const VOID *arr, Mat *header,
         int *coi VOS_DEFAULT(NULL));


void
CreateData(VOID *arr);

void
ReleaseData(VOID *arr);

void
SetData(VOID *arr, void *data, int step);

Size
GetSize(const VOID *arr);
void
Copy(const VOID *src, VOID *dst);

void
ConvertScale(const Mat *src, Mat *dst);
#define Convert(src, dst) ConvertScale((src), (dst))

void
SysPow(const Mat *src, Mat *dst, double power);

#define VOS_SVD_MODIFY_A 1
#define VOS_SVD_U_T 2
#define VOS_SVD_V_T 4

void
SVD(VOID *A, VOID *W, VOID *U VOS_DEFAULT(NULL),
      VOID *V VOS_DEFAULT(NULL), int flags VOS_DEFAULT(0));

void
SVBkSb(const VOID *W, const VOID *U,
         const VOID *V, const VOID *B,
         VOID *X, int flags);

#define VOS_SVD 1
#define VOS_SVD_SYM 2

int
SliceLength(Slice slice, const Seq *seq);

MemStorage *
CreateMemStorage(int block_size VOS_DEFAULT(0));

MemStorage *
CreateChildMemStorage(MemStorage *parent);

void
ReleaseMemStorage(MemStorage **storage);

void
ClearMemStorage(MemStorage *storage);

void
SaveMemStoragePos(const MemStorage *storage, MemStoragePos *pos);

void
RestoreMemStoragePos(MemStorage *storage, MemStoragePos *pos);

void *
MemStorageAlloc(MemStorage *storage, size_t size);

Seq *
CreateSeq(int seq_flags, int header_size,
            int elem_size, MemStorage *storage);


void
SetSeqBlockSize(Seq *seq, int delta_elems);

char *
SeqPush(Seq *seq, void *element VOS_DEFAULT(NULL));

void
SeqPop(Seq *seq, void *element VOS_DEFAULT(NULL));

void
SeqPopMulti(Seq *seq, void *elements,
              int count, int in_front VOS_DEFAULT(0));


char *
GetSeqElem(const Seq *seq, int index);

void
StartAppendToSeq(Seq *seq, SeqWriter *writer);

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

int
GetSeqReaderPos(SeqReader *reader);

void
SetSeqReaderPos(SeqReader *reader, int index,
                  int is_relative VOS_DEFAULT(0));

void
ChangeSeqBlock(void *reader, int direction);
void
CreateSeqBlock(SeqWriter *writer);

int
StdErrReport(int status, const char *func_name, const char *err_msg,
               const char *file_name, int line, void *userdata);

void
InsertNodeIntoTree(void *node, void *parent, void *frame);

int
GetErrStatus(void);

void
SetErrStatus(int status);

#define VOS_ErrModeLeaf 0   /* Print error and exit program */
#define VOS_ErrModeParent 1 /* Print error and continue */
#define VOS_ErrModeSilent 2 /* Don't print and continue */


void
SysError(int status, const char *func_name,
        const char *err_msg, const char *file_name, int line);

const char *
SysErrorStr(int status);

typedef int(*SysErrorCallback)(int status, const char *func_name,
                                        const char *err_msg, const char *file_name, int line, void *userdata);

#ifdef __cplusplus
}

#endif

#endif /*_CXCORE_H_*/

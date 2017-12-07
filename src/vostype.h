#ifndef VOS_TYPE_H
#define VOS_TYPE_H

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef int INT32;
typedef signed char schar;
#define VOID void

typedef U8 *LPBYTE;

typedef struct CvPoint
{
    int x;
    int y;
} CvPoint;

typedef struct stPoint
{
    INT32 x;
    INT32 y;
} Point, *lpPoint;

typedef struct stLine
{
    INT32 x0;
    INT32 y0;
    INT32 x1;
    INT32 y1;
} Line, *lpLine;

#ifndef VOS_INLINE
#if defined __cplusplus
#define VOS_INLINE inline
#else
#define VOS_INLINE static
#endif
#endif /* VOS_INLINE */

VOS_INLINE Point Point(int x, int y)
{
    Point p;

    p.x = x;
    p.y = y;

    return p;
}

typedef struct CvSeqBlock
{
    struct CvSeqBlock *prev; /* Previous sequence block.                   */
    struct CvSeqBlock *next; /* Next sequence block.                       */
    int start_index;         /* Index of the first element in the block +  */
                             /* sequence->first->start_index.              */
    int count;               /* Number of elements in the block.           */
    schar *data;             /* Pointer to the first element of the block. */
} CvSeqBlock;

typedef struct CvMemBlock
{
    struct CvMemBlock *prev;
    struct CvMemBlock *next;
} CvMemBlock;

typedef struct CvMemStorage
{
    int signature;
    CvMemBlock *bottom;          /* First allocated block.                   */
    CvMemBlock *top;             /* Current memory block - top of the stack. */
    struct CvMemStorage *parent; /* We get new blocks from parent as needed. */
    int block_size;              /* Block size.                              */
    int free_space;              /* Remaining free space in current block.   */
} CvMemStorage;

#define TREE_NODE_FIELDS(node_type)                          \
    int flags;                /* Miscellaneous flags.     */ \
    int header_size;          /* Size of sequence header. */ \
    struct node_type *h_prev; /* Previous sequence.       */ \
    struct node_type *h_next; /* Next sequence.           */ \
    struct node_type *v_prev; /* 2nd previous sequence.   */ \
    struct node_type *v_next  /* 2nd next sequence.       */

#define SEQUENCE_FIELDS()                                               \
    TREE_NODE_FIELDS(Seq);                                              \
    int total;               /* Total number of elements.            */ \
    int elem_size;           /* Size of sequence element in bytes.   */ \
    schar *block_max;        /* Maximal bound of the last block.     */ \
    schar *ptr;              /* Current write pointer.               */ \
    int delta_elems;         /* Grow seq this many at a time.        */ \
    CvMemStorage *storage;   /* Where the seq is stored.             */ \
    CvSeqBlock *free_blocks; /* Free blocks list.                    */ \
    CvSeqBlock *first;       /* Pointer to the first sequence block. */

typedef struct stSeq
{
    SEQUENCE_FIELDS()
} Seq;

typedef struct stChain
{
    CV_SEQUENCE_FIELDS()
    Point origin;
} Chain;

#endif

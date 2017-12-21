
#include "_cxcore.h"

#define ICV_FREE_PTR(storage)  \
    ((char*)(storage)->top + (storage)->block_size - (storage)->free_space)

#define ICV_ALIGNED_SEQ_BLOCK_SIZE  \
    (int)cvAlign(sizeof(CvSeqBlock), CV_STRUCT_ALIGN)

CV_INLINE int
cvAlignLeft( int size, int align )
{
    return size & -align;
}

#define CV_GET_LAST_ELEM( seq, block ) \
    ((block)->data + ((block)->count - 1)*((seq)->elem_size))

#define CV_SWAP_ELEMS(a,b,elem_size)  \
{                                     \
    int k;                            \
    for( k = 0; k < elem_size; k++ )  \
    {                                 \
        char t0 = (a)[k];             \
        char t1 = (b)[k];             \
        (a)[k] = t1;                  \
        (b)[k] = t0;                  \
    }                                 \
}

#define ICV_SHIFT_TAB_MAX 32
static const char icvPower2ShiftTab[] =
{
    0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
};

/****************************************************************************************\
*            Functions for manipulating memory storage - list of memory blocks           *
\****************************************************************************************/

/* initializes allocated storage */
static void
icvInitMemStorage( CvMemStorage* storage, int block_size )
{
    CV_FUNCNAME( "icvInitMemStorage " );
    
    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( block_size <= 0 )
        block_size = CV_STORAGE_BLOCK_SIZE;

    block_size = cvAlign( block_size, CV_STRUCT_ALIGN );
    assert( sizeof(CvMemBlock) % CV_STRUCT_ALIGN == 0 );

    memset( storage, 0, sizeof( *storage ));
    storage->signature = CV_STORAGE_MAGIC_VAL;
    storage->block_size = block_size;

    __END__;
}


/* creates root memory storage */
CV_IMPL CvMemStorage*
cvCreateMemStorage( int block_size )
{
    CvMemStorage *storage = 0;

    CV_FUNCNAME( "cvCreateMemStorage" );

    __BEGIN__;

    CV_CALL( storage = (CvMemStorage *)cvAlloc( sizeof( CvMemStorage )));
    CV_CALL( icvInitMemStorage( storage, block_size ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}


/* creates child memory storage */
CV_IMPL CvMemStorage *
cvCreateChildMemStorage( CvMemStorage * parent )
{
    CvMemStorage *storage = 0;
    CV_FUNCNAME( "cvCreateChildMemStorage" );

    __BEGIN__;

    if( !parent )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( storage = cvCreateMemStorage(parent->block_size));
    storage->parent = parent;

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}


/* releases all blocks of the storage (or returns them to parent if any) */
static void
icvDestroyMemStorage( CvMemStorage* storage )
{
    CV_FUNCNAME( "icvDestroyMemStorage" );

    __BEGIN__;

    int k = 0;

    CvMemBlock *block;
    CvMemBlock *dst_top = 0;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
        dst_top = storage->parent->top;

    for( block = storage->bottom; block != 0; k++ )
    {
        CvMemBlock *temp = block;

        block = block->next;
        if( storage->parent )
        {
            if( dst_top )
            {
                temp->prev = dst_top;
                temp->next = dst_top->next;
                if( temp->next )
                    temp->next->prev = temp;
                dst_top = dst_top->next = temp;
            }
            else
            {
                dst_top = storage->parent->bottom = storage->parent->top = temp;
                temp->prev = temp->next = 0;
                storage->free_space = storage->block_size - sizeof( *temp );
            }
        }
        else
        {
            cvFree( &temp );
        }
    }

    storage->top = storage->bottom = 0;
    storage->free_space = 0;

    __END__;
}


/* releases memory storage */
CV_IMPL void
cvReleaseMemStorage( CvMemStorage** storage )
{
    CvMemStorage *st;
    CV_FUNCNAME( "cvReleaseMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    st = *storage;
    *storage = 0;

    if( st )
    {
        CV_CALL( icvDestroyMemStorage( st ));
        cvFree( &st );
    }

    __END__;
}


/* clears memory storage (returns blocks to the parent if any) */
CV_IMPL void
cvClearMemStorage( CvMemStorage * storage )
{
    CV_FUNCNAME( "cvClearMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
    {
        icvDestroyMemStorage( storage );
    }
    else
    {
        storage->top = storage->bottom;
        storage->free_space = storage->bottom ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}


/* moves stack pointer to next block.
   If no blocks, allocate new one and link it to the storage */
static void
icvGoNextMemBlock( CvMemStorage * storage )
{
    CV_FUNCNAME( "icvGoNextMemBlock" );
    
    __BEGIN__;
    
    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !storage->top || !storage->top->next )
    {
        CvMemBlock *block;

        if( !(storage->parent) )
        {
            CV_CALL( block = (CvMemBlock *)cvAlloc( storage->block_size ));
        }
        else
        {
            CvMemStorage *parent = storage->parent;
            CvMemStoragePos parent_pos;

            cvSaveMemStoragePos( parent, &parent_pos );
            CV_CALL( icvGoNextMemBlock( parent ));

            block = parent->top;
            cvRestoreMemStoragePos( parent, &parent_pos );

            if( block == parent->top )  /* the single allocated block */
            {
                assert( parent->bottom == block );
                parent->top = parent->bottom = 0;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if( block->next )
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = 0;
        block->prev = storage->top;

        if( storage->top )
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if( storage->top->next )
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(CvMemBlock);
    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    __END__;
}


/* remembers memory storage position */
CV_IMPL void
cvSaveMemStoragePos( const CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvSaveMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );

    pos->top = storage->top;
    pos->free_space = storage->free_space;

    __END__;
}


/* restores memory storage position */
CV_IMPL void
cvRestoreMemStoragePos( CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvRestoreMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );
    if( pos->free_space > storage->block_size )
        CV_ERROR( CV_StsBadSize, "" );

    /*
    // this breaks icvGoNextMemBlock, so comment it off for now
    if( storage->parent && (!pos->top || pos->top->next) )
    {
        CvMemBlock* save_bottom;
        if( !pos->top )
            save_bottom = 0;
        else
        {
            save_bottom = storage->bottom;
            storage->bottom = pos->top->next;
            pos->top->next = 0;
            storage->bottom->prev = 0;
        }
        icvDestroyMemStorage( storage );
        storage->bottom = save_bottom;
    }*/

    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if( !storage->top )
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}


/* Allocates continuous buffer of the specified size in the storage */
CV_IMPL void*
cvMemStorageAlloc( CvMemStorage* storage, size_t size )
{
    char *ptr = 0;
    
    CV_FUNCNAME( "cvMemStorageAlloc" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );

    if( size > INT_MAX )
        CV_ERROR( CV_StsOutOfRange, "Too large memory block is requested" );

    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    if( (size_t)storage->free_space < size )
    {
        size_t max_free_space = cvAlignLeft(storage->block_size - sizeof(CvMemBlock), CV_STRUCT_ALIGN);
        if( max_free_space < size )
            CV_ERROR( CV_StsOutOfRange, "requested size is negative or too big" );

        CV_CALL( icvGoNextMemBlock( storage ));
    }

    ptr = ICV_FREE_PTR(storage);
    assert( (size_t)ptr % CV_STRUCT_ALIGN == 0 );
    storage->free_space = cvAlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN );

    __END__;

    return ptr;
}


CV_IMPL CvString
cvMemStorageAllocString( CvMemStorage* storage, const char* ptr, int len )
{
    CvString str;
    CV_FUNCNAME( "cvMemStorageAllocString" );

    __BEGIN__;

    str.len = len >= 0 ? len : (int)strlen(ptr);
    CV_CALL( str.ptr = (char*)cvMemStorageAlloc( storage, str.len + 1 ));
    memcpy( str.ptr, ptr, str.len );
    str.ptr[str.len] = '\0';

    __END__;

    return str;
}


/****************************************************************************************\
*                               Sequence implementation                                  *
\****************************************************************************************/

/* creates empty sequence */
CV_IMPL CvSeq *
cvCreateSeq( int seq_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvCreateSeq" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSeq ) || elem_size <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    /* allocate sequence header */
    CV_CALL( seq = (CvSeq*)cvMemStorageAlloc( storage, header_size ));
    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Specified element size doesn't match to the size of the specified element type "
            "(try to use 0 for element type)" );
    }
    seq->elem_size = elem_size;
    seq->storage = storage;

    CV_CALL( cvSetSeqBlockSize( seq, (1 << 10)/elem_size ));

    __END__;

    return seq;
}


/* adjusts <delta_elems> field of sequence. It determines how much the sequence
   grows if there are no free space inside the sequence buffers */
CV_IMPL void
cvSetSeqBlockSize( CvSeq *seq, int delta_elements )
{
    int elem_size;
    int useful_block_size;

    CV_FUNCNAME( "cvSetSeqBlockSize" );

    __BEGIN__;

    if( !seq || !seq->storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( delta_elements < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    useful_block_size = cvAlignLeft(seq->storage->block_size - sizeof(CvMemBlock) -
                                    sizeof(CvSeqBlock), CV_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if( delta_elements == 0 )
    {
        delta_elements = (1 << 10) / elem_size;
        delta_elements = MAX( delta_elements, 1 );
    }
    if( delta_elements * elem_size > useful_block_size )
    {
        delta_elements = useful_block_size / elem_size;
        if( delta_elements == 0 )
            CV_ERROR( CV_StsOutOfRange, "Storage block size is too small "
                                        "to fit the sequence elements" );
    }

    seq->delta_elems = delta_elements;

    __END__;
}


/* finds sequence element by its index */
CV_IMPL char*
cvGetSeqElem( const CvSeq *seq, int index )
{
    CvSeqBlock *block;
    int count, total = seq->total;

    if( (unsigned)index >= (unsigned)total )
    {
        index += index < 0 ? total : 0;
        index -= index >= total ? total : 0;
        if( (unsigned)index >= (unsigned)total )
            return 0;
    }

    block = seq->first;
    if( index + index <= total )
    {
        while( index >= (count = block->count) )
        {
            block = block->next;
            index -= count;
        }
    }
    else
    {
        do
        {
            block = block->prev;
            total -= block->count;
        }
        while( index < total );
        index -= total;
    }

    return block->data + index * seq->elem_size;
}


CV_IMPL int
cvSliceLength( CvSlice slice, const CvSeq* seq )
{
    int total = seq->total;
    int length = slice.end_index - slice.start_index;
    
    if( length != 0 )
    {
        if( slice.start_index < 0 )
            slice.start_index += total;
        if( slice.end_index <= 0 )
            slice.end_index += total;

        length = slice.end_index - slice.start_index;
    }

    if( length < 0 )
    {
        length += total;
        /*if( length < 0 )
            length += total;*/
    }
    else if( length > total )
        length = total;

    return length;
}


/* copies all the sequence elements into single continuous array */
CV_IMPL void*
cvCvtSeqToArray( const CvSeq *seq, void *array, CvSlice slice )
{
    CV_FUNCNAME( "cvCvtSeqToArray" );

    __BEGIN__;

    int elem_size, total;
    CvSeqReader reader;
    char *dst = (char*)array;

    if( !seq || !array )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    total = cvSliceLength( slice, seq )*elem_size;
    
    if( total == 0 )
        EXIT;
    
    cvStartReadSeq( seq, &reader, 0 );
    CV_CALL( cvSetSeqReaderPos( &reader, slice.start_index, 0 ));

    do
    {
        int count = (int)(reader.block_max - reader.ptr);
        if( count > total )
            count = total;

        memcpy( dst, reader.ptr, count );
        dst += count;
        reader.block = reader.block->next;
        reader.ptr = reader.block->data;
        reader.block_max = reader.ptr + reader.block->count*elem_size;
        total -= count;
    }
    while( total > 0 );

    __END__;

    return array;
}


/* constructs sequence from array without copying any data.
   the resultant sequence can't grow above its initial size */
CV_IMPL CvSeq*
cvMakeSeqHeaderForArray( int seq_flags, int header_size, int elem_size,
                         void *array, int total, CvSeq *seq, CvSeqBlock * block )
{
    CvSeq* result = 0;
    
    CV_FUNCNAME( "cvMakeSeqHeaderForArray" );

    __BEGIN__;

    if( elem_size <= 0 || header_size < (int)sizeof( CvSeq ) || total < 0 )
        CV_ERROR( CV_StsBadSize, "" );

    if( !seq || ((!array || !block) && total > 0) )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Element size doesn't match to the size of predefined element type "
            "(try to use 0 for sequence element type)" );
    }
    seq->elem_size = elem_size;
    seq->total = total;
    seq->block_max = seq->ptr = (char *) array + total * elem_size;

    if( total > 0 )
    {
        seq->first = block;
        block->prev = block->next = block;
        block->start_index = 0;
        block->count = total;
        block->data = (char *) array;
    }

    result = seq;

    __END__;

    return result;
}


/* the function allocates space for at least one more sequence element.
   if there are free sequence blocks (seq->free_blocks != 0),
   they are reused, otherwise the space is allocated in the storage */
static void
icvGrowSeq( CvSeq *seq, int in_front_of )
{
    CV_FUNCNAME( "icvGrowSeq" );

    __BEGIN__;

    CvSeqBlock *block;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    block = seq->free_blocks;

    if( !block )
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        CvMemStorage *storage = seq->storage;

        if( seq->total >= delta_elems*4 )
            cvSetSeqBlockSize( seq, delta_elems*2 );

        if( !storage )
            CV_ERROR( CV_StsNullPtr, "The sequence has NULL storage pointer" );

        /* if there is a free space just after last allocated block
           and it's big enough then enlarge the last block
           (this can happen only if the new block is added to the end of sequence */
        if( (unsigned)(ICV_FREE_PTR(storage) - seq->block_max) < CV_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = MIN( delta, delta_elems ) * elem_size;
            seq->block_max += delta;
            storage->free_space = cvAlignLeft((int)(((char*)storage->top + storage->block_size) -
                                              seq->block_max), CV_STRUCT_ALIGN );
            EXIT;
        }
        else
        {
            int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

            /* try to allocate <delta_elements> elements */
            if( storage->free_space < delta )
            {
                int small_block_size = MAX(1, delta_elems/3)*elem_size +
                                       ICV_ALIGNED_SEQ_BLOCK_SIZE;
                /* try to allocate smaller part */
                if( storage->free_space >= small_block_size + CV_STRUCT_ALIGN )
                {
                    delta = (storage->free_space - ICV_ALIGNED_SEQ_BLOCK_SIZE)/seq->elem_size;
                    delta = delta*seq->elem_size + ICV_ALIGNED_SEQ_BLOCK_SIZE;
                }
                else
                {
                    CV_CALL( icvGoNextMemBlock( storage ));
                    assert( storage->free_space >= delta );
                }
            }

            CV_CALL( block = (CvSeqBlock*)cvMemStorageAlloc( storage, delta ));
            block->data = (char*)cvAlignPtr( block + 1, CV_STRUCT_ALIGN );
            block->count = delta - ICV_ALIGNED_SEQ_BLOCK_SIZE;
            block->prev = block->next = 0;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if( !(seq->first) )
    {
        seq->first = block;
        block->prev = block->next = block;
    }
    else
    {
        block->prev = seq->first->prev;
        block->next = seq->first;
        block->prev->next = block->next->prev = block;
    }

    /* for free blocks the <count> field means total number of bytes in the block.
       And for used blocks it means a current number of sequence
       elements in the block */
    assert( block->count % seq->elem_size == 0 && block->count > 0 );

    if( !in_front_of )
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 :
            block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if( block != block->prev )
        {
            assert( seq->first->start_index == 0 );
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for( ;; )
        {
            block->start_index += delta;
            block = block->next;
            if( block == seq->first )
                break;
        }
    }

    block->count = 0;

    __END__;
}

/* recycles a sequence block for the further use */
static void
icvFreeSeqBlock( CvSeq *seq, int in_front_of )
{
    /*CV_FUNCNAME( "icvFreeSeqBlock" );*/

    __BEGIN__;

    CvSeqBlock *block = seq->first;

    assert( (in_front_of ? block : block->prev)->count == 0 );

    if( block == block->prev )  /* single block case */
    {
        block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
        block->data = seq->block_max - block->count;
        seq->first = 0;
        seq->ptr = seq->block_max = 0;
        seq->total = 0;
    }
    else
    {
        if( !in_front_of )
        {
            block = block->prev;
            assert( seq->ptr == block->data );

            block->count = (int)(seq->block_max - seq->ptr);
            seq->block_max = seq->ptr = block->prev->data +
                block->prev->count * seq->elem_size;
        }
        else
        {
            int delta = block->start_index;

            block->count = delta * seq->elem_size;
            block->data -= block->count;

            /* update start indices of sequence blocks */
            for( ;; )
            {
                block->start_index -= delta;
                block = block->next;
                if( block == seq->first )
                    break;
            }

            seq->first = block->next;
        }

        block->prev->next = block->next;
        block->next->prev = block->prev;
    }

    assert( block->count > 0 && block->count % seq->elem_size == 0 );
    block->next = seq->free_blocks;
    seq->free_blocks = block;

    __END__;
}


/****************************************************************************************\
*                             Sequence Writer implementation                             *
\****************************************************************************************/

/* initializes sequence writer */
CV_IMPL void
cvStartAppendToSeq( CvSeq *seq, CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvStartAppendToSeq" );

    __BEGIN__;

    if( !seq || !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( writer, 0, sizeof( *writer ));
    writer->header_size = sizeof( CvSeqWriter );

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}


/* initializes sequence writer */
CV_IMPL void
cvStartWriteSeq( int seq_flags, int header_size,
                 int elem_size, CvMemStorage * storage, CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvStartWriteSeq" );

    __BEGIN__;

    if( !storage || !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( seq = cvCreateSeq( seq_flags, header_size, elem_size, storage ));
    cvStartAppendToSeq( seq, writer );

    __END__;
}


/* updates sequence header */
CV_IMPL void
cvFlushSeqWriter( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvFlushSeqWriter" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;
    seq->ptr = writer->ptr;

    if( writer->block )
    {
        int total = 0;
        CvSeqBlock *first_block = writer->seq->first;
        CvSeqBlock *block = first_block;

        writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
        assert( writer->block->count > 0 );

        do
        {
            total += block->count;
            block = block->next;
        }
        while( block != first_block );

        writer->seq->total = total;
    }

    __END__;
}


/* calls icvFlushSeqWriter and finishes writing process */
CV_IMPL CvSeq *
cvEndWriteSeq( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvEndWriteSeq" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( cvFlushSeqWriter( writer ));
    seq = writer->seq;

    /* truncate the last block */
    if( writer->block && writer->seq->storage )
    {
        CvMemStorage *storage = seq->storage;
        char *storage_block_max = (char *) storage->top + storage->block_size;

        assert( writer->block->count > 0 );

        if( (unsigned)((storage_block_max - storage->free_space)
            - seq->block_max) < CV_STRUCT_ALIGN )
        {
            storage->free_space = cvAlignLeft((int)(storage_block_max - seq->ptr), CV_STRUCT_ALIGN);
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = 0;

    __END__;

    return seq;
}


/* creates new sequence block */
CV_IMPL void
cvCreateSeqBlock( CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvCreateSeqBlock" );

    __BEGIN__;

    CvSeq *seq;

    if( !writer || !writer->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;

    cvFlushSeqWriter( writer );

    CV_CALL( icvGrowSeq( seq, 0 ));

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}


/****************************************************************************************\
*                               Sequence Reader implementation                           *
\****************************************************************************************/

/* initializes sequence reader */
CV_IMPL void
cvStartReadSeq( const CvSeq *seq, CvSeqReader * reader, int reverse )
{
    CvSeqBlock *first_block;
    CvSeqBlock *last_block;

    CV_FUNCNAME( "cvStartReadSeq" );

    if( reader )
    {
        reader->seq = 0;
        reader->block = 0;
        reader->ptr = reader->block_max = reader->block_min = 0;
    }

    __BEGIN__;

    if( !seq || !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    reader->header_size = sizeof( CvSeqReader );
    reader->seq = (CvSeq*)seq;

    first_block = seq->first;

    if( first_block )
    {
        last_block = first_block->prev;
        reader->ptr = first_block->data;
        reader->prev_elem = CV_GET_LAST_ELEM( seq, last_block );
        reader->delta_index = seq->first->start_index;

        if( reverse )
        {
            char *temp = reader->ptr;

            reader->ptr = reader->prev_elem;
            reader->prev_elem = temp;

            reader->block = last_block;
        }
        else
        {
            reader->block = first_block;
        }

        reader->block_min = reader->block->data;
        reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
    }
    else
    {
        reader->delta_index = 0;
        reader->block = 0;

        reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
    }

    __END__;
}


/* changes the current reading block to the previous or to the next */
CV_IMPL void
cvChangeSeqBlock( void* _reader, int direction )
{
    CV_FUNCNAME( "cvChangeSeqBlock" );

    __BEGIN__;

    CvSeqReader* reader = (CvSeqReader*)_reader;
    
    if( !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    if( direction > 0 )
    {
        reader->block = reader->block->next;
        reader->ptr = reader->block->data;
    }
    else
    {
        reader->block = reader->block->prev;
        reader->ptr = CV_GET_LAST_ELEM( reader->seq, reader->block );
    }
    reader->block_min = reader->block->data;
    reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;

    __END__;
}


/* returns the current reader position */
CV_IMPL int
cvGetSeqReaderPos( CvSeqReader* reader )
{
    int elem_size;
    int index = -1;

    CV_FUNCNAME( "cvGetSeqReaderPos" );

    __BEGIN__;

    if( !reader || !reader->ptr )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = reader->seq->elem_size;
    if( elem_size <= ICV_SHIFT_TAB_MAX && (index = icvPower2ShiftTab[elem_size - 1]) >= 0 )
        index = (int)((reader->ptr - reader->block_min) >> index);
    else
        index = (int)((reader->ptr - reader->block_min) / elem_size);

    index += reader->block->start_index - reader->delta_index;

    __END__;

    return index;
}


/* sets reader position to given absolute or relative
   (relatively to the current one) position */
CV_IMPL void
cvSetSeqReaderPos( CvSeqReader* reader, int index, int is_relative )
{
    CV_FUNCNAME( "cvSetSeqReaderPos" );

    __BEGIN__;

    CvSeqBlock *block;
    int elem_size, count, total;

    if( !reader || !reader->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if( !is_relative )
    {
        if( index < 0 )
        {
            if( index < -total )
                CV_ERROR( CV_StsOutOfRange, "" );
            index += total;
        }
        else if( index >= total )
        {
            index -= total;
            if( index >= total )
                CV_ERROR( CV_StsOutOfRange, "" );
        }

        block = reader->seq->first;
        if( index >= (count = block->count) )
        {
            if( index + index <= total )
            {
                do
                {
                    block = block->next;
                    index -= count;
                }
                while( index >= (count = block->count) );
            }
            else
            {
                do
                {
                    block = block->prev;
                    total -= block->count;
                }
                while( index < total );
                index -= total;
            }
        }
        reader->ptr = block->data + index * elem_size;
        if( reader->block != block )
        {
            reader->block = block;
            reader->block_min = block->data;
            reader->block_max = block->data + block->count * elem_size;
        }
    }
    else
    {
        char* ptr = reader->ptr;
        index *= elem_size;
        block = reader->block;

        if( index > 0 )
        {
            while( ptr + index >= reader->block_max )
            {
                int delta = (int)(reader->block_max - ptr);
                index -= delta;
                reader->block = block = block->next;
                reader->block_min = ptr = block->data;
                reader->block_max = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
        else
        {
            while( ptr + index < reader->block_min )
            {
                int delta = (int)(ptr - reader->block_min);
                index += delta;
                reader->block = block = block->prev;
                reader->block_min = block->data;
                reader->block_max = ptr = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
    }

    __END__;
}


/* pushes element to the sequence */
CV_IMPL char*
cvSeqPush( CvSeq *seq, void *element )
{
    char *ptr = 0;
    size_t elem_size;

    CV_FUNCNAME( "cvSeqPush" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if( ptr >= seq->block_max )
    {
        CV_CALL( icvGrowSeq( seq, 0 ));

        ptr = seq->ptr;
        assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */  );
    }

    if( element )
        CV_MEMCPY_AUTO( ptr, element, elem_size );
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    __END__;

    return ptr;
}


/* pops the last element out of the sequence */
CV_IMPL void
cvSeqPop( CvSeq *seq, void *element )
{
    char *ptr;
    int elem_size;

    CV_FUNCNAME( "cvSeqPop" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    seq->ptr = ptr = seq->ptr - elem_size;

    if( element )
        CV_MEMCPY_AUTO( element, ptr, elem_size );
    seq->ptr = ptr;
    seq->total--;

    if( --(seq->first->prev->count) == 0 )
    {
        icvFreeSeqBlock( seq, 0 );
        assert( seq->ptr == seq->block_max );
    }

    __END__;
}


/* pushes element to the front of the sequence */
CV_IMPL char*
cvSeqPushFront( CvSeq *seq, void *element )
{
    char* ptr = 0;
    int elem_size;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqPushFront" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( !block || block->start_index == 0 )
    {
        CV_CALL( icvGrowSeq( seq, 1 ));

        block = seq->first;
        assert( block->start_index > 0 );
    }

    ptr = block->data -= elem_size;

    if( element )
        CV_MEMCPY_AUTO( ptr, element, elem_size );
    block->count++;
    block->start_index--;
    seq->total++;

    __END__;

    return ptr;
}


/* pulls out the first element of the sequence */
CV_IMPL void
cvSeqPopFront( CvSeq *seq, void *element )
{
    int elem_size;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqPopFront" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( element )
        CV_MEMCPY_AUTO( element, block->data, elem_size );
    block->data += elem_size;
    block->start_index++;
    seq->total--;

    if( --(block->count) == 0 )
    {
        icvFreeSeqBlock( seq, 1 );
    }

    __END__;
}

/* inserts new element in the middle of the sequence */
CV_IMPL char*
cvSeqInsert( CvSeq *seq, int before_index, void *element )
{
    int elem_size;
    int block_size;
    CvSeqBlock *block;
    int delta_index;
    int total;
    char* ret_ptr = 0;

    CV_FUNCNAME( "cvSeqInsert" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    total = seq->total;
    before_index += before_index < 0 ? total : 0;
    before_index -= before_index > total ? total : 0;

    if( (unsigned)before_index > (unsigned)total )
        CV_ERROR( CV_StsOutOfRange, "" );

    if( before_index == total )
    {
        CV_CALL( ret_ptr = cvSeqPush( seq, element ));
    }
    else if( before_index == 0 )
    {
        CV_CALL( ret_ptr = cvSeqPushFront( seq, element ));
    }
    else
    {
        elem_size = seq->elem_size;

        if( before_index >= total >> 1 )
        {
            char *ptr = seq->ptr + elem_size;

            if( ptr > seq->block_max )
            {
                CV_CALL( icvGrowSeq( seq, 0 ));

                ptr = seq->ptr + elem_size;
                assert( ptr <= seq->block_max );
            }

            delta_index = seq->first->start_index;
            block = seq->first->prev;
            block->count++;
            block_size = (int)(ptr - block->data);

            while( before_index < block->start_index - delta_index )
            {
                CvSeqBlock *prev_block = block->prev;

                memmove( block->data + elem_size, block->data, block_size - elem_size );
                block_size = prev_block->count * elem_size;
                memcpy( block->data, prev_block->data + block_size - elem_size, elem_size );
                block = prev_block;

                /* check that we don't fall in the infinite loop */
                assert( block != seq->first->prev );
            }

            before_index = (before_index - block->start_index + delta_index) * elem_size;
            memmove( block->data + before_index + elem_size, block->data + before_index,
                     block_size - before_index - elem_size );

            ret_ptr = block->data + before_index;

            if( element )
                memcpy( ret_ptr, element, elem_size );
            seq->ptr = ptr;
        }
        else
        {
            block = seq->first;

            if( block->start_index == 0 )
            {
                CV_CALL( icvGrowSeq( seq, 1 ));

                block = seq->first;
            }

            delta_index = block->start_index;
            block->count++;
            block->start_index--;
            block->data -= elem_size;

            while( before_index > block->start_index - delta_index + block->count )
            {
                CvSeqBlock *next_block = block->next;

                block_size = block->count * elem_size;
                memmove( block->data, block->data + elem_size, block_size - elem_size );
                memcpy( block->data + block_size - elem_size, next_block->data, elem_size );
                block = next_block;
                /* check that we don't fall in the infinite loop */
                assert( block != seq->first );
            }

            before_index = (before_index - block->start_index + delta_index) * elem_size;
            memmove( block->data, block->data + elem_size, before_index - elem_size );

            ret_ptr = block->data + before_index - elem_size;

            if( element )
                memcpy( ret_ptr, element, elem_size );
        }

        seq->total = total + 1;
    }

    __END__;

    return ret_ptr;
}


/* removes element from the sequence */
CV_IMPL void
cvSeqRemove( CvSeq *seq, int index )
{
    char *ptr;
    int elem_size;
    int block_size;
    CvSeqBlock *block;
    int delta_index;
    int total, front = 0;

    CV_FUNCNAME( "cvSeqRemove" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    total = seq->total;

    index += index < 0 ? total : 0;
    index -= index >= total ? total : 0;

    if( (unsigned) index >= (unsigned) total )
        CV_ERROR( CV_StsOutOfRange, "Invalid index" );

    if( index == total - 1 )
    {
        cvSeqPop( seq, 0 );
    }
    else if( index == 0 )
    {
        cvSeqPopFront( seq, 0 );
    }
    else
    {
        block = seq->first;
        elem_size = seq->elem_size;
        delta_index = block->start_index;
        while( block->start_index - delta_index + block->count <= index )
            block = block->next;

        ptr = block->data + (index - block->start_index + delta_index) * elem_size;

        front = index < total >> 1;
        if( !front )
        {
            block_size = block->count * elem_size - (int)(ptr - block->data);

            while( block != seq->first->prev )  /* while not the last block */
            {
                CvSeqBlock *next_block = block->next;

                memmove( ptr, ptr + elem_size, block_size - elem_size );
                memcpy( ptr + block_size - elem_size, next_block->data, elem_size );
                block = next_block;
                ptr = block->data;
                block_size = block->count * elem_size;
            }

            memmove( ptr, ptr + elem_size, block_size - elem_size );
            seq->ptr -= elem_size;
        }
        else
        {
            ptr += elem_size;
            block_size = (int)(ptr - block->data);

            while( block != seq->first )
            {
                CvSeqBlock *prev_block = block->prev;

                memmove( block->data + elem_size, block->data, block_size - elem_size );
                block_size = prev_block->count * elem_size;
                memcpy( block->data, prev_block->data + block_size - elem_size, elem_size );
                block = prev_block;
            }

            memmove( block->data + elem_size, block->data, block_size - elem_size );
            block->data += elem_size;
            block->start_index++;
        }

        seq->total = total - 1;
        if( --block->count == 0 )
            icvFreeSeqBlock( seq, front );
    }

    __END__;
}


/* adds several elements to the end or in the beginning of sequence */
CV_IMPL void
cvSeqPushMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    CV_FUNCNAME( "cvSeqPushMulti" );

    __BEGIN__;
    int elem_size;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_ERROR( CV_StsBadSize, "number of removed elements is negative" );

    elem_size = seq->elem_size;

    if( !front )
    {
        while( count > 0 )
        {
            int delta = (int)((seq->block_max - seq->ptr) / elem_size);

            delta = MIN( delta, count );
            if( delta > 0 )
            {
                seq->first->prev->count += delta;
                seq->total += delta;
                count -= delta;
                delta *= elem_size;
                if( elements )
                {
                    memcpy( seq->ptr, elements, delta );
                    elements += delta;
                }
                seq->ptr += delta;
            }

            if( count > 0 )
                CV_CALL( icvGrowSeq( seq, 0 ));
        }
    }
    else
    {
        CvSeqBlock* block = seq->first;
        
        while( count > 0 )
        {
            int delta;
            
            if( !block || block->start_index == 0 )
            {
                CV_CALL( icvGrowSeq( seq, 1 ));

                block = seq->first;
                assert( block->start_index > 0 );
            }

            delta = MIN( block->start_index, count );
            count -= delta;
            block->start_index -= delta;
            block->count += delta;
            seq->total += delta;
            delta *= elem_size;
            block->data -= delta;

            if( elements )
                memcpy( block->data, elements + count*elem_size, delta );
        }
    }

    __END__;
}


/* removes several elements from the end of sequence */
CV_IMPL void
cvSeqPopMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    CV_FUNCNAME( "cvSeqPopMulti" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_ERROR( CV_StsBadSize, "number of removed elements is negative" );

    count = MIN( count, seq->total );

    if( !front )
    {
        if( elements )
            elements += count * seq->elem_size;

        while( count > 0 )
        {
            int delta = seq->first->prev->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->prev->count -= delta;
            seq->total -= delta;
            count -= delta;
            delta *= seq->elem_size;
            seq->ptr -= delta;

            if( elements )
            {
                elements -= delta;
                memcpy( elements, seq->ptr, delta );
            }

            if( seq->first->prev->count == 0 )
                icvFreeSeqBlock( seq, 0 );
        }
    }
    else
    {
        while( count > 0 )
        {
            int delta = seq->first->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->count -= delta;
            seq->total -= delta;
            count -= delta;
            seq->first->start_index += delta;
            delta *= seq->elem_size;

            if( elements )
            {
                memcpy( elements, seq->first->data, delta );
                elements += delta;
            }

            seq->first->data += delta;
            if( seq->first->count == 0 )
                icvFreeSeqBlock( seq, 1 );
        }
    }

    __END__;
}


/* removes all elements from the sequence */
CV_IMPL void
cvClearSeq( CvSeq *seq )
{
    CV_FUNCNAME( "cvClearSeq" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    cvSeqPopMulti( seq, 0, seq->total );

    __END__;
}



typedef struct CvSeqReaderPos
{
    CvSeqBlock* block;
    char* ptr;
    char* block_min;
    char* block_max;
}
CvSeqReaderPos;

#define CV_SAVE_READER_POS( reader, pos )   \
{                                           \
    (pos).block = (reader).block;           \
    (pos).ptr = (reader).ptr;               \
    (pos).block_min = (reader).block_min;   \
    (pos).block_max = (reader).block_max;   \
}

#define CV_RESTORE_READER_POS( reader, pos )\
{                                           \
    (reader).block = (pos).block;           \
    (reader).ptr = (pos).ptr;               \
    (reader).block_min = (pos).block_min;   \
    (reader).block_max = (pos).block_max;   \
}

inline char*
icvMed3( char* a, char* b, char* c, CvCmpFunc cmp_func, void* aux )
{
    return cmp_func(a, b, aux) < 0 ?
      (cmp_func(b, c, aux) < 0 ? b : cmp_func(a, c, aux) < 0 ? c : a)
     :(cmp_func(b, c, aux) > 0 ? b : cmp_func(a, c, aux) < 0 ? a : c);
}

CV_IMPL void
cvSeqSort( CvSeq* seq, CvCmpFunc cmp_func, void* aux )
{         
    int elem_size;
    int isort_thresh = 7;
    CvSeqReader left, right;
    int sp = 0;

    struct
    {
        CvSeqReaderPos lb;
        CvSeqReaderPos ub;
    }
    stack[48];

    CV_FUNCNAME( "cvSeqSort" );

    __BEGIN__;
    
    if( !CV_IS_SEQ(seq) )
        CV_ERROR( !seq ? CV_StsNullPtr : CV_StsBadArg, "Bad input sequence" );

    if( !cmp_func )
        CV_ERROR( CV_StsNullPtr, "Null compare function" );

    if( seq->total <= 1 )
        EXIT;

    elem_size = seq->elem_size;
    isort_thresh *= elem_size;

    cvStartReadSeq( seq, &left, 0 );
    right = left;
    CV_SAVE_READER_POS( left, stack[0].lb );
    CV_PREV_SEQ_ELEM( elem_size, right );
    CV_SAVE_READER_POS( right, stack[0].ub );

    while( sp >= 0 )
    {
        CV_RESTORE_READER_POS( left, stack[sp].lb );
        CV_RESTORE_READER_POS( right, stack[sp].ub );
        sp--;

        for(;;)
        {
            int i, n, m;
            CvSeqReader ptr, ptr2;

            if( left.block == right.block )
                n = (int)(right.ptr - left.ptr) + elem_size;
            else
            {
                n = cvGetSeqReaderPos( &right );
                n = (n - cvGetSeqReaderPos( &left ) + 1)*elem_size;
            }

            if( n <= isort_thresh )
            {
            insert_sort:
                ptr = ptr2 = left;
                CV_NEXT_SEQ_ELEM( elem_size, ptr );
                CV_NEXT_SEQ_ELEM( elem_size, right );
                while( ptr.ptr != right.ptr )
                {
                    ptr2.ptr = ptr.ptr;
                    if( ptr2.block != ptr.block )
                    {
                        ptr2.block = ptr.block;
                        ptr2.block_min = ptr.block_min;
                        ptr2.block_max = ptr.block_max;
                    }
                    while( ptr2.ptr != left.ptr )
                    {
                        char* cur = ptr2.ptr;
                        CV_PREV_SEQ_ELEM( elem_size, ptr2 );
                        if( cmp_func( ptr2.ptr, cur, aux ) <= 0 )
                            break;
                        CV_SWAP_ELEMS( ptr2.ptr, cur, elem_size );
                    }
                    CV_NEXT_SEQ_ELEM( elem_size, ptr );
                }
                break;
            }
            else
            {
                CvSeqReader left0, left1, right0, right1;
                CvSeqReader tmp0, tmp1;
                char *m1, *m2, *m3, *pivot;
                int swap_cnt = 0;
                int l, l0, l1, r, r0, r1;

                left0 = tmp0 = left;
                right0 = right1 = right;
                n /= elem_size;
                
                if( n > 40 )
                {
                    int d = n / 8;
                    char *p1, *p2, *p3;
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m1 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, (n/2) - d*3, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m2 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, n - 1 - d*3 - n/2, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m3 = icvMed3( p1, p2, p3, cmp_func, aux );
                }
                else
                {
                    m1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n/2, 1 );
                    m2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n - 1 - n/2, 1 );
                    m3 = tmp0.ptr;
                }

                pivot = icvMed3( m1, m2, m3, cmp_func, aux );
                left = left0;
                if( pivot != left.ptr )
                {
                    CV_SWAP_ELEMS( pivot, left.ptr, elem_size );
                    pivot = left.ptr;
                }
                CV_NEXT_SEQ_ELEM( elem_size, left );
                left1 = left;

                for(;;)
                {
                    while( left.ptr != right.ptr && (r = cmp_func(left.ptr, pivot, aux)) <= 0 )
                    {
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        CV_NEXT_SEQ_ELEM( elem_size, left );
                    }

                    while( left.ptr != right.ptr && (r = cmp_func(right.ptr,pivot, aux)) >= 0 )
                    {
                        if( r == 0 )
                        {
                            if( right1.ptr != right.ptr )
                                CV_SWAP_ELEMS( right1.ptr, right.ptr, elem_size );
                            swap_cnt = 1;
                            CV_PREV_SEQ_ELEM( elem_size, right1 );
                        }
                        CV_PREV_SEQ_ELEM( elem_size, right );
                    }

                    if( left.ptr == right.ptr )
                    {
                        r = cmp_func(left.ptr, pivot, aux);
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        if( r <= 0 )
                        {
                            CV_NEXT_SEQ_ELEM( elem_size, left );
                        }
                        else
                        {
                            CV_PREV_SEQ_ELEM( elem_size, right );
                        }
                        break;
                    }

                    CV_SWAP_ELEMS( left.ptr, right.ptr, elem_size );
                    CV_NEXT_SEQ_ELEM( elem_size, left );
                    r = left.ptr == right.ptr;
                    CV_PREV_SEQ_ELEM( elem_size, right );
                    swap_cnt = 1;
                    if( r )
                        break;
                }

                if( swap_cnt == 0 )
                {
                    left = left0, right = right0;
                    goto insert_sort;
                }

                l = cvGetSeqReaderPos( &left );
                if( l == 0 )
                    l = seq->total;
                l0 = cvGetSeqReaderPos( &left0 );
                l1 = cvGetSeqReaderPos( &left1 );
                if( l1 == 0 )
                    l1 = seq->total;
                
                n = MIN( l - l1, l1 - l0 );
                if( n > 0 )
                {
                    tmp0 = left0;
                    tmp1 = left;
                    cvSetSeqReaderPos( &tmp1, 0-n, 1 );
                    for( i = 0; i < n; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                r = cvGetSeqReaderPos( &right );
                r0 = cvGetSeqReaderPos( &right0 );
                r1 = cvGetSeqReaderPos( &right1 );
                m = MIN( r0 - r1, r1 - r );
                if( m > 0 )
                {
                    tmp0 = left;
                    tmp1 = right0;
                    cvSetSeqReaderPos( &tmp1, 1-m, 1 );
                    for( i = 0; i < m; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                n = l - l1;
                m = r1 - r;
                if( n > 1 )
                {
                    if( m > 1 )
                    {
                        if( n > m )
                        {
                            sp++;
                            CV_SAVE_READER_POS( left0, stack[sp].lb );
                            cvSetSeqReaderPos( &left0, n - 1, 1 );
                            CV_SAVE_READER_POS( left0, stack[sp].ub );
                            left = right = right0;
                            cvSetSeqReaderPos( &left, 1 - m, 1 );
                        }
                        else
                        {
                            sp++;
                            CV_SAVE_READER_POS( right0, stack[sp].ub );
                            cvSetSeqReaderPos( &right0, 1 - m, 1 );
                            CV_SAVE_READER_POS( right0, stack[sp].lb );
                            left = right = left0;
                            cvSetSeqReaderPos( &right, n - 1, 1 );
                        }
                    }
                    else
                    {
                        left = right = left0;
                        cvSetSeqReaderPos( &right, n - 1, 1 );
                    }
                }
                else if( m > 1 )
                {
                    left = right = right0;
                    cvSetSeqReaderPos( &left, 1 - m, 1 );
                }
                else
                    break;
            }
        }
    }

    __END__;
}


CV_IMPL char*
cvSeqSearch( CvSeq* seq, const void* _elem, CvCmpFunc cmp_func,
             int is_sorted, int* _idx, void* userdata )
{
    char* result = 0;
    const char* elem = (const char*)_elem;
    int idx = -1;
    
    CV_FUNCNAME("cvSeqSort");

    __BEGIN__;

    int elem_size, i, j, total;

    if( !CV_IS_SEQ(seq) )
        CV_ERROR( !seq ? CV_StsNullPtr : CV_StsBadArg, "Bad input sequence" );

    if( !elem )
        CV_ERROR( CV_StsNullPtr, "Null element pointer" );

    elem_size = seq->elem_size;
    total = seq->total;

    if( total == 0 )
        EXIT;

    if( !is_sorted )
    {
        CvSeqReader reader;
        cvStartReadSeq( seq, &reader, 0 );

        if( cmp_func )
        {
            for( i = 0; i < total; i++ )
            {
                if( cmp_func( elem, reader.ptr, userdata ) == 0 )
                    break;
                CV_NEXT_SEQ_ELEM( elem_size, reader );
            }
        }
        else if( (elem_size & (sizeof(int)-1)) == 0 )
        {
            for( i = 0; i < total; i++ )
            {
                for( j = 0; j < elem_size; j += sizeof(int) )
                {
                    if( *(const int*)(reader.ptr + j) != *(const int*)(elem + j) )
                        break;
                }
                if( j == elem_size )
                    break;
                CV_NEXT_SEQ_ELEM( elem_size, reader );
            }
        }
        else
        {
            for( i = 0; i < total; i++ )
            {
                for( j = 0; j < elem_size; j++ )
                {
                    if( reader.ptr[j] != elem[j] )
                        break;
                }
                if( j == elem_size )
                    break;
                CV_NEXT_SEQ_ELEM( elem_size, reader );
            }
        }

        idx = i;
        if( i < total )
            result = reader.ptr;
    }
    else
    {
        if( !cmp_func )
            CV_ERROR( CV_StsNullPtr, "Null compare function" );

        i = 0, j = total;
        
        while( j > i )
        {
            int k = (i+j)>>1, code;
            char* ptr = cvGetSeqElem( seq, k );
            code = cmp_func( elem, ptr, userdata );
            if( !code )
            {
                result = ptr;
                idx = k;
                EXIT;
            }
            if( code < 0 )
                j = k;
            else
                i = k+1;
        }
        idx = j;
    }

    __END__;

    if( _idx )
        *_idx = idx;

    return result;
}


CV_IMPL void
cvSeqInvert( CvSeq* seq )
{
    CV_FUNCNAME( "cvSeqInvert" );

    __BEGIN__;

    CvSeqReader left_reader, right_reader;
    int elem_size;
    int i, count;

    CV_CALL( cvStartReadSeq( seq, &left_reader, 0 ));
    CV_CALL( cvStartReadSeq( seq, &right_reader, 1 ));
    elem_size = seq->elem_size;
    count = seq->total >> 1;

    for( i = 0; i < count; i++ )
    {
        CV_SWAP_ELEMS( left_reader.ptr, right_reader.ptr, elem_size );
        CV_NEXT_SEQ_ELEM( elem_size, left_reader );
        CV_PREV_SEQ_ELEM( elem_size, right_reader );
    }

    __END__;
}

/****************************************************************************************\
*                                      Set implementation                                *
\****************************************************************************************/

/* creates empty set */
CV_IMPL CvSet*
cvCreateSet( int set_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    CvSet *set = 0;

    CV_FUNCNAME( "cvCreateSet" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSet ) ||
        elem_size < (int)sizeof(void*)*2 ||
        (elem_size & (sizeof(void*)-1)) != 0 )
        CV_ERROR( CV_StsBadSize, "" );

    set = (CvSet*) cvCreateSeq( set_flags, header_size, elem_size, storage );
    set->flags = (set->flags & ~CV_MAGIC_MASK) | CV_SET_MAGIC_VAL;

    __END__;

    return set;
}


/* adds new element to the set */
CV_IMPL int
cvSetAdd( CvSet* set, CvSetElem* element, CvSetElem** inserted_element )
{
    int id = -1;

    CV_FUNCNAME( "cvSetAdd" );

    __BEGIN__;

    CvSetElem *free_elem;

    if( !set )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !(set->free_elems) )
    {
        int count = set->total;
        int elem_size = set->elem_size;
        char *ptr;
        CV_CALL( icvGrowSeq( (CvSeq *) set, 0 ));

        set->free_elems = (CvSetElem*) (ptr = set->ptr);
        for( ; ptr + elem_size <= set->block_max; ptr += elem_size, count++ )
        {
            ((CvSetElem*)ptr)->flags = count | CV_SET_ELEM_FREE_FLAG;
            ((CvSetElem*)ptr)->next_free = (CvSetElem*)(ptr + elem_size);
        }
        assert( count <= CV_SET_ELEM_IDX_MASK+1 );
        ((CvSetElem*)(ptr - elem_size))->next_free = 0;
        set->first->prev->count += count - set->total;
        set->total = count;
        set->ptr = set->block_max;
    }

    free_elem = set->free_elems;
    set->free_elems = free_elem->next_free;

    id = free_elem->flags & CV_SET_ELEM_IDX_MASK;
    if( element )
        CV_MEMCPY_INT( free_elem, element, (size_t)set->elem_size/sizeof(int) );

    free_elem->flags = id;
    set->active_count++;

    if( inserted_element )
        *inserted_element = free_elem;

    __END__;

    return id;
}


/* removes element from the set given its index */
CV_IMPL void
cvSetRemove( CvSet* set, int index )
{
    CV_FUNCNAME( "cvSetRemove" );

    __BEGIN__;

    CvSetElem* elem = cvGetSetElem( set, index );
    if( elem )
        cvSetRemoveByPtr( set, elem );
    else if( !set )
        CV_ERROR( CV_StsNullPtr, "" );

    __END__;
}


/* removes all elements from the set */
CV_IMPL void
cvClearSet( CvSet* set )
{
    CV_FUNCNAME( "cvClearSet" );

    __BEGIN__;

    CV_CALL( cvClearSeq( (CvSeq*)set ));
    set->free_elems = 0;
    set->active_count = 0;

    __END__;
}


/****************************************************************************************\
*                                 Working with sequence tree                             *
\****************************************************************************************/

typedef struct CvTreeNode
{
    int       flags;         /* micsellaneous flags */         
    int       header_size;   /* size of sequence header */     
    struct    CvTreeNode* h_prev; /* previous sequence */      
    struct    CvTreeNode* h_next; /* next sequence */          
    struct    CvTreeNode* v_prev; /* 2nd previous sequence */  
    struct    CvTreeNode* v_next; /* 2nd next sequence */
}
CvTreeNode;



// Insert contour into tree given certain parent sequence.
// If parent is equal to frame (the most external contour),
// then added contour will have null pointer to parent.
CV_IMPL void
cvInsertNodeIntoTree( void* _node, void* _parent, void* _frame )
{
    CV_FUNCNAME( "cvInsertNodeIntoTree" );

    __BEGIN__;

    CvTreeNode* node = (CvTreeNode*)_node;
    CvTreeNode* parent = (CvTreeNode*)_parent;

    if( !node || !parent )
        CV_ERROR( CV_StsNullPtr, "" );

    node->v_prev = _parent != _frame ? parent : 0;
    node->h_next = parent->v_next;

    assert( parent->v_next != node );

    if( parent->v_next )
        parent->v_next->h_prev = node;
    parent->v_next = node;

    __END__;
}


/* End of file. */

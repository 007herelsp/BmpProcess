
#include "core.precomp.hpp"

#define ICV_FREE_PTR(storage)  \
    ((schar*)(storage)->top + (storage)->block_size - (storage)->free_space)

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
static const schar icvPower2ShiftTab[] =
{
    0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
};

/****************************************************************************************\
*            Functions for manipulating memory storage - list of memory blocks           *
\****************************************************************************************/

/* Initialize allocated storage: */
static void
icvInitMemStorage( CvMemStorage* storage, int block_size )
{
    if( !storage )
        CV_Error( CV_StsNullPtr, "" );

    if( block_size <= 0 )
        block_size = CV_STORAGE_BLOCK_SIZE;

    block_size = cvAlign( block_size, CV_STRUCT_ALIGN );
    assert( sizeof(CvMemBlock) % CV_STRUCT_ALIGN == 0 );

    memset( storage, 0, sizeof( *storage ));
    storage->signature = CV_STORAGE_MAGIC_VAL;
    storage->block_size = block_size;
}


/* Create root memory storage: */
CV_IMPL CvMemStorage*
cvCreateMemStorage( int block_size )
{
    CvMemStorage* storage = (CvMemStorage *)cvAlloc( sizeof( CvMemStorage ));
    icvInitMemStorage( storage, block_size );
    return storage;
}


/* Create child memory storage: */
CV_IMPL CvMemStorage *
cvCreateChildMemStorage( CvMemStorage * parent )
{
    if( !parent )
        CV_Error( CV_StsNullPtr, "" );

    CvMemStorage* storage = cvCreateMemStorage(parent->block_size);
    storage->parent = parent;

    return storage;
}


/* Release all blocks of the storage (or return them to parent, if any): */
static void
icvDestroyMemStorage( CvMemStorage* storage )
{
    int k = 0;

    CvMemBlock *block;
    CvMemBlock *dst_top = 0;

    if( !storage )
        CV_Error( CV_StsNullPtr, "" );

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
}


/* Release memory storage: */
CV_IMPL void
cvReleaseMemStorage( CvMemStorage** storage )
{
    if( !storage )
        CV_Error( CV_StsNullPtr, "" );

    CvMemStorage* st = *storage;
    *storage = 0;
    if( st )
    {
        icvDestroyMemStorage( st );
        cvFree( &st );
    }
}


/* Clears memory storage (return blocks to the parent, if any): */
CV_IMPL void
cvClearMemStorage( CvMemStorage * storage )
{
    if( !storage )
        CV_Error( CV_StsNullPtr, "" );

    if( storage->parent )
        icvDestroyMemStorage( storage );
    else
    {
        storage->top = storage->bottom;
        storage->free_space = storage->bottom ? storage->block_size - sizeof(CvMemBlock) : 0;
    }
}


/* Moves stack pointer to next block.
   If no blocks, allocate new one and link it to the storage: */
static void
icvGoNextMemBlock( CvMemStorage * storage )
{
    if( !storage )
        CV_Error( CV_StsNullPtr, "" );

    if( !storage->top || !storage->top->next )
    {
        CvMemBlock *block;

        if( !(storage->parent) )
        {
            block = (CvMemBlock *)cvAlloc( storage->block_size );
        }
        else
        {
            CvMemStorage *parent = storage->parent;
            CvMemStoragePos parent_pos;

            cvSaveMemStoragePos( parent, &parent_pos );
            icvGoNextMemBlock( parent );

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
}


/* Remember memory storage position: */
CV_IMPL void
cvSaveMemStoragePos( const CvMemStorage * storage, CvMemStoragePos * pos )
{
    if( !storage || !pos )
        CV_Error( CV_StsNullPtr, "" );

    pos->top = storage->top;
    pos->free_space = storage->free_space;
}


/* Restore memory storage position: */
CV_IMPL void
cvRestoreMemStoragePos( CvMemStorage * storage, CvMemStoragePos * pos )
{
    if( !storage || !pos )
        CV_Error( CV_StsNullPtr, "" );
    if( pos->free_space > storage->block_size )
        CV_Error( CV_StsBadSize, "" );

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
}


/* Allocate continuous buffer of the specified size in the storage: */
CV_IMPL void*
cvMemStorageAlloc( CvMemStorage* storage, size_t size )
{
    schar *ptr = 0;
    if( !storage )
        CV_Error( CV_StsNullPtr, "NULL storage pointer" );

    if( size > INT_MAX )
        CV_Error( CV_StsOutOfRange, "Too large memory block is requested" );

    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    if( (size_t)storage->free_space < size )
    {
        size_t max_free_space = cvAlignLeft(storage->block_size - sizeof(CvMemBlock), CV_STRUCT_ALIGN);
        if( max_free_space < size )
            CV_Error( CV_StsOutOfRange, "requested size is negative or too big" );

        icvGoNextMemBlock( storage );
    }

    ptr = ICV_FREE_PTR(storage);
    assert( (size_t)ptr % CV_STRUCT_ALIGN == 0 );
    storage->free_space = cvAlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN );

    return ptr;
}




/****************************************************************************************\
*                               Sequence implementation                                  *
\****************************************************************************************/

/* Create empty sequence: */
CV_IMPL CvSeq *
cvCreateSeq( int seq_flags, size_t header_size, size_t elem_size, CvMemStorage* storage )
{
    CvSeq *seq = 0;

    if( !storage )
        CV_Error( CV_StsNullPtr, "" );
    if( header_size < sizeof( CvSeq ) || elem_size <= 0 )
        CV_Error( CV_StsBadSize, "" );

    /* allocate sequence header */
    seq = (CvSeq*)cvMemStorageAlloc( storage, header_size );
    memset( seq, 0, header_size );

    seq->header_size = (int)header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC && elemtype != CV_USRTYPE1 &&
            typesize != 0 && typesize != (int)elem_size )
            CV_Error( CV_StsBadSize,
            "Specified element size doesn't match to the size of the specified element type "
            "(try to use 0 for element type)" );
    }
    seq->elem_size = (int)elem_size;
    seq->storage = storage;

    cvSetSeqBlockSize( seq, (int)((1 << 10)/elem_size) );

    return seq;
}


/* adjusts <delta_elems> field of sequence. It determines how much the sequence
   grows if there are no free space inside the sequence buffers */
CV_IMPL void
cvSetSeqBlockSize( CvSeq *seq, int delta_elements )
{
    int elem_size;
    int useful_block_size;

    if( !seq || !seq->storage )
        CV_Error( CV_StsNullPtr, "" );
    if( delta_elements < 0 )
        CV_Error( CV_StsOutOfRange, "" );

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
            CV_Error( CV_StsOutOfRange, "Storage block size is too small "
                                        "to fit the sequence elements" );
    }

    seq->delta_elems = delta_elements;
}


/* Find a sequence element by its index: */
CV_IMPL schar*
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

    while( length < 0 )
        length += total;
    if( length > total )
        length = total;

    return length;
}


/* Copy all sequence elements into single continuous array: */
CV_IMPL void*
cvCvtSeqToArray( const CvSeq *seq, void *array, CvSlice slice )
{
    int elem_size, total;
    CvSeqReader reader;
    char *dst = (char*)array;

    if( !seq || !array )
        CV_Error( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    total = cvSliceLength( slice, seq )*elem_size;

    if( total == 0 )
        return 0;

    cvStartReadSeq( seq, &reader, 0 );
    cvSetSeqReaderPos( &reader, slice.start_index, 0 );

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

    return array;
}


/* Construct a sequence from an array without copying any data.
   NB: The resultant sequence cannot grow beyond its initial size: */
CV_IMPL CvSeq*
cvMakeSeqHeaderForArray( int seq_flags, int header_size, int elem_size,
                         void *array, int total, CvSeq *seq, CvSeqBlock * block )
{
    CvSeq* result = 0;

    if( elem_size <= 0 || header_size < (int)sizeof( CvSeq ) || total < 0 )
        CV_Error( CV_StsBadSize, "" );

    if( !seq || ((!array || !block) && total > 0) )
        CV_Error( CV_StsNullPtr, "" );

    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_Error( CV_StsBadSize,
            "Element size doesn't match to the size of predefined element type "
            "(try to use 0 for sequence element type)" );
    }
    seq->elem_size = elem_size;
    seq->total = total;
    seq->block_max = seq->ptr = (schar *) array + total * elem_size;

    if( total > 0 )
    {
        seq->first = block;
        block->prev = block->next = block;
        block->start_index = 0;
        block->count = total;
        block->data = (schar *) array;
    }

    result = seq;

    return result;
}


/* The function allocates space for at least one more sequence element.
   If there are free sequence blocks (seq->free_blocks != 0)
   they are reused, otherwise the space is allocated in the storage: */
static void
icvGrowSeq( CvSeq *seq, int in_front_of )
{
    CvSeqBlock *block;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );
    block = seq->free_blocks;

    if( !block )
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        CvMemStorage *storage = seq->storage;

        if( seq->total >= delta_elems*4 )
            cvSetSeqBlockSize( seq, delta_elems*2 );

        if( !storage )
            CV_Error( CV_StsNullPtr, "The sequence has NULL storage pointer" );

        /* If there is a free space just after last allocated block
           and it is big enough then enlarge the last block.
           This can happen only if the new block is added to the end of sequence: */
        if( (size_t)(ICV_FREE_PTR(storage) - seq->block_max) < CV_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = MIN( delta, delta_elems ) * elem_size;
            seq->block_max += delta;
            storage->free_space = cvAlignLeft((int)(((schar*)storage->top + storage->block_size) -
                                              seq->block_max), CV_STRUCT_ALIGN );
            return;
        }
        else
        {
            int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

            /* Try to allocate <delta_elements> elements: */
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
                    icvGoNextMemBlock( storage );
                    assert( storage->free_space >= delta );
                }
            }

            block = (CvSeqBlock*)cvMemStorageAlloc( storage, delta );
            block->data = (schar*)cvAlignPtr( block + 1, CV_STRUCT_ALIGN );
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

    /* For free blocks the <count> field means
     * total number of bytes in the block.
     *
     * For used blocks it means current number
     * of sequence elements in the block:
     */
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
}

/* Recycle a sequence block: */
static void
icvFreeSeqBlock( CvSeq *seq, int in_front_of )
{
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

            /* Update start indices of sequence blocks: */
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
}


/****************************************************************************************\
*                             Sequence Writer implementation                             *
\****************************************************************************************/

/* Initialize sequence writer: */
CV_IMPL void
cvStartAppendToSeq( CvSeq *seq, CvSeqWriter * writer )
{
    if( !seq || !writer )
        CV_Error( CV_StsNullPtr, "" );

    memset( writer, 0, sizeof( *writer ));
    writer->header_size = sizeof( CvSeqWriter );

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;
}


/* Initialize sequence writer: */
CV_IMPL void
cvStartWriteSeq( int seq_flags, int header_size,
                 int elem_size, CvMemStorage * storage, CvSeqWriter * writer )
{
    if( !storage || !writer )
        CV_Error( CV_StsNullPtr, "" );

    CvSeq* seq = cvCreateSeq( seq_flags, header_size, elem_size, storage );
    cvStartAppendToSeq( seq, writer );
}


/* Update sequence header: */
CV_IMPL void
cvFlushSeqWriter( CvSeqWriter * writer )
{
    if( !writer )
        CV_Error( CV_StsNullPtr, "" );

    CvSeq* seq = writer->seq;
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
}


/* Calls icvFlushSeqWriter and finishes writing process: */
CV_IMPL CvSeq *
cvEndWriteSeq( CvSeqWriter * writer )
{
    if( !writer )
        CV_Error( CV_StsNullPtr, "" );

    cvFlushSeqWriter( writer );
    CvSeq* seq = writer->seq;

    /* Truncate the last block: */
    if( writer->block && writer->seq->storage )
    {
        CvMemStorage *storage = seq->storage;
        schar *storage_block_max = (schar *) storage->top + storage->block_size;

        assert( writer->block->count > 0 );

        if( (unsigned)((storage_block_max - storage->free_space)
            - seq->block_max) < CV_STRUCT_ALIGN )
        {
            storage->free_space = cvAlignLeft((int)(storage_block_max - seq->ptr), CV_STRUCT_ALIGN);
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = 0;
    return seq;
}


/* Create new sequence block: */
CV_IMPL void
cvCreateSeqBlock( CvSeqWriter * writer )
{
    if( !writer || !writer->seq )
        CV_Error( CV_StsNullPtr, "" );

    CvSeq* seq = writer->seq;

    cvFlushSeqWriter( writer );

    icvGrowSeq( seq, 0 );

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;
}


/****************************************************************************************\
*                               Sequence Reader implementation                           *
\****************************************************************************************/

/* Initialize sequence reader: */
CV_IMPL void
cvStartReadSeq( const CvSeq *seq, CvSeqReader * reader, int reverse )
{
    CvSeqBlock *first_block;
    CvSeqBlock *last_block;

    if( reader )
    {
        reader->seq = 0;
        reader->block = 0;
        reader->ptr = reader->block_max = reader->block_min = 0;
    }

    if( !seq || !reader )
        CV_Error( CV_StsNullPtr, "" );

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
            schar *temp = reader->ptr;

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
}


/* Change the current reading block
 * to the previous or to the next:
 */
CV_IMPL void
cvChangeSeqBlock( void* _reader, int direction )
{
    CvSeqReader* reader = (CvSeqReader*)_reader;

    if( !reader )
        CV_Error( CV_StsNullPtr, "" );

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
}


/* Return the current reader position: */
CV_IMPL int
cvGetSeqReaderPos( CvSeqReader* reader )
{
    int elem_size;
    int index = -1;

    if( !reader || !reader->ptr )
        CV_Error( CV_StsNullPtr, "" );

    elem_size = reader->seq->elem_size;
    if( elem_size <= ICV_SHIFT_TAB_MAX && (index = icvPower2ShiftTab[elem_size - 1]) >= 0 )
        index = (int)((reader->ptr - reader->block_min) >> index);
    else
        index = (int)((reader->ptr - reader->block_min) / elem_size);

    index += reader->block->start_index - reader->delta_index;

    return index;
}


/* Set reader position to given position,
 * either absolute or relative to the
 *  current one:
 */
CV_IMPL void
cvSetSeqReaderPos( CvSeqReader* reader, int index, int is_relative )
{
    CvSeqBlock *block;
    int elem_size, count, total;

    if( !reader || !reader->seq )
        CV_Error( CV_StsNullPtr, "" );

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if( !is_relative )
    {
        if( index < 0 )
        {
            if( index < -total )
                CV_Error( CV_StsOutOfRange, "" );
            index += total;
        }
        else if( index >= total )
        {
            index -= total;
            if( index >= total )
                CV_Error( CV_StsOutOfRange, "" );
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
        schar* ptr = reader->ptr;
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
}


/* Push element onto the sequence: */
CV_IMPL schar*
cvSeqPush( CvSeq *seq, const void *element )
{
    schar *ptr = 0;
    size_t elem_size;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if( ptr >= seq->block_max )
    {
        icvGrowSeq( seq, 0 );

        ptr = seq->ptr;
        assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */  );
    }

    if( element )
        memcpy( ptr, element, elem_size );
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    return ptr;
}


/* Pop last element off of the sequence: */
CV_IMPL void
cvSeqPop( CvSeq *seq, void *element )
{
    schar *ptr;
    int elem_size;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_Error( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    seq->ptr = ptr = seq->ptr - elem_size;

    if( element )
        memcpy( element, ptr, elem_size );
    seq->ptr = ptr;
    seq->total--;

    if( --(seq->first->prev->count) == 0 )
    {
        icvFreeSeqBlock( seq, 0 );
        assert( seq->ptr == seq->block_max );
    }
}


/* Push element onto the front of the sequence: */
CV_IMPL schar*
cvSeqPushFront( CvSeq *seq, const void *element )
{
    schar* ptr = 0;
    int elem_size;
    CvSeqBlock *block;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( !block || block->start_index == 0 )
    {
        icvGrowSeq( seq, 1 );

        block = seq->first;
        assert( block->start_index > 0 );
    }

    ptr = block->data -= elem_size;

    if( element )
        memcpy( ptr, element, elem_size );
    block->count++;
    block->start_index--;
    seq->total++;

    return ptr;
}


/* Shift out first element of the sequence: */
CV_IMPL void
cvSeqPopFront( CvSeq *seq, void *element )
{
    int elem_size;
    CvSeqBlock *block;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_Error( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( element )
        memcpy( element, block->data, elem_size );
    block->data += elem_size;
    block->start_index++;
    seq->total--;

    if( --(block->count) == 0 )
        icvFreeSeqBlock( seq, 1 );
}

/* Insert new element in middle of sequence: */
CV_IMPL schar*
cvSeqInsert( CvSeq *seq, int before_index, const void *element )
{
    int elem_size;
    int block_size;
    CvSeqBlock *block;
    int delta_index;
    int total;
    schar* ret_ptr = 0;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );

    total = seq->total;
    before_index += before_index < 0 ? total : 0;
    before_index -= before_index > total ? total : 0;

    if( (unsigned)before_index > (unsigned)total )
        CV_Error( CV_StsOutOfRange, "" );

    if( before_index == total )
    {
        ret_ptr = cvSeqPush( seq, element );
    }
    else if( before_index == 0 )
    {
        ret_ptr = cvSeqPushFront( seq, element );
    }
    else
    {
        elem_size = seq->elem_size;

        if( before_index >= total >> 1 )
        {
            schar *ptr = seq->ptr + elem_size;

            if( ptr > seq->block_max )
            {
                icvGrowSeq( seq, 0 );

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

                /* Check that we don't fall into an infinite loop: */
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
                icvGrowSeq( seq, 1 );

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

                /* Check that we don't fall into an infinite loop: */
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

    return ret_ptr;
}


/* Removes element from sequence: */
CV_IMPL void
cvSeqRemove( CvSeq *seq, int index )
{
    schar *ptr;
    int elem_size;
    int block_size;
    CvSeqBlock *block;
    int delta_index;
    int total, front = 0;

    if( !seq )
        CV_Error( CV_StsNullPtr, "" );

    total = seq->total;

    index += index < 0 ? total : 0;
    index -= index >= total ? total : 0;

    if( (unsigned) index >= (unsigned) total )
        CV_Error( CV_StsOutOfRange, "Invalid index" );

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
}


/* Remove several elements from the end of sequence: */
CV_IMPL void
cvSeqPopMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    if( !seq )
        CV_Error( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_Error( CV_StsBadSize, "number of removed elements is negative" );

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
}


/* Remove all elements from a sequence: */
CV_IMPL void
cvClearSeq( CvSeq *seq )
{
    if( !seq )
        CV_Error( CV_StsNullPtr, "" );
    cvSeqPopMulti( seq, 0, seq->total );
}



typedef struct CvSeqReaderPos
{
    CvSeqBlock* block;
    schar* ptr;
    schar* block_min;
    schar* block_max;
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

inline schar*
icvMed3( schar* a, schar* b, schar* c, CvCmpFunc cmp_func, void* aux )
{
    return cmp_func(a, b, aux) < 0 ?
      (cmp_func(b, c, aux) < 0 ? b : cmp_func(a, c, aux) < 0 ? c : a)
     :(cmp_func(b, c, aux) > 0 ? b : cmp_func(a, c, aux) < 0 ? a : c);
}


CV_IMPL void
cvSeqInvert( CvSeq* seq )
{
    CvSeqReader left_reader, right_reader;
    int elem_size;
    int i, count;

    cvStartReadSeq( seq, &left_reader, 0 );
    cvStartReadSeq( seq, &right_reader, 1 );
    elem_size = seq->elem_size;
    count = seq->total >> 1;

    for( i = 0; i < count; i++ )
    {
        CV_SWAP_ELEMS( left_reader.ptr, right_reader.ptr, elem_size );
        CV_NEXT_SEQ_ELEM( elem_size, left_reader );
        CV_PREV_SEQ_ELEM( elem_size, right_reader );
    }
}


/****************************************************************************************\
*                                      Set implementation                                *
\****************************************************************************************/

/* Creates empty set: */
CV_IMPL CvSet*
cvCreateSet( int set_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    if( !storage )
        CV_Error( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSet ) ||
        elem_size < (int)sizeof(void*)*2 ||
        (elem_size & (sizeof(void*)-1)) != 0 )
        CV_Error( CV_StsBadSize, "" );

    CvSet* set = (CvSet*) cvCreateSeq( set_flags, header_size, elem_size, storage );
    set->flags = (set->flags & ~CV_MAGIC_MASK) | CV_SET_MAGIC_VAL;

    return set;
}


/* Add new element to the set: */
CV_IMPL int
cvSetAdd( CvSet* set, CvSetElem* element, CvSetElem** inserted_element )
{
    int id = -1;
    CvSetElem *free_elem;

    if( !set )
        CV_Error( CV_StsNullPtr, "" );

    if( !(set->free_elems) )
    {
        int count = set->total;
        int elem_size = set->elem_size;
        schar *ptr;
        icvGrowSeq( (CvSeq *) set, 0 );

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
        memcpy( free_elem, element, set->elem_size );

    free_elem->flags = id;
    set->active_count++;

    if( inserted_element )
        *inserted_element = free_elem;

    return id;
}


/* Remove element from a set given element index: */
CV_IMPL void
cvSetRemove( CvSet* set, int index )
{
    CvSetElem* elem = cvGetSetElem( set, index );
    if( elem )
        cvSetRemoveByPtr( set, elem );
    else if( !set )
        CV_Error( CV_StsNullPtr, "" );
}


/* Remove all elements from a set: */
CV_IMPL void
cvClearSet( CvSet* set )
{
    cvClearSeq( (CvSeq*)set );
    set->free_elems = 0;
    set->active_count = 0;
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
// then added contour will have null pointer to parent:
CV_IMPL void
cvInsertNodeIntoTree( void* _node, void* _parent, void* _frame )
{
    CvTreeNode* node = (CvTreeNode*)_node;
    CvTreeNode* parent = (CvTreeNode*)_parent;

    if( !node || !parent )
        CV_Error( CV_StsNullPtr, "" );

    node->v_prev = _parent != _frame ? parent : 0;
    node->h_next = parent->v_next;

    assert( parent->v_next != node );

    if( parent->v_next )
        parent->v_next->h_prev = node;
    parent->v_next = node;
}



/* End of file. */

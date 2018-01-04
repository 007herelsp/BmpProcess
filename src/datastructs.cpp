
#include "_cxcore.h"

#define IVOS_FREE_PTR(storage) \
    ((char *)(storage)->top + (storage)->block_size - (storage)->free_space)

#define IVOS_ALIGNED_SEQ_BLOCK_SIZE \
    (int)Align(sizeof(SeqBlock), VOS_STRUCT_ALIGN)

VOS_INLINE int
sysAlignLeft(int size, int align)
{
    return size & -align;
}

#define VOS_GET_LAST_ELEM(seq, block) \
    ((block)->data + ((block)->count - 1) * ((seq)->elem_size))


#define IVOS_SHIFT_TAB_MAX 32
static const char iPower2ShiftTab[] =
    {
        0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5};

/****************************************************************************************\
*            Functions for manipulating memory storage - list of memory blocks           *
\****************************************************************************************/

/* initializes allocated storage */
static void
iInitMemStorage(MemStorage *storage, int block_size)
{
    VOS_FUNCNAME("iInitMemStorage ");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (block_size <= 0)
        block_size = VOS_STORAGE_BLOCK_SIZE;

    block_size = Align(block_size, VOS_STRUCT_ALIGN);
    assert(sizeof(MemBlock) % VOS_STRUCT_ALIGN == 0);

    VOS_MEMSET(storage, 0, sizeof(*storage));
    storage->signature = VOS_STORAGE_MAGIC_VAL;
    storage->block_size = block_size;

    __END__;
}

/* creates root memory storage */
MemStorage *
CreateMemStorage(int block_size)
{
    MemStorage *storage = 0;

    VOS_FUNCNAME("CreateMemStorage");

    __BEGIN__;

    VOS_CALL(storage = (MemStorage *)SysAlloc(sizeof(MemStorage)));
    VOS_CALL(iInitMemStorage(storage, block_size));

    __END__;

    if (GetErrStatus() < 0)
        SYS_FREE(&storage);

    return storage;
}

/* creates child memory storage */
MemStorage *
CreateChildMemStorage(MemStorage *parent)
{
    MemStorage *storage = NULL;
    VOS_FUNCNAME("CreateChildMemStorage");

    __BEGIN__;

    if (!parent)
        VOS_ERROR(VOS_StsNullPtr, "");

    VOS_CALL(storage = CreateMemStorage(parent->block_size));
    storage->parent = parent;

    __END__;

    if (GetErrStatus() < 0)
        SYS_FREE(&storage);

    return storage;
}

/* releases all blocks of the storage (or returns them to parent if any) */
static void
iDestroyMemStorage(MemStorage *storage)
{
    VOS_FUNCNAME("iDestroyMemStorage");

    __BEGIN__;

    int k = 0;

    MemBlock *block = NULL;
    MemBlock *dst_top = NULL;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (storage->parent)
        dst_top = storage->parent->top;

    for (block = storage->bottom; block != 0; k++)
    {
        MemBlock *temp = block;

        block = block->next;
        if (storage->parent)
        {
            if (dst_top)
            {
                temp->prev = dst_top;
                temp->next = dst_top->next;
                if (temp->next)
                    temp->next->prev = temp;
                dst_top = dst_top->next = temp;
            }
            else
            {
                dst_top = storage->parent->bottom = storage->parent->top = temp;
                temp->prev = temp->next = NULL;
                storage->free_space = storage->block_size - sizeof(*temp);
            }
        }
        else
        {
            SYS_FREE(&temp);
        }
    }

    storage->top = storage->bottom = NULL;
    storage->free_space = 0;

    __END__;
}

/* releases memory storage */
void ReleaseMemStorage(MemStorage **storage)
{
    MemStorage *st;
    VOS_FUNCNAME("ReleaseMemStorage");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    st = *storage;
    *storage = NULL;

    if (st)
    {
        VOS_CALL(iDestroyMemStorage(st));
        SYS_FREE(&st);
    }

    __END__;
}

/* clears memory storage (returns blocks to the parent if any) */
void ClearMemStorage(MemStorage *storage)
{
    VOS_FUNCNAME("ClearMemStorage");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (storage->parent)
    {
        iDestroyMemStorage(storage);
    }
    else
    {
        storage->top = storage->bottom;
        storage->free_space = storage->bottom ? storage->block_size - sizeof(MemBlock) : 0;
    }

    __END__;
}


static void
iGoNextMemBlock(MemStorage *storage)
{
    VOS_FUNCNAME("iGoNextMemBlock");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (!storage->top || !storage->top->next)
    {
        MemBlock *block;

        if (!(storage->parent))
        {
            VOS_CALL(block = (MemBlock *)SysAlloc(storage->block_size));
        }
        else
        {
            MemStorage *parent = storage->parent;
            MemStoragePos parent_pos;

            SaveMemStoragePos(parent, &parent_pos);
            VOS_CALL(iGoNextMemBlock(parent));

            block = parent->top;
            RestoreMemStoragePos(parent, &parent_pos);

            if (block == parent->top) /* the single allocated block */
            {
                assert(parent->bottom == block);
                parent->top = parent->bottom = NULL;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if (block->next)
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = NULL;
        block->prev = storage->top;

        if (storage->top)
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if (storage->top->next)
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(MemBlock);
    assert(storage->free_space % VOS_STRUCT_ALIGN == 0);

    __END__;
}

/* remembers memory storage position */
void SaveMemStoragePos(const MemStorage *storage, MemStoragePos *pos)
{
    VOS_FUNCNAME("SaveMemStoragePos");

    __BEGIN__;

    if (!storage || !pos)
        VOS_ERROR(VOS_StsNullPtr, "");

    pos->top = storage->top;
    pos->free_space = storage->free_space;

    __END__;
}

/* restores memory storage position */
void RestoreMemStoragePos(MemStorage *storage, MemStoragePos *pos)
{
    VOS_FUNCNAME("RestoreMemStoragePos");

    __BEGIN__;

    if (!storage || !pos)
        VOS_ERROR(VOS_StsNullPtr, "");
    if (pos->free_space > storage->block_size)
        VOS_ERROR(VOS_StsBadSize, "");

    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if (!storage->top)
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(MemBlock) : 0;
    }

    __END__;
}

/* Allocates continuous buffer of the specified size in the storage */
void *
MemStorageAlloc(MemStorage *storage, size_t size)
{
    char *ptr = NULL;

    VOS_FUNCNAME("MemStorageAlloc");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "NULL storage pointer");

    if (size > INT_MAX)
        VOS_ERROR(VOS_StsOutOfRange, "Too large memory block is requested");

    assert(storage->free_space % VOS_STRUCT_ALIGN == 0);

    if ((size_t)storage->free_space < size)
    {
        size_t max_free_space = sysAlignLeft(storage->block_size - sizeof(MemBlock), VOS_STRUCT_ALIGN);
        if (max_free_space < size)
            VOS_ERROR(VOS_StsOutOfRange, "requested size is negative or too big");

        VOS_CALL(iGoNextMemBlock(storage));
    }

    ptr = IVOS_FREE_PTR(storage);
    assert((size_t)ptr % VOS_STRUCT_ALIGN == 0);
    storage->free_space = sysAlignLeft(storage->free_space - (int)size, VOS_STRUCT_ALIGN);

    __END__;

    return ptr;
}

/****************************************************************************************\
*                               Sequence implementation                                  *
\****************************************************************************************/

Seq *CreateSeq(int seq_flags, int header_size, int elem_size, MemStorage *storage)
{
    Seq *seq = NULL;

    VOS_FUNCNAME("CreateSeq");

    __BEGIN__;

    if (!storage)
        VOS_ERROR(VOS_StsNullPtr, "");
    if (header_size < (int)sizeof(Seq) || elem_size <= 0)
        VOS_ERROR(VOS_StsBadSize, "");

    /* allocate sequence header */
    VOS_CALL(seq = (Seq *)MemStorageAlloc(storage, header_size));
    VOS_MEMSET(seq, 0, header_size);

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~VOS_MAGIC_MASK) | VOS_SEQ_MAGIC_VAL;
    {
        int elemtype = VOS_MAT_TYPE(seq_flags);
        int typesize = VOS_ELEM_SIZE(elemtype);

        if (elemtype != VOS_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size)
            VOS_ERROR(VOS_StsBadSize,
                      "Specified element size doesn't match to the size of the specified element type "
                      "(try to use 0 for element type)");
    }
    seq->elem_size = elem_size;
    seq->storage = storage;

    VOS_CALL(SetSeqBlockSize(seq, (1 << 10) / elem_size));

    __END__;

    return seq;
}

void SetSeqBlockSize(Seq *seq, int delta_elements)
{
    int elem_size;
    int useful_block_size;

    VOS_FUNCNAME("SetSeqBlockSize");

    __BEGIN__;

    if (!seq || !seq->storage)
        VOS_ERROR(VOS_StsNullPtr, "");
    if (delta_elements < 0)
        VOS_ERROR(VOS_StsOutOfRange, "");

    useful_block_size = sysAlignLeft(seq->storage->block_size - sizeof(MemBlock) -
                                        sizeof(SeqBlock),
                                    VOS_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if (0 == delta_elements)
    {
        delta_elements = (1 << 10) / elem_size;
        delta_elements = VOS_MAX(delta_elements, 1);
    }
    if (delta_elements * elem_size > useful_block_size)
    {
        delta_elements = useful_block_size / elem_size;
        if (0 == delta_elements)
            VOS_ERROR(VOS_StsOutOfRange, "Storage block size is too small "
                                         "to fit the sequence elements");
    }

    seq->delta_elems = delta_elements;

    __END__;
}

/* finds sequence element by its index */
char *
GetSeqElem(const Seq *seq, int index)
{
    SeqBlock *block;
    int count, total = seq->total;

    if ((unsigned)index >= (unsigned)total)
    {
        index += index < 0 ? total : 0;
        index -= index >= total ? total : 0;
        if ((unsigned)index >= (unsigned)total)
            return NULL;
    }

    block = seq->first;
    if (index + index <= total)
    {
        while (index >= (count = block->count))
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
        } while (index < total);
        index -= total;
    }

    return block->data + index * seq->elem_size;
}

int SliceLength(Slice slice, const Seq *seq)
{
    int total = seq->total;
    int length = slice.end_index - slice.start_index;

    if (length != 0)
    {
        if (slice.start_index < 0)
            slice.start_index += total;
        if (slice.end_index <= 0)
            slice.end_index += total;

        length = slice.end_index - slice.start_index;
    }

    if (length < 0)
    {
        length += total;
    }
    else if (length > total)
        length = total;

    return length;
}

static void
iGrowSeq(Seq *seq, int in_front_of)
{
    VOS_FUNCNAME("iGrowSeq");

    __BEGIN__;

    SeqBlock *block;

    if (!seq)
        VOS_ERROR(VOS_StsNullPtr, "");
    block = seq->free_blocks;

    if (!block)
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        MemStorage *storage = seq->storage;

        if (seq->total >= delta_elems * 4)
            SetSeqBlockSize(seq, delta_elems * 2);

        if (!storage)
            VOS_ERROR(VOS_StsNullPtr, "The sequence has NULL storage pointer");

        if ((unsigned)(IVOS_FREE_PTR(storage) - seq->block_max) < VOS_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of)
        {
            int delta = storage->free_space / elem_size;

            delta = VOS_MIN(delta, delta_elems) * elem_size;
            seq->block_max += delta;
            storage->free_space = sysAlignLeft((int)(((char *)storage->top + storage->block_size) -
                                                    seq->block_max),
                                              VOS_STRUCT_ALIGN);
            EXIT;
        }
        else
        {
            int delta = elem_size * delta_elems + IVOS_ALIGNED_SEQ_BLOCK_SIZE;

            /* try to allocate <delta_elements> elements */
            if (storage->free_space < delta)
            {
                int small_block_size = VOS_MAX(1, delta_elems / 3) * elem_size +
                                       IVOS_ALIGNED_SEQ_BLOCK_SIZE;
                /* try to allocate smaller part */
                if (storage->free_space >= small_block_size + VOS_STRUCT_ALIGN)
                {
                    delta = (storage->free_space - IVOS_ALIGNED_SEQ_BLOCK_SIZE) / seq->elem_size;
                    delta = delta * seq->elem_size + IVOS_ALIGNED_SEQ_BLOCK_SIZE;
                }
                else
                {
                    VOS_CALL(iGoNextMemBlock(storage));
                    assert(storage->free_space >= delta);
                }
            }

            VOS_CALL(block = (SeqBlock *)MemStorageAlloc(storage, delta));
            block->data = (char *)AlignPtr(block + 1, VOS_STRUCT_ALIGN);
            block->count = delta - IVOS_ALIGNED_SEQ_BLOCK_SIZE;
            block->prev = block->next = NULL;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if (!(seq->first))
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
    assert(seq->elem_size != 0);
    assert(block->count % seq->elem_size == 0 && block->count > 0);

    if (!in_front_of)
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 : block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if (block != block->prev)
        {
            assert(0 == seq->first->start_index);
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for (;;)
        {
            block->start_index += delta;
            block = block->next;
            if (block == seq->first)
                break;
        }
    }

    block->count = 0;

    __END__;
}

/* recycles a sequence block for the further use */
static void
iFreeSeqBlock(Seq *seq, int in_front_of)
{
    /*VOS_FUNCNAME( "iFreeSeqBlock" );*/

    __BEGIN__;

    SeqBlock *block = seq->first;

    assert((in_front_of ? block : block->prev)->count == 0);

    if (block == block->prev) /* single block case */
    {
        block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
        block->data = seq->block_max - block->count;
        seq->first = 0;
        seq->ptr = seq->block_max = 0;
        seq->total = 0;
    }
    else
    {
        if (!in_front_of)
        {
            block = block->prev;
            assert(seq->ptr == block->data);

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
            for (;;)
            {
                block->start_index -= delta;
                block = block->next;
                if (block == seq->first)
                    break;
            }

            seq->first = block->next;
        }

        block->prev->next = block->next;
        block->next->prev = block->prev;
    }
    assert(0 != seq->elem_size);
    assert(block->count > 0 && block->count % seq->elem_size == 0);
    block->next = seq->free_blocks;
    seq->free_blocks = block;

    __END__;
}

/****************************************************************************************\
*                             Sequence Writer implementation                             *
\****************************************************************************************/

/* initializes sequence writer */
void StartAppendToSeq(Seq *seq, SeqWriter *writer)
{
    VOS_FUNCNAME("StartAppendToSeq");

    __BEGIN__;

    if (!seq || !writer)
        VOS_ERROR(VOS_StsNullPtr, "");

    VOS_MEMSET(writer, 0, sizeof(*writer));
    writer->header_size = sizeof(SeqWriter);

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}

/* initializes sequence writer */
void StartWriteSeq(int seq_flags, int header_size,
                   int elem_size, MemStorage *storage, SeqWriter *writer)
{
    Seq *seq = 0;

    VOS_FUNCNAME("StartWriteSeq");

    __BEGIN__;

    if (!storage || !writer)
        VOS_ERROR(VOS_StsNullPtr, "");

    VOS_CALL(seq = CreateSeq(seq_flags, header_size, elem_size, storage));
    StartAppendToSeq(seq, writer);

    __END__;
}

/* updates sequence header */
void FlushSeqWriter(SeqWriter *writer)
{
    Seq *seq = 0;

    VOS_FUNCNAME("FlushSeqWriter");

    __BEGIN__;

    if (!writer)
        VOS_ERROR(VOS_StsNullPtr, "");

    seq = writer->seq;
    seq->ptr = writer->ptr;

    if (writer->block)
    {
        int total = 0;
        SeqBlock *first_block = writer->seq->first;
        SeqBlock *block = first_block;

        writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
        assert(writer->block->count > 0);

        do
        {
            total += block->count;
            block = block->next;
        } while (block != first_block);

        writer->seq->total = total;
    }

    __END__;
}

/* calls icvFlushSeqWriter and finishes writing process */
Seq *EndWriteSeq(SeqWriter *writer)
{
    Seq *seq = NULL;

    VOS_FUNCNAME("EndWriteSeq");

    __BEGIN__;

    if (!writer)
        VOS_ERROR(VOS_StsNullPtr, "");

    VOS_CALL(FlushSeqWriter(writer));
    seq = writer->seq;

    /* truncate the last block */
    if (writer->block && writer->seq->storage)
    {
        MemStorage *storage = seq->storage;
        char *storage_block_max = (char *)storage->top + storage->block_size;

        assert(writer->block->count > 0);

        if ((unsigned)((storage_block_max - storage->free_space) - seq->block_max) < VOS_STRUCT_ALIGN)
        {
            storage->free_space = sysAlignLeft((int)(storage_block_max - seq->ptr), VOS_STRUCT_ALIGN);
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = NULL;

    __END__;

    return seq;
}

/* creates new sequence block */
void CreateSeqBlock(SeqWriter *writer)
{
    VOS_FUNCNAME("CreateSeqBlock");

    __BEGIN__;

    Seq *seq;

    if (!writer || !writer->seq)
        VOS_ERROR(VOS_StsNullPtr, "");

    seq = writer->seq;

    FlushSeqWriter(writer);

    VOS_CALL(iGrowSeq(seq, 0));

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}

/****************************************************************************************\
*                               Sequence Reader implementation                           *
\****************************************************************************************/

/* initializes sequence reader */
void StartReadSeq(const Seq *seq, SeqReader *reader, int reverse)
{
    SeqBlock *first_block;
    SeqBlock *last_block;

    VOS_FUNCNAME("StartReadSeq");

    if (reader)
    {
        reader->seq = NULL;
        reader->block = NULL;
        reader->ptr = reader->block_max = reader->block_min = NULL;
    }

    __BEGIN__;

    if (!seq || !reader)
        VOS_ERROR(VOS_StsNullPtr, "");

    reader->header_size = sizeof(SeqReader);
    reader->seq = (Seq *)seq;

    first_block = seq->first;

    if (first_block)
    {
        last_block = first_block->prev;
        reader->ptr = first_block->data;
        reader->prev_elem = VOS_GET_LAST_ELEM(seq, last_block);
        reader->delta_index = seq->first->start_index;

        if (reverse)
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
        reader->block = NULL;

        reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = NULL;
    }

    __END__;
}

/* changes the current reading block to the previous or to the next */
void ChangeSeqBlock(void *_reader, int direction)
{
    VOS_FUNCNAME("cvChangeSeqBlock");

    __BEGIN__;

    SeqReader *reader = (SeqReader *)_reader;

    if (!reader)
        VOS_ERROR(VOS_StsNullPtr, "");

    if (direction > 0)
    {
        reader->block = reader->block->next;
        reader->ptr = reader->block->data;
    }
    else
    {
        reader->block = reader->block->prev;
        reader->ptr = VOS_GET_LAST_ELEM(reader->seq, reader->block);
    }
    reader->block_min = reader->block->data;
    reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;

    __END__;
}

/* returns the current reader position */
int GetSeqReaderPos(SeqReader *reader)
{
    int elem_size;
    int index = -1;

    VOS_FUNCNAME("GetSeqReaderPos");

    __BEGIN__;

    if (!reader || !reader->ptr)
        VOS_ERROR(VOS_StsNullPtr, "");

    elem_size = reader->seq->elem_size;
    if (elem_size - 1 < 0)
    {
        VOS_ERROR(VOS_StsBadSize, "");
    }
    if (elem_size <= IVOS_SHIFT_TAB_MAX && (index = iPower2ShiftTab[elem_size - 1]) >= 0)
        index = (int)((reader->ptr - reader->block_min) >> index);
    else
        index = (int)((reader->ptr - reader->block_min) / elem_size);

    index += reader->block->start_index - reader->delta_index;

    __END__;

    return index;
}

void SetSeqReaderPos(SeqReader *reader, int index, int is_relative)
{
    VOS_FUNCNAME("cvSetSeqReaderPos");

    __BEGIN__;

    SeqBlock *block;
    int elem_size, count, total;

    if (!reader || !reader->seq)
        VOS_ERROR(VOS_StsNullPtr, "");

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if (!is_relative)
    {
        if (index < 0)
        {
            if (index < -total)
                VOS_ERROR(VOS_StsOutOfRange, "");
            index += total;
        }
        else if (index >= total)
        {
            index -= total;
            if (index >= total)
                VOS_ERROR(VOS_StsOutOfRange, "");
        }

        block = reader->seq->first;
        if (index >= (count = block->count))
        {
            if (index + index <= total)
            {
                do
                {
                    block = block->next;
                    index -= count;
                } while (index >= (count = block->count));
            }
            else
            {
                do
                {
                    block = block->prev;
                    total -= block->count;
                } while (index < total);
                index -= total;
            }
        }
        reader->ptr = block->data + index * elem_size;
        if (reader->block != block)
        {
            reader->block = block;
            reader->block_min = block->data;
            reader->block_max = block->data + block->count * elem_size;
        }
    }
    else
    {
        char *ptr = reader->ptr;
        index *= elem_size;
        block = reader->block;

        if (index > 0)
        {
            while (ptr + index >= reader->block_max)
            {
                int delta = (int)(reader->block_max - ptr);
                index -= delta;
                reader->block = block = block->next;
                reader->block_min = ptr = block->data;
                reader->block_max = block->data + block->count * elem_size;
            }
            reader->ptr = ptr + index;
        }
        else
        {
            while (ptr + index < reader->block_min)
            {
                int delta = (int)(ptr - reader->block_min);
                index += delta;
                reader->block = block = block->prev;
                reader->block_min = block->data;
                reader->block_max = ptr = block->data + block->count * elem_size;
            }
            reader->ptr = ptr + index;
        }
    }

    __END__;
}

/* pushes element to the sequence */
char *
SeqPush(Seq *seq, void *element)
{
    char *ptr = NULL;
    size_t elem_size;

    VOS_FUNCNAME("SeqPush");

    __BEGIN__;

    if (!seq)
        VOS_ERROR(VOS_StsNullPtr, "");

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if (ptr >= seq->block_max)
    {
        VOS_CALL(iGrowSeq(seq, 0));

        ptr = seq->ptr;
        assert(ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */);
    }

    if (element)
        VOS_SYS_MEMCPY_AUTO(ptr, element, elem_size);
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    __END__;

    return ptr;
}

/* pops the last element out of the sequence */
void SeqPop(Seq *seq, void *element)
{
    char *ptr;
    int elem_size;

    VOS_FUNCNAME("SeqPop");

    __BEGIN__;

    if (!seq)
        VOS_ERROR(VOS_StsNullPtr, "");
    if (seq->total <= 0)
        VOS_ERROR(VOS_StsBadSize, "");

    elem_size = seq->elem_size;
    seq->ptr = ptr = seq->ptr - elem_size;

    if (element)
        VOS_SYS_MEMCPY_AUTO(element, ptr, elem_size);
    seq->ptr = ptr;
    seq->total--;

    if (--(seq->first->prev->count) == 0)
    {
        iFreeSeqBlock(seq, 0);
        assert(seq->ptr == seq->block_max);
    }

    __END__;
}

/* removes several elements from the end of sequence */
void SeqPopMulti(Seq *seq, void *_elements, int count, int front)
{
    char *elements = (char *)_elements;

    VOS_FUNCNAME("SeqPopMulti");

    __BEGIN__;

    if (!seq)
        VOS_ERROR(VOS_StsNullPtr, "NULL sequence pointer");
    if (count < 0)
        VOS_ERROR(VOS_StsBadSize, "number of removed elements is negative");

    count = VOS_MIN(count, seq->total);

    if (!front)
    {
        if (elements)
            elements += count * seq->elem_size;

        while (count > 0)
        {
            int delta = seq->first->prev->count;

            delta = VOS_MIN(delta, count);
            assert(delta > 0);

            seq->first->prev->count -= delta;
            seq->total -= delta;
            count -= delta;
            delta *= seq->elem_size;
            seq->ptr -= delta;

            if (elements)
            {
                elements -= delta;
                VOS_MEMCPY(elements, seq->ptr, delta);
            }

            if (seq->first->prev->count == 0)
                iFreeSeqBlock(seq, 0);
        }
    }
    else
    {
        while (count > 0)
        {
            int delta = seq->first->count;

            delta = VOS_MIN(delta, count);
            assert(delta > 0);

            seq->first->count -= delta;
            seq->total -= delta;
            count -= delta;
            seq->first->start_index += delta;
            delta *= seq->elem_size;

            if (elements)
            {
                VOS_MEMCPY(elements, seq->first->data, delta);
                elements += delta;
            }

            seq->first->data += delta;
            if (0 == seq->first->count)
                iFreeSeqBlock(seq, 1);
        }
    }

    __END__;
}

typedef struct CvSeqReaderPos
{
    SeqBlock *block;
    char *ptr;
    char *block_min;
    char *block_max;
} CvSeqReaderPos;

typedef struct CvTreeNode
{
    int flags;                 /* micsellaneous flags */
    int header_size;           /* size of sequence header */
    struct CvTreeNode *h_prev; /* previous sequence */
    struct CvTreeNode *h_next; /* next sequence */
    struct CvTreeNode *v_prev; /* 2nd previous sequence */
    struct CvTreeNode *v_next; /* 2nd next sequence */
} CvTreeNode;

void InsertNodeIntoTree(void *_node, void *_parent, void *_frame)
{
    VOS_FUNCNAME("InsertNodeIntoTree");

    __BEGIN__;

    CvTreeNode *node = (CvTreeNode *)_node;
    CvTreeNode *parent = (CvTreeNode *)_parent;

    if (!node || !parent)
        VOS_ERROR(VOS_StsNullPtr, "");

    node->v_prev = _parent != _frame ? parent : NULL;
    node->h_next = parent->v_next;

    assert(parent->v_next != node);

    if (parent->v_next)
        parent->v_next->h_prev = node;
    parent->v_next = node;

    __END__;
}

/* End of file. */

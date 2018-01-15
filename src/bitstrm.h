#ifndef _BITSTRM_H_
#define _BITSTRM_H_

#include "types.h"

#include <limits.h>
#include <ctype.h>
#include <stdio.h>

class RBaseStream
{
public:
    RBaseStream();
    virtual ~RBaseStream();

    virtual bool Open(const char *filename);
    virtual void Close();
    bool IsOpened();
    void SetPos(int pos);
    int GetPos();
    void Skip(int bytes);

protected:
    uchar *m_start;
    uchar *m_end;
    uchar *m_current;
    FILE *m_file;
    int m_unGetsize;
    int m_block_size;
    int m_block_pos;
    bool m_is_opened;

    virtual void ReadBlock();
    virtual void Release();
    virtual void Allocate();
};

class RLByteStream : public RBaseStream
{
public:
    virtual ~RLByteStream();

    int GetByte();
    void GetBytes(void *buffer, int count, int *readed = 0);
    int GetWord();
    int GetDWord();
};

class WBaseStream
{
public:
    WBaseStream();
    virtual ~WBaseStream();

    virtual bool Open(const char *filename);
    virtual void Close();
    bool IsOpened();
    int GetPos();

protected:
    uchar *m_start;
    uchar *m_end;
    uchar *m_current;
    int m_block_size;
    int m_block_pos;
    FILE *m_file;
    bool m_is_opened;

    virtual void WriteBlock();
    virtual void Release();
    virtual void Allocate();
};

class WLByteStream : public WBaseStream
{
public:
    virtual ~WLByteStream();

    void PutByte(int val);
    void PutBytes(const void *buffer, int count);
    void PutWord(int val);
    void PutDWord(int val);
};

#endif /*_BITSTRM_H_*/

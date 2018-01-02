#ifndef _BITSTRM_H_
#define _BITSTRM_H_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

typedef unsigned char uchar;
typedef unsigned long ulong;

// class RBaseStream - base class for other reading streams.
class RBaseStream
{
public:
    //methods
    RBaseStream();
    virtual ~RBaseStream();
    
    virtual bool  Open( const char* filename );
    virtual void  Close();
    void          SetBlockSize( int block_size, int unGetsize = 4 );
    bool          IsOpened();
    void          SetPos( int pos );
    int           GetPos();
    void          Skip( int bytes );
    
protected:

    uchar*  m_start;
    uchar*  m_end;
    uchar*  m_current;
    FILE*   m_file;
    int     m_unGetsize;
    int     m_block_size;
    int     m_block_pos;
    bool    m_jmp_set;
    bool    m_is_opened;

    virtual void  ReadBlock();
    virtual void  Release();
    virtual void  Allocate();
};

// class RLByteStream - uchar-oriented stream.
// l in prefix means that the least significant uchar of a multi-uchar value goes first
class RLByteStream : public RBaseStream
{
public:
    virtual ~RLByteStream();
    
    int     GetByte();
    void    GetBytes( void* buffer, int count, int* readed = 0 );
    int     GetWord();
    int     GetDWord(); 
};

// WBaseStream - base class for output streams
class WBaseStream
{
public:
    //methods
    WBaseStream();
    virtual ~WBaseStream();
    
    virtual bool  Open( const char* filename );
    virtual void  Close();
    void          SetBlockSize( int block_size );
    bool          IsOpened();
    int           GetPos();
    
protected:
    
    uchar*  m_start;
    uchar*  m_end;
    uchar*  m_current;
    int     m_block_size;
    int     m_block_pos;
    FILE*   m_file;
    bool    m_is_opened;
    
    virtual void  WriteBlock();
    virtual void  Release();
    virtual void  Allocate();
};


// class WLByteStream - uchar-oriented stream.
// l in prefix means that the least significant uchar of a multi-byte value goes first
class WLByteStream : public WBaseStream
{
public:
    virtual ~WLByteStream();

    void    PutByte( int val );
    void    PutBytes( const void* buffer, int count );
    void    PutWord( int val );
    void    PutDWord( int val ); 
};


#endif/*_BITSTRM_H_*/

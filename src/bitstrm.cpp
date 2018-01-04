#include "highgui.h"
#include "bitstrm.h"

#define  BS_DEF_BLOCK_SIZE   (1<<15)


/////////////////////////  RBaseStream ////////////////////////////

bool  RBaseStream::IsOpened()
{ 
    return m_is_opened;
}

void  RBaseStream::Allocate()
{
    if( !m_start )
    {
        m_start = new uchar[m_block_size + m_unGetsize];
        m_start+= m_unGetsize;
    }
    m_end = m_start + m_block_size;
    m_current = m_end;
}


RBaseStream::RBaseStream()
{
    m_start = m_end = m_current = NULL;
    m_file = NULL;
    m_block_size = BS_DEF_BLOCK_SIZE;
    m_unGetsize = 4; // 32 bits
    m_is_opened = false;
}


RBaseStream::~RBaseStream()
{
    Close();    // Close files
    Release();  // free  buffers
}


void  RBaseStream::ReadBlock()
{
    size_t readed;
    assert(  NULL!= m_file );

    // copy unget buffer
    if( m_start )
    {
        VOS_MEMCPY( m_start - m_unGetsize, m_end - m_unGetsize, m_unGetsize );
    }

    SetPos( GetPos() ); // normalize position

    fseek( m_file, m_block_pos, SEEK_SET );
    readed = fread( m_start, 1, m_block_size, m_file );
    m_end = m_start + readed;
    m_current   -= m_block_size;
    m_block_pos += m_block_size;

}


bool  RBaseStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_file = fopen( filename, "rb" );
    
    if( m_file )
    {
        m_is_opened = true;
        SetPos(0);
    }
    return   NULL != m_file;
}

void  RBaseStream::Close()
{
    if( m_file )
    {
        fclose( m_file );
        m_file = NULL;
    }
    m_is_opened = false;
}


void  RBaseStream::Release()
{
    if( m_start )
    {
        delete[] (m_start - m_unGetsize);
    }
    m_start = m_end = m_current = NULL;
}



void  RBaseStream::SetPos( int pos )
{
    int offset = pos & (m_block_size - 1);
    int block_pos = pos - offset;
    
    assert( IsOpened() && pos >= 0 );
    
    if( m_current < m_end && block_pos == m_block_pos - m_block_size )
    {
        m_current = m_start + offset;
    }
    else
    {
        m_block_pos = block_pos;
        m_current = m_start + m_block_size + offset;
    }
}


int  RBaseStream::GetPos()
{
    assert( IsOpened() );
    return m_block_pos - m_block_size + (int)(m_current - m_start);
}

void  RBaseStream::Skip( int bytes )
{
    assert( bytes >= 0 );
    m_current += bytes;
}


/////////////////////////  RLByteStream ////////////////////////////

RLByteStream::~RLByteStream()
{
}

int  RLByteStream::GetByte()
{
    uchar *current = m_current;
    int   val;

    if( current >= m_end )
    {
        ReadBlock();
        current = m_current;
    }

    val = *((uchar*)current);
    m_current = current + 1;
    return val;
}


void  RLByteStream::GetBytes( void* buffer, int count, int* readed )
{
    uchar*  data = (uchar*)buffer;
    assert( count >= 0 );
    
    if( readed) *readed = 0;

    while( count > 0 )
    {
        int l;

        for(;;)
        {
            l = (int)(m_end - m_current);
            if( l > count ) l = count;
            if( l > 0 ) break;
            ReadBlock();
        }
        VOS_MEMCPY( data, m_current, l );
        m_current += l;
        data += l;
        count -= l;
        if( readed ) *readed += l;
    }
}

int  RLByteStream::GetWord()
{
    uchar *current = m_current;
    int   val;

    if( current+1 < m_end )
    {
        val = current[0] + (current[1] << 8);
        m_current = current + 2;
    }
    else
    {
        val = GetByte();
        val|= GetByte() << 8;
    }
    return val;
}

int  RLByteStream::GetDWord()
{
    uchar *current = m_current;
    int   val;

    if( current+3 < m_end )
    {
        val = current[0] + (current[1] << 8) +
              (current[2] << 16) + (current[3] << 24);
        m_current = current + 4;
    }
    else
    {
        val = GetByte();
        val |= GetByte() << 8;
        val |= GetByte() << 16;
        val |= GetByte() << 24;
    }
    return val;
}

WBaseStream::WBaseStream()
{
    m_start = m_end = m_current = NULL;
    m_file = 0;
    m_block_size = BS_DEF_BLOCK_SIZE;
    m_is_opened = false;
}

WBaseStream::~WBaseStream()
{
    Close();    // Close files
    Release();  // free  buffers
}

bool  WBaseStream::IsOpened()
{ 
    return m_is_opened;
}


void  WBaseStream::Allocate()
{
    if( !m_start )
        m_start = new uchar[m_block_size];

    m_end = m_start + m_block_size;
    m_current = m_start;
}


void  WBaseStream::WriteBlock()
{
    int size = (int)(m_current - m_start);
    assert(  NULL!=m_file  );

    //fseek( m_file, m_block_pos, SEEK_SET );
    fwrite( m_start, 1, size, m_file );
    m_current = m_start;

    
    m_block_pos += size;
}


bool  WBaseStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_file = fopen( filename, "wb" );
    
    if( m_file )
    {
        m_is_opened = true;
        m_block_pos = 0;
        m_current = m_start;
    }
    return   NULL!=m_file;
}


void  WBaseStream::Close()
{
    if( m_file )
    {
        WriteBlock();
        fclose( m_file );
        m_file = NULL;
    }
    m_is_opened = false;
}


void  WBaseStream::Release()
{
    if( m_start )
    {
        delete[] m_start;
    }
    m_start = m_end = m_current = 0;
}


int  WBaseStream::GetPos()
{
    assert( IsOpened() );
    return m_block_pos + (int)(m_current - m_start);
}


///////////////////////////// WLByteStream /////////////////////////////////// 

WLByteStream::~WLByteStream()
{
}

void WLByteStream::PutByte( int val )
{
    *m_current++ = (uchar)val;
    if( m_current >= m_end )
        WriteBlock();
}


void WLByteStream::PutBytes( const void* buffer, int count )
{
    uchar* data = (uchar*)buffer;
    
    assert( data && m_current && count >= 0 );

    while( count )
    {
        int l = (int)(m_end - m_current);
        
        if( l > count )
            l = count;
        
        if( l > 0 )
        {
            VOS_MEMCPY( m_current, data, l );
            m_current += l;
            data += l;
            count -= l;
        }
        if( m_current == m_end )
            WriteBlock();
    }
}


void WLByteStream::PutWord( int val )
{
    uchar *current = m_current;

    if( current+1 < m_end )
    {
        current[0] = (uchar)val;
        current[1] = (uchar)(val >> 8);
        m_current = current + 2;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val);
        PutByte(val >> 8);
    }
}


void WLByteStream::PutDWord( int val )
{
    uchar *current = m_current;

    if( current+3 < m_end )
    {
        current[0] = (uchar)val;
        current[1] = (uchar)(val >> 8);
        current[2] = (uchar)(val >> 16);
        current[3] = (uchar)(val >> 24);
        m_current = current + 4;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val);
        PutByte(val >> 8);
        PutByte(val >> 16);
        PutByte(val >> 24);
    }
}


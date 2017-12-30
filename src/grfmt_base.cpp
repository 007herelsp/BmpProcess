#include "_highgui.h"

#include "grfmt_base.h"
#include "bitstrm.h"


GrFmtReader::GrFmtReader( const char* filename )
{
    strncpy( m_filename, filename, sizeof(m_filename) - 1 );
    m_filename[sizeof(m_filename)-1] = '\0';
    m_width = m_height = 0;
    m_iscolor = false;
    m_bit_depth = 8;
    m_native_depth = false;
    m_isfloat = false;
}


GrFmtReader::~GrFmtReader()
{
    Close();
}


void GrFmtReader::Close()
{
    m_width = m_height = 0;
    m_iscolor = false;
}


GrFmtWriter::GrFmtWriter( const char* filename )
{
    strncpy( m_filename, filename, sizeof(m_filename) - 1 );
    m_filename[sizeof(m_filename)-1] = '\0';
}


bool  GrFmtWriter::IsFormatSupported( int depth )
{
    return depth == IPL_DEPTH_8U;
}

/* End of file. */


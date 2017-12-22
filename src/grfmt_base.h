
#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_


#include "utils.h"
#include "bitstrm.h"

#define  RBS_BAD_HEADER     -125  /* invalid image header */
#define  BAD_HEADER_ERR()   goto bad_header_exit

#ifndef _MAX_PATH
    #define _MAX_PATH    1024
#endif


///////////////////////////////// base class for readers ////////////////////////
class   GrFmtReader
{
public:
    
    GrFmtReader( const char* filename );
    virtual ~GrFmtReader();

    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    bool  IsColor()   { return m_iscolor; };
    int   GetDepth()  { return m_bit_depth; };
    void  UseNativeDepth( bool yes ) { m_native_depth = yes; };
    bool  IsFloat()   { return m_isfloat; };

    virtual bool  ReadHeader() = 0;
    virtual bool  ReadData( uchar* data, int step, int color ) = 0;
    virtual void  Close();

protected:

    bool    m_iscolor;
    int     m_width;    // width  of the image ( filled by ReadHeader )
    int     m_height;   // height of the image ( filled by ReadHeader )
    int     m_bit_depth;// bit depth per channel (normally 8)
    char    m_filename[_MAX_PATH]; // filename
    bool    m_native_depth;// use the native bit depth of the image
    bool    m_isfloat;  // is image saved as float or double?
};


///////////////////////////// base class for writers ////////////////////////////
class   GrFmtWriter
{
public:

    GrFmtWriter( const char* filename );
    virtual ~GrFmtWriter() {};
    virtual bool  IsFormatSupported( int depth );
    virtual bool  WriteImage( const uchar* data, int step,
                              int width, int height, int depth, int channels ) = 0;
protected:
    char    m_filename[_MAX_PATH]; // filename
};


////////////////////////////// base class for filter factories //////////////////
class   GrFmtFilterFactory
{
public:

    GrFmtFilterFactory();
    virtual ~GrFmtFilterFactory() {};

    const char*  GetDescription() { return m_description; };
    int     GetSignatureLength()  { return m_sign_len; };
    virtual bool CheckSignature( const char* signature );
    virtual bool CheckExtension( const char* filename );
    virtual GrFmtReader* NewReader( const char* filename ) = 0;
    virtual GrFmtWriter* NewWriter( const char* filename ) = 0;

protected:
    const char* m_description;
           // graphic format description in form:
           // <Some textual description>( *.<extension1> [; *.<extension2> ...]).
           // the textual description can not contain symbols '(', ')'
           // and may be, some others. It is safe to use letters, digits and spaces only.
           // e.g. "Targa (*.tga)",
           // or "Portable Graphic Format (*.pbm;*.pgm;*.ppm)"

    int          m_sign_len;    // length of the signature of the format
    const char*  m_signature;   // signature of the format
};


/////////////////////////// list of graphic format filters ///////////////////////////////

typedef void* ListPosition;

class   GrFmtFactoriesList
{
public:

    GrFmtFactoriesList();
    virtual ~GrFmtFactoriesList();
    void  RemoveAll();
    bool  AddFactory( GrFmtFilterFactory* factory );
    int   FactoriesCount() { return m_curFactories; };
    ListPosition  GetFirstFactoryPos();
    GrFmtFilterFactory*  GetNextFactory( ListPosition& pos );
    virtual GrFmtReader*  FindReader( const char* filename );
    virtual GrFmtWriter*  FindWriter( const char* filename );

protected:

    GrFmtFilterFactory** m_factories;
    int  m_maxFactories;
    int  m_curFactories;
};

#endif/*_GRFMT_BASE_H_*/

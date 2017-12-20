
#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_

#include "bitstrm.hpp"


namespace cv
{
	
	
	class BaseImageDecoder;
	class BaseImageEncoder;
	typedef Ptr<BaseImageEncoder> ImageEncoder;
	typedef Ptr<BaseImageDecoder> ImageDecoder;
	
	///////////////////////////////// base class for decoders ////////////////////////
	class BaseImageDecoder
	{
	public:
		BaseImageDecoder();
		virtual ~BaseImageDecoder() {};
	
		int width() const { return m_width; };
		int height() const { return m_height; };
		virtual int type() const { return m_type; };
	
		virtual bool setSource( const string& filename );
		virtual bool readHeader() = 0;
		virtual bool readData( Mat& img ) = 0;
	
		virtual size_t signatureLength() const;
		virtual bool checkSignature( const string& signature ) const;
		virtual ImageDecoder newDecoder() const;
	
	protected:
		int  m_width;  // width  of the image ( filled by readHeader )
		int  m_height; // height of the image ( filled by readHeader )
		int  m_type;
		string m_filename;
		string m_signature;
		Mat m_buf;
		bool m_buf_supported;
	};
	
	
	///////////////////////////// base class for encoders ////////////////////////////
	class BaseImageEncoder
	{
	public:
		BaseImageEncoder();
		virtual ~BaseImageEncoder() {};
		virtual bool isFormatSupported( int depth ) const;
	
		virtual bool setDestination( const string& filename );
		virtual bool write( const Mat& img, const vector<int>& params ) = 0;
	
		virtual string getDescription() const;
		virtual ImageEncoder newEncoder() const;
	
		virtual void throwOnEror() const;
	
	protected:
		string m_description;
	
		string m_filename;
		vector<uchar>* m_buf;
		bool m_buf_supported;
	
		string m_last_error;
	};
	
	int validateToInt(size_t step);
	
	struct PaletteEntry
	{
		unsigned char b, g, r, a;
	};
	
#define WRITE_PIX( ptr, clr )       \
		(((uchar*)(ptr))[0] = (clr).b,	\
		 ((uchar*)(ptr))[1] = (clr).g,	\
		 ((uchar*)(ptr))[2] = (clr).r)
	
#define  descale(x,n)  (((x) + (1 << ((n)-1))) >> (n))
#define  saturate(x)   (uchar)(((x) & ~255) == 0 ? (x) : ~((x)>>31))
	
	void icvCvt_BGR2Gray_8u_C3C1R( const uchar* bgr, int bgr_step,
								   uchar* gray, int gray_step,
								   CvSize size, int swap_rb=0 );
	
	void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );
	void  CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries );
	


enum BmpCompression
{
    BMP_RGB = 0,
    BMP_RLE8 = 1,
    BMP_RLE4 = 2,
    BMP_BITFIELDS = 3
};


// Windows Bitmap reader
class BmpDecoder : public BaseImageDecoder
{
public:

    BmpDecoder();
    ~BmpDecoder();

    bool  readData( Mat& img );
    bool  readHeader();
    void  close();

    ImageDecoder newDecoder() const;

protected:

    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_origin;
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
};


// ... writer
class BmpEncoder : public BaseImageEncoder
{
public:
    BmpEncoder();
    ~BmpEncoder();

    bool  write( const Mat& img, const vector<int>& params );

    ImageEncoder newEncoder() const;
};


}





#endif/*_GRFMT_BMP_H_*/

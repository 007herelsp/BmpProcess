#include "highgui/precomp.hpp"
#include "grfmt_bmp.hpp"

namespace cv
{

static const char *fmtSignBmp = "BM";

/************************ BMP decoder *****************************/

BmpDecoder::BmpDecoder()
{
    m_signature = fmtSignBmp;
    m_offset = -1;
    m_buf_supported = true;
}

BmpDecoder::~BmpDecoder()
{
}

void BmpDecoder::close()
{
    m_strm.close();
}

ImageDecoder BmpDecoder::newDecoder() const
{
    return new BmpDecoder;
}

bool BmpDecoder::readHeader()
{
    bool result = false;
    bool iscolor = false;

    if (!m_buf.empty())
    {
        if (!m_strm.open(m_buf))
            return false;
    }
    else if (!m_strm.open(m_filename))
        return false;

    try
    {
        m_strm.skip(10);
        m_offset = m_strm.getDWord();

        int size = m_strm.getDWord();

        if (size >= 36)
        {
            m_width = m_strm.getDWord();
            m_height = m_strm.getDWord();
            m_bpp = m_strm.getDWord() >> 16;
            m_rle_code = (BmpCompression)m_strm.getDWord();
            m_strm.skip(12);
            int clrused = m_strm.getDWord();
            m_strm.skip(size - 36);

            if (m_width > 0 && m_height != 0 &&
                (m_bpp == 24))
            {
                iscolor = true;
                result = true;
            }
        }
        else if (size == 12)
        {
            m_width = m_strm.getWord();
            m_height = m_strm.getWord();
            m_bpp = m_strm.getDWord() >> 16;
            m_rle_code = BMP_RGB;

            if (m_width > 0 && m_height != 0 &&
                (m_bpp == 24))
            {
                result = true;
            }
        }
    }
    catch (...)
    {
        throw;
    }

    m_type = iscolor ? CV_8UC3 : CV_8UC1;
    m_origin = m_height > 0 ? IPL_ORIGIN_BL : IPL_ORIGIN_TL;
    m_height = std::abs(m_height);

    if (!result)
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.close();
    }
    return result;
}

bool BmpDecoder::readData(Mat &img)
{
    uchar *data = img.data;
    int step = validateToInt(img.step);
    bool color = img.channels() > 1;
    uchar gray_palette[256];
    bool result = false;
    int src_pitch = ((m_width * (m_bpp != 15 ? m_bpp : 16) + 7) / 8 + 3) & -4;
    int nch = color ? 3 : 1;
    int y, width3 = m_width * nch;

    if (m_offset < 0 || !m_strm.isOpened())
        return false;

    if (m_origin == IPL_ORIGIN_BL)
    {
        data += (m_height - 1) * (size_t)step;
        step = -step;
    }

    AutoBuffer<uchar> _src, _bgr;
    _src.allocate(src_pitch + 32);

    if (!color)
    {
        if (m_bpp <= 8)
        {
            CvtPaletteToGray(m_palette, gray_palette, 1 << m_bpp);
        }
        _bgr.allocate(m_width * 3 + 32);
    }
    uchar *src = _src;

    try
    {
        m_strm.setPos(m_offset);

        switch (m_bpp)
        {
        case 24:
            for (y = 0; y < m_height; y++, data += step)
            {
                m_strm.getBytes(src, src_pitch);
                if (!color)
                    icvCvt_BGR2Gray_8u_C3C1R(src, 0, data, 0, cvSize(m_width, 1));
                else
                    memcpy(data, src, m_width * 3);
            }
            result = true;
            break;

        default:
            CV_Error(CV_StsError, "Invalid/unsupported mode");
        }
    }
    catch (...)
    {
        throw;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////

BmpEncoder::BmpEncoder()
{
    m_description = "Windows bitmap (*.bmp;*.dib)";
    m_buf_supported = true;
}

BmpEncoder::~BmpEncoder()
{
}

ImageEncoder BmpEncoder::newEncoder() const
{
    return new BmpEncoder;
}

bool BmpEncoder::write(const Mat &img, const vector<int> &)
{
    int width = img.cols, height = img.rows, channels = img.channels();
    int fileStep = (width * channels + 3) & -4;
    uchar zeropad[] = "\0\0\0\0";
    WLByteStream strm;

    if (m_buf)
    {
        if (!strm.open(*m_buf))
            return false;
    }
    else if (!strm.open(m_filename))
        return false;

    int bitmapHeaderSize = 40;
    int paletteSize = channels > 1 ? 0 : 1024;
    int headerSize = 14 /* fileheader */ + bitmapHeaderSize + paletteSize;
    size_t fileSize = (size_t)fileStep * height + headerSize;
    PaletteEntry palette[256];

    if (m_buf)
        m_buf->reserve(alignSize(fileSize + 16, 256));

    // write signature 'BM'
    strm.putBytes(fmtSignBmp, (int)strlen(fmtSignBmp));

    // write file header
    strm.putDWord(validateToInt(fileSize)); // file size
    strm.putDWord(0);
    strm.putDWord(headerSize);

    // write bitmap header
    strm.putDWord(bitmapHeaderSize);
    strm.putDWord(width);
    strm.putDWord(height);
    strm.putWord(1);
    strm.putWord(channels << 3);
    strm.putDWord(BMP_RGB);
    strm.putDWord(0);
    strm.putDWord(0);
    strm.putDWord(0);
    strm.putDWord(0);
    strm.putDWord(0);

    if (channels == 1)
    {
        FillGrayPalette(palette, 8);
        strm.putBytes(palette, sizeof(palette));
    }

    width *= channels;
    for (int y = height - 1; y >= 0; y--)
    {
        strm.putBytes(img.data + img.step * y, width);
        if (fileStep > width)
            strm.putBytes(zeropad, fileStep - width);
    }

    strm.close();
    return true;
}
}

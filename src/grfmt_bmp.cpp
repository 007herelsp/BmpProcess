#include "highgui.h"
#include "grfmt_bmp.h"

static const char *fmtSignBmp = "BM";



static void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative=false)
{
    int i, length = 1 << bpp;
    int xor_mask = negative ? 255 : 0;

    for( i = 0; i < length; i++ )
    {
        int val = (i * 255/(length - 1)) ^ xor_mask;
        palette[i].b = palette[i].g = palette[i].r = (uchar)val;
        palette[i].a = 0;
    }
}



/************************ BMP reader *****************************/

GrFmtBmpReader::GrFmtBmpReader(const char *filename) : GrFmtReader(filename)
{
    m_offset = -1;
}

GrFmtBmpReader::~GrFmtBmpReader()
{
}

void GrFmtBmpReader::Close()
{
    m_strm.Close();
    GrFmtReader::Close();
}

bool GrFmtBmpReader::ReadHeader()
{
    bool result = false;

    assert(strlen(m_filename) != 0);
    if (!m_strm.Open(m_filename))
        return false;

    {
        m_strm.Skip(10);
        m_offset = m_strm.GetDWord();

        int size = m_strm.GetDWord();

        if (size >= 36)
        {
            m_width = m_strm.GetDWord();
            m_height = m_strm.GetDWord();
            m_bpp = m_strm.GetDWord() >> 16;
            m_rle_code = (BmpCompression)m_strm.GetDWord();
            m_strm.Skip(12);
            int clrused = m_strm.GetDWord();
            m_strm.Skip(size - 36);

            if (m_width > 0 && m_height > 0 &&
                ((m_bpp == 24) && (m_rle_code == BMP_RGB)))
            {
                m_iscolor = true;
                result = true;
            }
        }
        else if (size == 12)
        {
            m_width = m_strm.GetWord();
            m_height = m_strm.GetWord();
            m_bpp = m_strm.GetDWord() >> 16;
            m_rle_code = BMP_RGB;

            if (m_width > 0 && m_height > 0 &&                (m_bpp == 24))
            {
                result = true;
            }
        }
    }

    if (!result)
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }
    return result;
}

bool GrFmtBmpReader::ReadData(uchar *data, int step, int color)
{
    const int buffer_size = 1 << 12;
    uchar buffer[buffer_size];
    bool result = false;
    uchar *src = buffer;
    int src_pitch = ((m_width * (m_bpp != 15 ? m_bpp : 16) + 7) / 8 + 3) & -4;
    int y;

    if (m_offset < 0 || !m_strm.IsOpened())
        return false;

    data += (m_height - 1) * step;
    step = -step;
    assert("herelsp remove" && (m_bpp == 24));
    if (24 != m_bpp || (!color))
    {
        return false;
    }


    {
        m_strm.SetPos(m_offset);

        /************************* 24 BPP ************************/
        for (y = 0; y < m_height; y++, data += step)
        {
            m_strm.GetBytes(color ? data : src, src_pitch);
        }
        result = true;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////

GrFmtBmpWriter::GrFmtBmpWriter(const char *filename) : GrFmtWriter(filename)
{
}

GrFmtBmpWriter::~GrFmtBmpWriter()
{
}

bool GrFmtBmpWriter::WriteImage(const uchar *data, int step,
                                int width, int height, int /*depth*/, int channels)
{
    bool result = false;
    int fileStep = (width * channels + 3) & -4;
    uchar zeropad[] = "\0\0\0\0";

    assert(data && width > 0 && height > 0 && step >= fileStep);

    if (m_strm.Open(m_filename))
    {
        int bitmapHeaderSize = 40;
        int paletteSize = channels > 1 ? 0 : 1024;
        int headerSize = 14 /* fileheader */ + bitmapHeaderSize + paletteSize;
        PaletteEntry palette[256];

        // write signature 'BM'
        m_strm.PutBytes(fmtSignBmp, (int)strlen(fmtSignBmp));

        // write file header
        m_strm.PutDWord(fileStep * height + headerSize); // file size
        m_strm.PutDWord(0);
        m_strm.PutDWord(headerSize);

        // write bitmap header
        m_strm.PutDWord(bitmapHeaderSize);
        m_strm.PutDWord(width);
        m_strm.PutDWord(height);
        m_strm.PutWord(1);
        m_strm.PutWord(channels << 3);
        m_strm.PutDWord(BMP_RGB);
        m_strm.PutDWord(0);
        m_strm.PutDWord(0);
        m_strm.PutDWord(0);
        m_strm.PutDWord(0);
        m_strm.PutDWord(0);

        if (channels == 1)
        {
            FillGrayPalette(palette, 8);
            m_strm.PutBytes(palette, sizeof(palette));
        }

        width *= channels;
        data += step * (height - 1);
        for (; height--; data -= step)
        {
            m_strm.PutBytes(data, width);
            if (fileStep > width)
                m_strm.PutBytes(zeropad, fileStep - width);
        }

        m_strm.Close();
        result = true;
    }
    return result;
}

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "_highgui.h"
#include "grfmt_bmp.h"

static const char *fmtSignBmp = "BM";

GrFmtBmp::GrFmtBmp()
{
    m_sign_len = 2;
    m_signature = fmtSignBmp;
    m_description = "Windows bitmap (*.bmp;*.dib)";
}

GrFmtBmp::~GrFmtBmp()
{
}

GrFmtReader *GrFmtBmp::NewReader(const char *filename)
{
    return new GrFmtBmpReader(filename);
}

GrFmtWriter *GrFmtBmp::NewWriter(const char *filename)
{
    return new GrFmtBmpWriter(filename);
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

    if (setjmp(m_strm.JmpBuf()) == 0)
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
                (((m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                   m_bpp == 24 || m_bpp == 32) &&
                  m_rle_code == BMP_RGB) ||
                 (m_bpp == 16 && (m_rle_code == BMP_RGB || m_rle_code == BMP_BITFIELDS)) ||
                 (m_bpp == 4 && m_rle_code == BMP_RLE4) ||
                 (m_bpp == 8 && m_rle_code == BMP_RLE8)))
            {
                m_iscolor = true;
                result = true;

                if (m_bpp <= 8)
                {
                    memset(m_palette, 0, sizeof(m_palette));
                    m_strm.GetBytes(m_palette, (clrused == 0 ? 1 << m_bpp : clrused) * 4);
                    m_iscolor = IsColorPalette(m_palette, m_bpp);
                }
                else if (m_bpp == 16 && m_rle_code == BMP_BITFIELDS)
                {
                    int redmask = m_strm.GetDWord();
                    int greenmask = m_strm.GetDWord();
                    int bluemask = m_strm.GetDWord();

                    if (bluemask == 0x1f && greenmask == 0x3e0 && redmask == 0x7c00)
                        m_bpp = 15;
                    else if (bluemask == 0x1f && greenmask == 0x7e0 && redmask == 0xf800)
                        ;
                    else
                        result = false;
                }
                else if (m_bpp == 16 && m_rle_code == BMP_RGB)
                    m_bpp = 15;
            }
        }
        else if (size == 12)
        {
            m_width = m_strm.GetWord();
            m_height = m_strm.GetWord();
            m_bpp = m_strm.GetDWord() >> 16;
            m_rle_code = BMP_RGB;

            if (m_width > 0 && m_height > 0 &&
                (m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                 m_bpp == 24 || m_bpp == 32))
            {
                if (m_bpp <= 8)
                {
                    uchar buffer[256 * 3];
                    int j, clrused = 1 << m_bpp;
                    m_strm.GetBytes(buffer, clrused * 3);
                    for (j = 0; j < clrused; j++)
                    {
                        m_palette[j].b = buffer[3 * j + 0];
                        m_palette[j].g = buffer[3 * j + 1];
                        m_palette[j].r = buffer[3 * j + 2];
                    }
                }
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
    uchar bgr_buffer[buffer_size];
    bool result = false;
    uchar *src = buffer;
    int src_pitch = ((m_width * (m_bpp != 15 ? m_bpp : 16) + 7) / 8 + 3) & -4;
    int nch = color ? 3 : 1;
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

    if (setjmp(m_strm.JmpBuf()) == 0)
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

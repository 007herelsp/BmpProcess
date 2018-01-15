
#include "process.h"
#include "misc.h"

static void Sobel(const AutoBuffer *src, AutoBuffer *dst, int dx, int dy, int aperture_size)
{
    SepFilter filter;

    VOS_FUNCNAME("Sobel");

    __BEGIN__;

    int src_type, dst_type;

    src_type = VOS_BUFFER_TYPE(src->type);
    dst_type = VOS_BUFFER_TYPE(dst->type);

    if (!VOS_ARE_SIZES_EQ(src, dst))
        VOS_ERROR(VOS_StsBadArg, "");

    VOS_CALL(filter.init_deriv(src->cols, src_type, dst_type, dx, dy,
                               aperture_size, 0));
    VOS_CALL(filter.process(src, dst));

    __END__;
}

#define CANNY_SHIFT 15
static const int TG22 = (int)(0.4142135623730950488016887242097 * (1 << CANNY_SHIFT) + 0.5);

// 仿照matlab，自适应求高低两个门限
void CannyAdaptiveFindThreshold(AutoBuffer *dx, AutoBuffer *dy, double *low, double *high)
{
    Size size;
    int i, j;
    int hist_size = 255;
    double PercentOfPixelsNotEdges = 0.7;
    size = GetSize(dx);
    BmpImage *imge = CreateImage(size, SYS_DEPTH_32F, 1);

    // 计算边缘的强度, 并存于图像中
    float maxv = 0;
    float minv = 0;
    for (i = 0; i < size.height; i++)
    {
        const short *_dx = (short *)(dx->data.ptr + dx->step * i);
        const short *_dy = (short *)(dy->data.ptr + dy->step * i);
        float *_image = (float *)(imge->imageData + imge->widthStep * i);
        for (j = 0; j < size.width; j++)
        {
            _image[j] = (float)(abs(_dx[j]) + abs(_dy[j]));
            maxv = maxv < _image[j] ? _image[j] : maxv;
            minv = minv > _image[j] ? _image[j] : minv;
        }
    }
    if (maxv == 0 || (maxv == minv))
    {
        *high = 1;
        *low = 3;
        ReleaseImage(&imge);
        return;
    }
    float t = 255 / (maxv - minv);
    float range0 = t;
    float range1 = -t * minv;
    // 计算直方图
    hist_size = (int)(hist_size > maxv ? maxv : hist_size);

    double bins[256] = {0};
    float *img = (float *)imge->imageData;
    int sz = 255;
    int x;
    size.width *= size.height;
    size.height = 1;
    static int step = VOS_STUB_STEP / 4;

    for (; size.height--; img += step)
    {
        float *ptr = img;
        for (x = 0; x <= size.width - 4; x += 4)
        {
            int v0 = SysFloor(ptr[x] * range0 + range1);
            int v1 = SysFloor(ptr[x + 1] * range0 + range1);

            if ((unsigned)v0 < (unsigned)sz)
                bins[v0]++;
            if ((unsigned)v1 < (unsigned)sz)
                bins[v1]++;

            v0 = SysFloor(ptr[x + 2] * range0 + range1);
            v1 = SysFloor(ptr[x + 3] * range0 + range1);

            if ((unsigned)v0 < (unsigned)sz)
                bins[v0]++;
            if ((unsigned)v1 < (unsigned)sz)
                bins[v1]++;
        }

        for (; x < size.width; x++)
        {
            int v0 = SysFloor(ptr[x] * range0 + range1);
            if ((unsigned)v0 < (unsigned)sz)
                bins[v0]++;
        }
    }

    int total = (int)(imge->height * imge->width * PercentOfPixelsNotEdges);
    double sum = 0;

    sum = 0;
    for (i = 0; i < hist_size; i++)
    {
        sum += bins[i];
        if (sum > total)
            break;
    }
    // 计算高低门限
    *high = (i + 1) * maxv / hist_size;
    *low = *high * 0.4;
    ReleaseImage(&imge);
}

void Canny(const void *srcarr, void *dstarr,
           double low_thresh, double high_thresh, int aperture_size)
{
    AutoBuffer *dx = NULL, *dy = NULL;
    void *buffer = NULL;
    uchar **stack_top, **stack_bottom = NULL;

    VOS_FUNCNAME("Canny");

    __BEGIN__;

    AutoBuffer srcstub, *src = (AutoBuffer *)srcarr;
    AutoBuffer dststub, *dst = (AutoBuffer *)dstarr;
    Size size;
    int flags = aperture_size;
    int low, high;
    int *mag_buf[3];
    uchar *map;
    int mapstep, maxsize;
    int i, j;
    AutoBuffer mag_row;

    VOS_CALL(src = GetAutoBuffer(src, &srcstub));
    VOS_CALL(dst = GetAutoBuffer(dst, &dststub));

    if (VOS_BUFFER_TYPE(src->type) != VOS_8UC1 ||
            VOS_BUFFER_TYPE(dst->type) != VOS_8UC1)
        VOS_ERROR(VOS_StsUnsupportedFormat, "");

    aperture_size &= INT_MAX;
    if ((aperture_size & 1) == 0 || aperture_size < 3 || aperture_size > 7)
        VOS_ERROR(VOS_StsBadFlag, "");

    size = GetBufferSize(src);

    dx = CreateAutoBuffer(size.height, size.width, VOS_16SC1);
    dy = CreateAutoBuffer(size.height, size.width, VOS_16SC1);
    Sobel(src, dx, 1, 0, aperture_size);
    Sobel(src, dy, 0, 1, aperture_size);

    ///////////////////

    CannyAdaptiveFindThreshold(dx, dy, &low_thresh, &high_thresh);
    if (low_thresh > high_thresh)
    {
        double t;
        VOS_SWAP(low_thresh, high_thresh, t);
    }
    //printf("%g,%g\n", low_thresh, high_thresh);
    ///////////////////

    if (flags & VOS_CANNY_L2_GRADIENT)
    {
        Sys32suf ul, uh;
        ul.f = (float)low_thresh;
        uh.f = (float)high_thresh;

        low = ul.i;
        high = uh.i;
    }
    else
    {
        low = SysFloor(low_thresh);
        high = SysFloor(high_thresh);
    }

    VOS_CALL(buffer = SysAlloc((size.width + 2) * (size.height + 2) +
                               (size.width + 2) * 3 * sizeof(int)));

    mag_buf[0] = (int *)buffer;
    mag_buf[1] = mag_buf[0] + size.width + 2;
    mag_buf[2] = mag_buf[1] + size.width + 2;
    map = (uchar *)(mag_buf[2] + size.width + 2);
    mapstep = size.width + 2;

    maxsize = VOS_MAX(1 << 10, size.width * size.height / 10);
    VOS_CALL(stack_top = stack_bottom = (uchar **)SysAlloc(maxsize * sizeof(stack_top[0])));

    VOS_MEMSET(mag_buf[0], 0, (size.width + 2) * sizeof(int));
    VOS_MEMSET(map, 1, mapstep);
    VOS_MEMSET(map + mapstep * (size.height + 1), 1, mapstep);

    /* sector numbers
       (Top-Left Origin)

        1   2   3
         *  *  *
          * * *
        0*******0
          * * *
         *  *  *
        3   2   1
    */

#define CANNY_PUSH(d) *(d) = (uchar)2, *stack_top++ = (d)
#define CANNY_POP(d) (d) = *--stack_top

    mag_row = InitAutoBuffer(1, size.width, VOS_32F);

    for (i = 0; i <= size.height; i++)
    {
        int *_mag = mag_buf[(i > 0) + 1] + 1;
        float *_magf = (float *)_mag;
        const short *_dx = (short *)(dx->data.ptr + dx->step * i);
        const short *_dy = (short *)(dy->data.ptr + dy->step * i);
        uchar *_map;
        int x, y;
        int magstep1, magstep2;
        int prev_flag = 0;

        if (i < size.height)
        {
            _mag[-1] = _mag[size.width] = 0;

            if (!(flags & VOS_CANNY_L2_GRADIENT))
            {
                for (j = 0; j < size.width; j++)
                    _mag[j] = abs(_dx[j]) + abs(_dy[j]);
            }
            else
            {
                for (j = 0; j < size.width; j++)
                {
                    x = _dx[j];
                    y = _dy[j];
                    _magf[j] = (float)sqrt((double)x * x + (double)y * y);
                }
            }
        }
        else
            VOS_MEMSET(_mag - 1, 0, (size.width + 2) * sizeof(int));

        if (0 == i)
            continue;

        _map = map + mapstep * i + 1;
        _map[-1] = _map[size.width] = 1;

        _mag = mag_buf[1] + 1; // take the central row
        _dx = (short *)(dx->data.ptr + dx->step * (i - 1));
        _dy = (short *)(dy->data.ptr + dy->step * (i - 1));

        magstep1 = (int)(mag_buf[2] - mag_buf[1]);
        magstep2 = (int)(mag_buf[0] - mag_buf[1]);

        if ((stack_top - stack_bottom) + size.width > maxsize)
        {
            uchar **new_stack_bottom;
            maxsize = VOS_MAX(maxsize * 3 / 2, maxsize + size.width);
            VOS_CALL(new_stack_bottom = (uchar **)SysAlloc(maxsize * sizeof(stack_top[0])));
            VOS_MEMCPY(new_stack_bottom, stack_bottom, (stack_top - stack_bottom) * sizeof(stack_top[0]));
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            SYS_FREE(&stack_bottom);
            stack_bottom = new_stack_bottom;
        }

        for (j = 0; j < size.width; j++)
        {

            x = _dx[j];
            y = _dy[j];
            int s = x ^ y;
            int m = _mag[j];

            x = abs(x);
            y = abs(y);
            if (m > low)
            {
                int tg22x = x * TG22;
                int tg67x = tg22x + ((x + x) << CANNY_SHIFT);

                y <<= CANNY_SHIFT;

                if (y < tg22x)
                {
                    if (m > _mag[j - 1] && m >= _mag[j + 1])
                    {
                        if (m > high && !prev_flag && _map[j - mapstep] != 2)
                        {
                            CANNY_PUSH(_map + j);
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else if (y > tg67x)
                {
                    if (m > _mag[j + magstep2] && m >= _mag[j + magstep1])
                    {
                        if (m > high && !prev_flag && _map[j - mapstep] != 2)
                        {
                            CANNY_PUSH(_map + j);
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else
                {
                    s = s < 0 ? -1 : 1;
                    if (m > _mag[j + magstep2 - s] && m > _mag[j + magstep1 + s])
                    {
                        if (m > high && !prev_flag && _map[j - mapstep] != 2)
                        {
                            CANNY_PUSH(_map + j);
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
            }
            prev_flag = 0;
            _map[j] = (uchar)1;
        }

        // scroll the ring buffer
        _mag = mag_buf[0];
        mag_buf[0] = mag_buf[1];
        mag_buf[1] = mag_buf[2];
        mag_buf[2] = _mag;
    }

    // now track the edges (hysteresis thresholding)
    while (stack_top > stack_bottom)
    {
        uchar *m;
        if ((stack_top - stack_bottom) + 8 > maxsize)
        {
            uchar **new_stack_bottom;
            maxsize = VOS_MAX(maxsize * 3 / 2, maxsize + 8);
            VOS_CALL(new_stack_bottom = (uchar **)SysAlloc(maxsize * sizeof(stack_top[0])));
            VOS_MEMCPY(new_stack_bottom, stack_bottom, (stack_top - stack_bottom) * sizeof(stack_top[0]));
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            SYS_FREE(&stack_bottom);
            stack_bottom = new_stack_bottom;
        }

        CANNY_POP(m);

        if (!m[-1])
            CANNY_PUSH(m - 1);
        if (!m[1])
            CANNY_PUSH(m + 1);
        if (!m[-mapstep - 1])
            CANNY_PUSH(m - mapstep - 1);
        if (!m[-mapstep])
            CANNY_PUSH(m - mapstep);
        if (!m[-mapstep + 1])
            CANNY_PUSH(m - mapstep + 1);
        if (!m[mapstep - 1])
            CANNY_PUSH(m + mapstep - 1);
        if (!m[mapstep])
            CANNY_PUSH(m + mapstep);
        if (!m[mapstep + 1])
            CANNY_PUSH(m + mapstep + 1);
    }

    // the final pass, form the final image
    for (i = 0; i < size.height; i++)
    {
        const uchar *_map = map + mapstep * (i + 1) + 1;
        uchar *_dst = dst->data.ptr + dst->step * i;

        for (j = 0; j < size.width; j++)
            _dst[j] = (uchar) - (_map[j] >> 1);
    }

    __END__;

    ReleaseAutoBuffer(&dx);
    ReleaseAutoBuffer(&dy);
    SYS_FREE(&buffer);
    SYS_FREE(&stack_bottom);
}

/* End of file. */

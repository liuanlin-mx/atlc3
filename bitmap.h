#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"


struct bitmap_file_head_struct
{
    std::uint8_t zz_magic[2];    /* 00 "BM" */
    std::int32_t bf_size;       /* 02 */
    std::int32_t zz_hot_x;      /* 06 */
    std::int32_t zz_hot_y;      /* 08 */
    std::int32_t bf_offs;       /* 0A */
    std::int32_t bi_size;       /* 0E */
};

struct bitmap_head_struct
{
    std::int32_t bi_width;      /* 12 */
    std::int32_t bi_height;     /* 16 */
    std::int32_t bi_planes;     /* 1A */
    std::int32_t bi_bit_cnt;    /* 1C */
    std::int32_t bi_compr;      /* 1E */
    std::int32_t bi_size_im;    /* 22 */
    std::int32_t bi_x_pels;     /* 26 */
    std::int32_t bi_y_pels;     /* 2A */
    std::int32_t bi_clr_used;   /* 2E */
    std::int32_t bi_clr_imp;    /* 32 */
                                /* 36 */
};

#pragma pack(1)
struct rgb
{
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;
};
#pragma pack()

typedef matrix<rgb> matrix_rgb;

#if 0
struct bitmap_file_head
{
    std::uint16_t magic; // "BM"，即0x4d42
    std::uint32_t bf_size; //文件大小 
    std::uint16_t reserved1; //保留字，不考虑 
    std::uint16_t reserved2; //保留字，不考虑
    std::uint32_t bf_offset; //实际位图数据的偏移字节数，即前三个部分长度之和
};

struct bitmap_head
{
    std::uint32_t bi_size; //指定此结构体的长度，为40
    std::int32_t bi_width; //位图宽
    std::int32_t bi_height; //位图高
    std::uint16_t bi_planes; //平面数，为1
    std::uint16_t bi_bit_count; //采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
    std::uint32_t bi_compression; //压缩方式，可以是0，1，2，其中0表示不压缩
    std::uint32_t bi_size_image; //实际位图数据占用的字节数
    std::int32_t bi_x_pels; //X方向分辨率
    std::int32_t bi_y_pels; //Y方向分辨率
    std::uint32_t bi_clr_used; //使用的颜色数，如果为0，则表示默认值(2^颜色位数)
    std::uint32_t bi_clr_important; //重要颜色数，如果为0，则表示所有颜色都是重要的
};
#endif

bool bitmap_read_file_headers(FILE *fp, std::uint32_t& offset, std::uint32_t& size, std::uint32_t& width, std::uint32_t& height);
bool bitmap_read_data(FILE *fp, void *buf, std::uint32_t size);
bool bitmap_read(const char *filename, matrix_rgb& img);
bool bitmap_write(const char *filename, matrix_rgb& img);
#endif
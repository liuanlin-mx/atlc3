#include <string.h>
#include "bitmap.h"

#define BITMAP_HEADER_SIZE 0x36 /* 54 */

bool bitmap_read_file_headers(FILE *fp, std::uint32_t& offset, std::uint32_t& size, std::uint32_t& width, std::uint32_t& height)
{
    std::uint8_t bmp_buff[BITMAP_HEADER_SIZE] = {0};
    
    bitmap_file_head_struct bitmap_file_head;
    bitmap_head_struct bitmap_head;
    int colormap_size;
    
    /* Read the .bmp file header into a bmp_buff */
    if (BITMAP_HEADER_SIZE != fread(bmp_buff, 1, BITMAP_HEADER_SIZE, fp) || (strncmp((char *)bmp_buff, "BM", 2)))
    {
        fprintf(stderr, "is not a valid BMP file\n");
        return false;
    }

    /* On most machines, sizeof(short)==2 and sizeof(int)==4. This is so no matter
    if the machine is 32 or 64 bis. An exception is the Cray Y-MP, which has
    sizeof(short)=8
    sizeof(int)=8
    sizeof(long)=8.

    In this case, it is much more difficult to write the header for the bitmap. But in
    the aid of portability, this is done. So these is a section of code that will work
    even if sizeof(short)=8 and sizeof(int)=8. See below for that. */


    /* Read the bmp_buff into the two structures we want */

    bitmap_file_head.zz_magic[0x0] = bmp_buff[0];
    bitmap_file_head.zz_magic[0x1] = bmp_buff[1];
    bitmap_file_head.bf_size = bmp_buff[0x2] + ((bmp_buff[3] + ((bmp_buff[4] + (bmp_buff[5] << 8)) << 8)) << 8);
    bitmap_file_head.zz_hot_x = bmp_buff[0x6] + (bmp_buff[7] << 8);
    bitmap_file_head.zz_hot_y = bmp_buff[0x8] + (bmp_buff[0x09] << 8);
    bitmap_file_head.bf_offs = bmp_buff[0x0a] + ((bmp_buff[0xb] + ((bmp_buff[0xc] + (bmp_buff[0x0d] << 8)) << 8)) << 8);
    bitmap_file_head.bi_size = bmp_buff[0x0E] + ((bmp_buff[0x0f] + ((bmp_buff[0x10] + (bmp_buff[0x11] << 8)) << 8)) << 8);
    
    bitmap_head.bi_width = bmp_buff[0x12] + ((bmp_buff[0x13] + ((bmp_buff[0x14] + (bmp_buff[0x15] << 8)) << 8)) << 8);
    bitmap_head.bi_height = bmp_buff[0x16] + ((bmp_buff[0x17] + ((bmp_buff[0x18] + (bmp_buff[0x19] << 8)) << 8)) << 8);
    bitmap_head.bi_planes = bmp_buff[0x1A] + (bmp_buff[0x1b] << 8);
    bitmap_head.bi_bit_cnt = bmp_buff[0x1C] + (bmp_buff[0x1d] << 8);
    bitmap_head.bi_compr = bmp_buff[0x1E] + ((bmp_buff[0x1f] + ((bmp_buff[0x20] + (bmp_buff[0x21] << 8)) << 8)) << 8);
    bitmap_head.bi_size_im = bmp_buff[0x22] + ((bmp_buff[0x23] + ((bmp_buff[0x24] + (bmp_buff[0x25] << 8)) << 8)) << 8);
    /* I thought the length of the image was always stored in Bitmap_Head.biSizeIm, but
    this appears not to be so. Hence it is now calculated from the length of the file
    */
    //bitmap_head.bi_size_im = length_of_file - BITMAP_HEADER_SIZE;
    bitmap_head.bi_x_pels = bmp_buff[0x26] + ((bmp_buff[0x27] + ((bmp_buff[0x28] + (bmp_buff[0x29] << 8)) << 8)) <<8);
    bitmap_head.bi_y_pels = bmp_buff[0x2A] + ((bmp_buff[0x2b] + ((bmp_buff[0x2c] + (bmp_buff[0x2d] << 8)) << 8)) <<8);
    bitmap_head.bi_clr_used = bmp_buff[0x2E] + ((bmp_buff[0x2f] + ((bmp_buff[0x30] + (bmp_buff[0x31] << 8)) << 8)) <<8);
    bitmap_head.bi_clr_imp  = bmp_buff[0x32] + ((bmp_buff[0x33] + ((bmp_buff[0x34] + (bmp_buff[0x35] << 8)) << 8)) <<8);

    if (bitmap_head.bi_bit_cnt != 24)
    {
        fprintf(stderr, "Sorry, the .bmp bitmap must have 24 bits per colour,\n");
        fprintf(stderr, "but it has %d bits. Resave the \n", bitmap_head.bi_bit_cnt);
        fprintf(stderr, "image using 24-bit colour\n");
        return false;
    }
    colormap_size = (bitmap_file_head.bf_offs - bitmap_file_head.bi_size - 14) / 4;

    if ((bitmap_head.bi_clr_used == 0) && (bitmap_head.bi_bit_cnt <= 8))
        bitmap_head.bi_clr_used = colormap_size;

    /* Sanity checks */

    if (bitmap_head.bi_height == 0 || bitmap_head.bi_width == 0)
    {
        fprintf(stderr, "error reading BMP file header - width or height is zero\n");
        return false;
    }
    
    if (bitmap_head.bi_planes != 1)
    {
        fprintf(stderr,"error reading BMP file header - bitplanes not equal to 1\n");
        return false;
    }
    
    if (colormap_size > 256 || bitmap_head.bi_clr_used > 256)
    {
        fprintf(stderr,"error reading BMP file header - colourmap size error\n");
        return false;
    }
    
    /* Windows and OS/2 declare filler so that rows are a multiple of
       word length (32 bits == 4 bytes)
    */

    width = bitmap_head.bi_width;
    height = bitmap_head.bi_height;
    offset = bitmap_file_head.bf_offs;
    size = bitmap_head.bi_size_im;
    if (size == 0)
    {
        size = bitmap_file_head.bf_size - offset;
    }
    if(size < 3 * (width) * (height))
    {
        fprintf(stderr,"Internal error in bitmap.c\n");
        return false;
    }
    
    return true;
}


bool bitmap_read_data(FILE *fp, void *buf, std::uint32_t size)
{
    std::uint32_t rlen = fread(buf, 1, size, fp);
    return rlen == size;
}


bool bitmap_read(const char *filename, matrix_rgb& img)
{
    FILE *fp;
    if (filename)
    {
        fp = fopen(filename, "rb");
    }
    else
    {
        fp = stdin;
    }
    
    if (fp == NULL)
    {
        return false;
    }
    
    std::uint32_t offset;
    std::uint32_t size;
    std::uint32_t width;
    std::uint32_t height;
    
    bool ret = false;
    do
    {
        if (!bitmap_read_file_headers(fp, offset, size, width, height))
        {
            break;
        }
        
        //printf("offset:%d, size:%d, width:%d, height:%d\n", offset, size, width, height);
        img.create(height, width);
        
        bool brk = false;
        for (std::int32_t row = 0; row < img.rows(); row++)
        {
            std::int32_t line_len = img.cols() * 3;
            char tmp[4] = {0, 0, 0, 0};
            if (!bitmap_read_data(fp, img.data() + (img.rows() - 1 - row) * img.cols(), img.cols() * 3))
            {
                brk = true;
                break;
            }
            
            if (line_len % 4 > 0)
            {
                if (!bitmap_read_data(fp, tmp, 4 - line_len % 4))
                {
                    brk = true;
                    break;
                }
            }
        }
        if (brk)
        {
            break;
        }
        if (!bitmap_read_data(fp, img.data(), width * height * sizeof(rgb)))
        {
            break;
        }
        ret = true;
    } while (0);
    
    if (filename)
    {
        fclose(fp);
    }
    return ret;
}




bool bitmap_write(const char *filename, matrix_rgb& img)
{
    std::uint8_t buff [BITMAP_HEADER_SIZE] = {0};

    struct bitmap_file_head_struct bitmap_file_head;
    struct bitmap_head_struct bitmap_head;
    
    FILE *fp;
    if (filename)
    {
        fp = fopen(filename, "wb");
    }
    else
    {
        fp = stdout;
    }
    
    if (fp == NULL)
    {
        return false;
    }

    /* fprintf(stderr,"file size=%ld\n",temp_long); */

    bitmap_file_head.zz_magic[0] = 'B';
    bitmap_file_head.zz_magic[1] = 'M';
    bitmap_file_head.bf_size = img.rows() * img.cols() * sizeof(rgb) + BITMAP_HEADER_SIZE;
    bitmap_file_head.zz_hot_x = 0;
    bitmap_file_head.zz_hot_y = 0;
    bitmap_file_head.bf_offs = BITMAP_HEADER_SIZE;
    bitmap_file_head.bi_size = 0x28;
    
    
    bitmap_head.bi_width = img.cols();
    bitmap_head.bi_height = img.rows();
    bitmap_head.bi_planes = 1;
    bitmap_head.bi_bit_cnt = 24;
    bitmap_head.bi_compr = 0;
    bitmap_head.bi_size_im = img.rows() * img.cols() * sizeof(rgb);
    bitmap_head.bi_x_pels = img.cols() * 10;  /* why ??? xxx */
    bitmap_head.bi_y_pels= img.rows() * 10;  /* why ??? xxx */
    bitmap_head.bi_clr_used = 0;
    bitmap_head.bi_clr_imp = 0;
    
    buff[0x00] = (std::uint8_t) (bitmap_file_head.zz_magic[0]);
    buff[0x01] = (std::uint8_t) (bitmap_file_head.zz_magic[1]);

    buff[0x02] =  (std::uint8_t) (bitmap_file_head.bf_size);
    buff[0x03] =  (std::uint8_t) (bitmap_file_head.bf_size >> 8);
    buff[0x04] =  (std::uint8_t) (bitmap_file_head.bf_size >> 16);
    buff[0x05] =  (std::uint8_t) (bitmap_file_head.bf_size >> 24);
    
    
    buff[0x06] = (std::uint8_t) (bitmap_file_head.zz_hot_x);
    buff[0x07] = (std::uint8_t) (bitmap_file_head.zz_hot_x >> 8);
    
    buff[0x08] = (std::uint8_t) (bitmap_file_head.zz_hot_y);
    buff[0x09] = (std::uint8_t) (bitmap_file_head.zz_hot_y >> 8);
    
    buff[0x0a] = (std::uint8_t) (bitmap_file_head.bf_offs);
    buff[0x0b] = (std::uint8_t) (bitmap_file_head.bf_offs>> 8);
    buff[0x0c] = (std::uint8_t) (bitmap_file_head.bf_offs>> 16);
    buff[0x0d] = (std::uint8_t) (bitmap_file_head.bf_offs>> 24);
    
    buff[0x0e] = (std::uint8_t) (bitmap_file_head.bi_size);
    buff[0x0f] = (std::uint8_t) (bitmap_file_head.bi_size>> 8);
    buff[0x10] = (std::uint8_t) (bitmap_file_head.bi_size>> 16);
    buff[0x11] = (std::uint8_t) (bitmap_file_head.bi_size>> 24);

    /* write contents of bitmap_head_struct */

    buff[0x12] = (std::uint8_t) (bitmap_head.bi_width);
    buff[0x13] = (std::uint8_t) (bitmap_head.bi_width>> 8);
    buff[0x14] = (std::uint8_t) (bitmap_head.bi_width>> 16);
    buff[0x15] = (std::uint8_t) (bitmap_head.bi_width>> 24);
    
    buff[0x16] = (std::uint8_t) (bitmap_head.bi_height);
    buff[0x17] = (std::uint8_t) (bitmap_head.bi_height >> 8);
    buff[0x18] = (std::uint8_t) (bitmap_head.bi_height >> 16);
    buff[0x19] = (std::uint8_t) (bitmap_head.bi_height >> 24);
    
    buff[0x1a] = (std::uint8_t) (bitmap_head.bi_planes);
    buff[0x1b] = (std::uint8_t) (bitmap_head.bi_planes >> 8);
    
    buff[0x1c] = (std::uint8_t) (bitmap_head.bi_bit_cnt);
    buff[0x1d] = (std::uint8_t) (bitmap_head.bi_bit_cnt >> 8);
    
    buff[0x1e] = (std::uint8_t) (bitmap_head.bi_compr);
    buff[0x1f] = (std::uint8_t) (bitmap_head.bi_compr >> 8);
    buff[0x20] = (std::uint8_t) (bitmap_head.bi_compr >> 16);
    buff[0x21] = (std::uint8_t) (bitmap_head.bi_compr >> 24);
    
    buff[0x22] = (std::uint8_t) (bitmap_head.bi_size_im);
    buff[0x23] = (std::uint8_t) (bitmap_head.bi_size_im >> 8);
    buff[0x24] = (std::uint8_t) (bitmap_head.bi_size_im >> 16);
    buff[0x25] = (std::uint8_t) (bitmap_head.bi_size_im >> 24);
    
    buff[0x26] = (std::uint8_t) (bitmap_head.bi_x_pels);
    buff[0x27] = (std::uint8_t) (bitmap_head.bi_x_pels >> 8);
    buff[0x28] = (std::uint8_t) (bitmap_head.bi_x_pels >> 16);
    buff[0x29] = (std::uint8_t) (bitmap_head.bi_x_pels >> 24);
    
    buff[0x2a] = (std::uint8_t) (bitmap_head.bi_y_pels);
    buff[0x2b] = (std::uint8_t) (bitmap_head.bi_y_pels >> 8);
    buff[0x2c] = (std::uint8_t) (bitmap_head.bi_y_pels >> 16);
    buff[0x2d] = (std::uint8_t) (bitmap_head.bi_y_pels >> 24);
    
    buff[0x2e] = (std::uint8_t) (bitmap_head.bi_clr_used);
    buff[0x2f] = (std::uint8_t) (bitmap_head.bi_clr_used >> 8);
    buff[0x30] = (std::uint8_t) (bitmap_head.bi_clr_used >> 16);
    buff[0x31] = (std::uint8_t) (bitmap_head.bi_clr_used >> 24);
    
    buff[0x32] = (std::uint8_t) (bitmap_head.bi_clr_imp);
    buff[0x33] = (std::uint8_t) (bitmap_head.bi_clr_imp >> 8);
    buff[0x34] = (std::uint8_t) (bitmap_head.bi_clr_imp >> 16);
    buff[0x35] = (std::uint8_t) (bitmap_head.bi_clr_imp >> 24);

    fwrite((void *)buff, 1, BITMAP_HEADER_SIZE, fp);
    
    for (std::int32_t row = 0; row < img.rows(); row++)
    {
        std::int32_t line_len = img.cols() * 3;
        char tmp[4] = {0, 0, 0, 0};
        
        fwrite(img.data() + (img.rows() - 1 - row) * img.cols(), 1, img.cols() * 3, fp);
        
        if (line_len % 4 > 0)
        {
            fwrite(tmp, 1, 4 - line_len % 4, fp);
        }
    }

    if (filename)
    {
        fclose(fp);
    }
    return true;
}

#include <stdio.h>
#include "matrix.h"
#include "bitmap.h"
#include "atlc3.h"
#include <opencv2/opencv.hpp>

int main(int argc, char **argv)
{
    matrix_rgb img;
    bitmap_read("pcb.bmp", img);
    //bitmap_write("test.bmp", img);
    atlc3 atlc;
    atlc.setup_arrays(img);
    atlc.set_oddity_value();
    atlc.do_fd_calculation();
    //bitmap_read_file_headers(stdin, offset, size, width, height);
    //printf("offset:%d, size:%d, width:%d, height:%d\n", offset, size, width, height);
    //cv::Mat cvimg(img.rows(), img.cols(), CV_8UC3, img.data());
    //cv::imshow("img", cvimg);
    //cv::waitKey();
	printf("hello world\n");
	return 0;
}

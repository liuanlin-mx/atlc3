#include <stdio.h>
#include "matrix.h"
#include "bitmap.h"
#include "atlc3.h"
#include <opencv2/opencv.hpp>

int main(int argc, char **argv)
{
    
#if 0
    cv::Mat cvimg(480, 640, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::circle(cvimg, cv::Point2i(320, 240), 5, cv::Scalar(0, 0, 255), 5);
    cv::imwrite("a.bmp", cvimg);
    cv::imshow("xx", cvimg);
    cv::waitKey();
    return 0;
#endif


    matrix_rgb img;
    //bitmap_read("pcb.bmp", img);
    //bitmap_read("test/test3.bmp", img);
    //bitmap_read("test/test6.bmp", img);
    //bitmap_read("mmtl_tmp0.bmp", img);
    bitmap_read("mmtl_tmp2.bmp", img);
    //bitmap_write("test.bmp", img);
    //bitmap_read("a.bmp", img);
    bitmap_read("test/test-coupler1.bmp", img);
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

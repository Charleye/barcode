#include <opencv2/opencv.hpp>

using namespace cv;

#ifdef __cplusplus
extern "C" {
#endif

void image_process(unsigned char *image, int width, int height)
{
    Mat src(height, width, CV_8UC2, image);
    Mat dst(width, height, CV_8UC3);

    cvtColor(src, dst, CV_YUV2BGR_YUYV);
    imshow("Test", dst);
    waitKey(0);
}

#ifdef __cplusplus
}
#endif

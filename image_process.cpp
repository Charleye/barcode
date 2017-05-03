#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

Mat detection(Mat &image)
{
    Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);

    Mat imgX16S, imgY16S, SobelOut;
    Sobel(gray, imgX16S, CV_16S, 1, 0, 3, 1, 0, 4);
    Sobel(gray, imgY16S, CV_16S, 0, 1, 3, 1, 0, 4);
    convertScaleAbs(imgX16S, imgX16S, 1, 0);
    convertScaleAbs(imgY16S, imgY16S, 1, 0);
    SobelOut = imgX16S + imgY16S;

    Mat binary;
    threshold(SobelOut, binary, 100, 255, CV_THRESH_BINARY);

    Mat element = getStructuringElement(0, Size(3, 3));
    morphologyEx(binary, binary, MORPH_OPEN, element);

    Mat ele1 = getStructuringElement(0, Size(5, 5));
    for (int j = 0; j < 1; j++)
        erode(binary, binary, ele1);

    Mat ele2 = getStructuringElement(0, Size(13, 13));
    for (int i = 0; i < 5; i++)
        dilate(binary, binary, ele2);

    vector<vector<Point>> contours;
    vector<Vec4i> hiera;
    findContours(binary, contours, hiera, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    unsigned long area = 0;
    Rect maxrect;
    for (int i = 0; i < contours.size(); i++)
    {
        Rect rect = boundingRect((Mat)contours[i]);
        if (rect.area() > area)
        {
            area = rect.area();
            maxrect = rect;
        }
    }

    maxrect = maxrect + Point(-5, -5);
    maxrect = maxrect + Size(20, 20);
    if (maxrect.x < 0)
        maxrect.x = 0;
    if (maxrect.y < 0)
        maxrect.y = 0;
    if (maxrect.x + maxrect.width > image.cols)
        maxrect.x = image.cols - maxrect.width;
    if (maxrect.y + maxrect.height > image.rows)
        maxrect.y = image.rows - maxrect.height;

    Mat ROI(image, maxrect);

    return ROI;
}

void correction(Mat& image, Mat &dete)
{
    Mat gray;
    cvtColor(dete, gray, CV_BGR2GRAY);

    Mat imgX16S, imgY16S, SobelOut;
    Sobel(gray, imgX16S, CV_16S, 1, 0, 3, 1, 0, 4);
    Sobel(gray, imgY16S, CV_16S, 0, 1, 3, 1, 0, 4);
    convertScaleAbs(imgX16S, imgX16S, 1, 0);
    convertScaleAbs(imgY16S, imgY16S, 1, 0);
    SobelOut = imgX16S + imgY16S;

    Mat pad;
    int n = getOptimalDFTSize(SobelOut.rows);
    int m = getOptimalDFTSize(SobelOut.cols);
    copyMakeBorder(SobelOut, pad, 0, n-SobelOut.rows, 0, m-SobelOut.cols, BORDER_CONSTANT, Scalar::all(0));

    Mat planes[] = {Mat_<float>(pad), Mat::zeros(pad.size(), CV_32F)};
    Mat comImage;
    merge(planes, 2, comImage);
    dft(comImage, comImage);
    split(comImage, planes);
    magnitude(planes[0], planes[0], planes[1]);
    Mat magMat = planes[0];
    magMat += Scalar::all(1);
    log(magMat, magMat);
    magMat = magMat(Rect(0, 0, magMat.cols & -2, magMat.rows & -2));
    int cx = magMat.cols / 2;
    int cy = magMat.rows / 2;
    Mat q0(magMat, Rect(0, 0, cx, cy));
    Mat q1(magMat, Rect(0, cy, cx, cy));
    Mat q2(magMat, Rect(cx, cy, cx, cy));
    Mat q3(magMat, Rect(cx, 0, cx, cy));
    Mat tmp;
    q0.copyTo(tmp);
    q2.copyTo(q0);
    tmp.copyTo(q2);
    q1.copyTo(tmp);
    q3.copyTo(q1);
    tmp.copyTo(q3);
    normalize(magMat, magMat, 0, 1, CV_MINMAX);
    imshow("magMat", magMat);
}

#ifdef __cplusplus
extern "C" {
#endif

void image_process(unsigned char *image, int width, int height)
{
    Mat src(height, width, CV_8UC2, image);
    Mat dst(width, height, CV_8UC3);
    cvtColor(src, dst, CV_YUV2BGR_YUYV);

    Mat dete = detection(dst);
    correction(dst, dete);
    imshow("dete", dete);

    waitKey(0);
}

#ifdef __cplusplus
}
#endif

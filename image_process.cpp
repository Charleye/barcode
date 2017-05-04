#include <opencv2/opencv.hpp>
#include <zbar.h>

using namespace zbar;
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
    GaussianBlur(gray, gray, Size(7, 7), 0);

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
    Mat magImage(magMat.size(), CV_8UC1);
    magMat.convertTo(magImage, CV_8UC1, 255, 0);

    int thresh = 190;
    vector<Vec2f> lines;
    float pi_180 = (float)CV_PI / 180.0;
    Mat lineImage(magImage.size(), CV_8UC3);
    Mat tmp1(magImage.size(), CV_8UC3);
    magImage.copyTo(tmp1);
    do {
        threshold(magImage, tmp1, thresh, 255, CV_THRESH_BINARY);
        HoughLines(tmp1, lines, 1, pi_180, 90, 0, 0);
        thresh -= 4;
    }while(lines.size() < 3 && thresh > 120);

    while (lines.size() > 2)
    {
        thresh += 1;
        threshold(magImage, tmp1, thresh, 255, CV_THRESH_BINARY);
        HoughLines(tmp1, lines, 1, pi_180, 90, 0, 0);
    }

    int numLines = lines.size();
    float theta;
    cout << "numLines: " << numLines << endl;
    for (int i = 0; i < numLines; i++) {
        cout << "theta: " << lines[i][1] * 180 / CV_PI << endl;
        cout << "RHO: " << lines[i][0] << endl;

        float rho = lines[i][0];
        theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        pt1.x = cvRound(x0 + 1000 * (-b));
        pt1.y = cvRound(y0 + 1000 * (a));
        pt2.x = cvRound(x0 - 1000 * (-b));
        pt2.y = cvRound(y0 - 1000 * (a));
        line(lineImage, pt1, pt2, Scalar(245, 200, 0), 3, 8, 0);
    }
    float angle = 0;
    if (abs(180 * theta / CV_PI - 90) < 2 || abs(180 * theta / CV_PI - 180) < 2)
        angle = 0;
    else if (180 * theta / CV_PI < 90)
        angle = 180 * theta / CV_PI  + 270;
    else
        angle = 180 * theta / CV_PI - 90;

    cout << "Angle: " << angle << endl;

    Point center(image.cols / 2, image. rows / 2);
    Mat rotMat = getRotationMatrix2D(center, angle, 1.0);
    warpAffine(image, image, rotMat, image.size(), 1, 0, Scalar(255, 255, 255));

}

void getBarCode(Mat &dst)
{
    Mat gray;
    cvtColor(dst, gray, CV_BGR2GRAY);

    ImageScanner scanner;
    scanner.set_config(ZBAR_EAN13, ZBAR_CFG_ENABLE, 1);
    scanner.set_config(ZBAR_UPCA, ZBAR_CFG_ENABLE, 1);
    int width = gray.cols;
    int height = gray.rows;
    Image image(width, height, "Y800", gray.data, width*height);
    scanner.scan(image);
    Image::SymbolIterator symbol = image.symbol_begin();
    if (image.symbol_begin() == image.symbol_end())
    {
        cout << "Fail to identify bar code in the picture, Capture image again.\n" << endl;
        return;
    }
    for (; symbol != image.symbol_end(); ++symbol)
    {
        cout << endl;
        cout << "BarCode Type:  " << symbol->get_type_name() << endl << endl;;
        cout << "Bar Code Number:   " << symbol->get_data() << endl << endl;;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

void image_process(unsigned char *image, int width, int height)
{
    Mat src(height, width, CV_8UC2, image);
    Mat dst(width, height, CV_8UC3);
    cvtColor(src, dst, CV_YUV2BGR_YUYV);

    Mat origin;
    resize(dst, origin, Size(600, 400), 0, 0);
    imshow("Capture Image", origin);

//    Mat dete = detection(dst);
//    correction(dst, dete);
    getBarCode(dst);

    resize(dst, dst, Size(600, 400), 0, 0);
    imshow("Result", dst);

    waitKey(0);
}

#ifdef __cplusplus
}
#endif

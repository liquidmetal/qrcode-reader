#include <stdio.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "qrReader.h"

#include "FinderPatternTrio.h"

using namespace std;

int main() {
    //cv::VideoCapture capture = cv::VideoCapture(1);
    qrReader qr = qrReader();

    //if(!capture.isOpened()) {
    //    printf("Unable to open camera. Quitting.");
    //    return 1;
    //}

    cv::Mat image = cv::imread("./sample02.jpg");
    cv::Mat imgBW = cv::Mat(image.rows, image.cols, CV_8UC1);
    //while(true) {
    //    capture >> image;

        cv::cvtColor(image, imgBW, CV_BGR2GRAY);
        cv::threshold(imgBW, imgBW, 128, 255, cv::THRESH_BINARY);

        qr.setImage(imgBW);
        FinderPatternTrio *trio = qr.find();

        FinderPattern *topLeft = trio->getTopLeft();
        FinderPattern *topRight = trio->getTopRight();
        FinderPattern *bottomLeft= trio->getBottomLeft();

        line(image, cv::Point(topLeft->getX(), topLeft->getY()), cv::Point(topRight->getX(), topRight->getY()), CV_RGB(255,0,0), 2);
        line(image, cv::Point(topLeft->getX(), topLeft->getY()), cv::Point(bottomLeft->getX(), bottomLeft->getY()), CV_RGB(255,255,0), 2);

        cv::imshow("Original", image);
        cv::imshow("QR Code", imgBW);
        cv::waitKey(30);
    //}

    cv::waitKey(0);
    return 0;
}

#include <stdio.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "qrReader.h"

#include "FinderPatternTrio.h"

using namespace std;

int main() {
    cv::VideoCapture capture = cv::VideoCapture(1);
    qrReader qr = qrReader();

    if(!capture.isOpened()) {
        printf("Unable to open camera. Quitting.");
        return 1;
    }

    cv::Mat image;
    cv::Mat imgBW;
    while(true) {
        capture >> image;

        cv::cvtColor(image, imgBW, CV_BGR2GRAY);
        cv::threshold(imgBW, imgBW, 128, 255, cv::THRESH_BINARY);

        qr.setImage(imgBW);
        FinderPatternTrio *trio = qr.find();

        cv::imshow("QR Code", imgBW);
        cv::waitKey(30);
    }

    cv::waitKey(0);
    return 0;
}

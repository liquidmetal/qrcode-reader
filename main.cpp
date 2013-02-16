#include <stdio.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "qrReader.h"

using namespace std;

int main() {
    cv::VideoCapture capture = VideoCapture(1);
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
        cv::threshold(imgBW, imgBW, 128, 255, THRESH_BINARY);

        bool found = qr.find(imgBW);
        if(found)
            qr.drawFinders(imgBW);

        cv::imshow("QR Code", imgBW);
        cv::waitKey(30);
    }

    cv::waitKey(0);
    return 0;
}

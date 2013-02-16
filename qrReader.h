#include <opencv2/opencv.hpp>
#include "FinderPattern.h"
#include "FinderPatternTrio.h"

class qrReader {
public:
    bool find(cv::Mat img);
    bool checkRatio(int currentState[]);
    void setImage(cv::Mat);
    qrReader();
    ~qrReader();

private:
    cv::Mat img;
    int crossCheckStateCount[5];
    vector<FinderPattern> possibleCenters;
};

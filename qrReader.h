#include <opencv2/opencv.hpp>
#include "FinderPattern.h"
#include "FinderPatternTrio.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>

class qrReader {
public:
    void setImage(cv::Mat img);
    FinderPatternTrio* find();
    bool checkRatio(int currentState[]);
    FinderPatternTrio getFinderPatternTrio();
    qrReader();
    ~qrReader();

private:
    int* getCrossCheckStateCount();
    bool handlePossibleCenter(int stateCount[], int i, int j);
    float centerFromEnd(int stateCount[], int end);
    float crossCheckHorizontal(int startJ, int centerI, int maxCount, int originalStateCountTotal);
    float crossCheckVertical(int startI, int centerJ, int maxCount, int originalStateCountTotal);
    FinderPattern* selectBestPatterns();
    void orderBestPatterns(FinderPattern[]);
    bool haveMultiplyConfirmedCenters();
    int getRowSkip();

private:
    int crossCheckStateCount[5];
    bool hasSkipped;

    std::vector<FinderPattern*> possibleCenters;
    cv::Mat img;
    const int CENTER_QUORUM = 2;
};

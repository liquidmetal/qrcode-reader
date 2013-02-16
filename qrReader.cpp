#include "qrReader.h"

qrReader::qrReader() {
    // Constructor
    this->img = NULL;
    this->possibleCenters = vector<FinderPattern>();
}

qrReader::~qrReader() {
    // Destructor
    this->img = NULL;
}

void qrReader::setImage(cv::Mat img) {
    this->img = img;
}

bool qrReader::find() {
    if(this->img==NULL) {
        printf("You need to set an image first.");
        return false;
    }

    int skipRows = 3;
    int stateCount[5] = {0};
    int currentState = 0;

    for(int row=skipRows-1; row<img.rows;row+=skipRows) {
        // Before starting a new row, reset all state data
        stateCount[0] = 0;
        stateCount[1] = 0;
        stateCount[2] = 0;
        stateCount[3] = 0;
        stateCount[4] = 0;
        stateCount[5] = 0;
        currentState = 0;

        // Grab a pointer to the current row
        uchar *ptr = img.ptr<uchar>(row);
        for(int col=0;col<img.cols;col++) {
            // Is this a black pixel?
            if(ptr[col]<128) {
                if(currentState & 0x1 == 1) {
                    // We were counting white pixels so
                    // change the state now
                    currentState++;
                }
                // Works for both W->B and B->B transition
                stateCount[currentState]++;
            }
            else {
                // We got to a white pixel...
                if((currentState & 0x1) == 1) {
                    stateCount[currentState]++;
                }
                else {
                    // ... but we were counting black pixels
                    if(currentState == 4) {
                        // We found a 'white' area after the finder pattern
                        // Do processing for it here
                        if(checkRatio(stateCount)) {
                            // This is where we do some more checks
                            bool confirmed = handlePossibleCenter(stateCount, i, j);
                            if(confirmed) {
                                rowSkip = 2;
                                if(hasSkipped) {
                                    done = haveMultiplyConfirmedCenters();
                                }
                                else {
                                    int rowsToSkip = findRowSkip();
                                    if(rowsToSkip > stateCount[2]) {
                                        i += rowsToSkip - stateCount[2] - rowSkip;
                                        j = maxJ - 1;
                                    }
                                }
                            }
                            else {
                                stateCount[0] = stateCount[2];
                                stateCount[1] = stateCount[3];
                                stateCount[2] = stateCount[4];
                                stateCount[3] = 1;
                                stateCount[4] = 0;
                                currentState = 3;
                                continue;
                            }
                            
                            // Reset all counts and continue looking for more
                            // finder patterns in the current row
                            currentState = 0;
                            stateCount[0] = stateCount[1] = stateCount[2] = stateCount[3] = stateCount[4] = 0;
                        }
                        else {
                            currentState = 3;
                            stateCount[0] = stateCount[2];
                            stateCount[1] = stateCount[3];
                            stateCount[2] = stateCount[4];
                            stateCount[3] = 1;
                            stateCount[4] = 0;
                        }
                    }
                    else {
                        stateCount[++currentState]++;
                    }
                }
                else {
                        // We haven't gone 'out' of the finder pattern yet
                        // So increment the state
                        stateCount[currentState]++;
                }
            }
        }
        // ^ ends looping through current row

        if(checkRatios(stateCount)) {
            bool confirmed = handlePossibleCenter(stateCount, i, maxJ);
            if(confirmed) {
                rowSkip = stateCount[0];
                if(hasSkipped) {
                    done = haveMultiplyConfirmedCenters();
                }
            }
        }
    }
    // ^ ends looping through rows

    FinderPattern patternInfo[3] = selectBestPatterns();
    orderBestPatterns(patternInfo);
    return new FinderPatternTrio(patternInfo);
}

bool qrReader::checkRatio(int stateCount[]) {
    int totalFinderSize = 0;
    for(int i=0;i<5;i++) {
        int count = stateCount[i];
        totalFinderSize += count;
        if(count==0)
            return false;
    }
    
    if(totalFinderSize<7) {
        return false;
    }

    // Calculate the size of one module
    int moduleSize = ceil(totalFinderSize / 7.0);
    int maxVariance = moduleSize/2;

    bool retVal = ((abs(moduleSize - stateCount[0])) < maxVariance &&
                   (abs(moduleSize - stateCount[1])) < maxVariance &&
                   (abs(moduleSize - stateCount[2])) < 3*maxVariance &&
                   (abs(moduleSize - stateCount[3])) < maxVariance &&
                   (abs(moduleSize - stateCount[4])) < maxVariance);

    return retVal;
}

float qrReader::centerFromEnd(int[] stateCount, int end) {
    return (float)(end - stateCount[4] - stateCount[3]) - stateCount[2]/2.0f;
}

int[] qrReader::getCrossCheckStateCount() {
    crossCheckStateCount[0] = 0;
    crossCheckStateCount[1] = 0;
    crossCheckStateCount[2] = 0;
    crossCheckStateCount[3] = 0;
    crossCheckStateCount[4] = 0;
    return crossCheckStateCount;
}

float qrReader::crossCheckVertical(int startI, int centerJ, int maxCount, int originalStateCountTotal) {
    cv::Mat duplicateImg = img.clone();
    int[] stateCount = getCrossCheckStateCount();

    int i = startI;
    while(i>=0 && img.at<uchar>(centerJ, i) > 128) {
        stateCount[2]++;
        i--;
    }
    if(i<0) {
        return 0.0/0.0;
    }

    while(i>=0 && img.at<uchar>(centerJ, i) < 128 && stateCount[1]<=maxCount) {
        stateCount[1]++;
        i--;
    }
    if(i<0 || stateCount[1] > maxCount) {
        return 0.0/0.0;
    }

    while(i>=0 && img.at<uchar>(centerJ, i) > 128 && stateCount[0] <= maxCount) {
        stateCount[0]++;
        i--;
    }
    if(stateCount[0]>maxCount) {
        return 0.0/0.0;
    }

    // Now start moving down from the center
    i=startI + 1;
    while(i<maxI && img.at<uchar>(centerJ, i) > 128) {
        stateCount[2]++;
        i++;
    }
    if(i==maxI) {
        return 0.0/0.0;
    }

    while(i<maxI && img.at<uchar>(centerJ, i) < 128 && stateCount[3]<maxCount) {
        stateCount[3]++;
        i++;
    }
    if(i==maxI || stateCount[3]>maxCount) {
        return 0.0/0.0;
    }

    while(i<maxI && img.at<uchar>(centerJ, i) > 128 && stateCount[4] < maxCount) {
        stateCount[4]++;
        i++;
    }
    if(stateCount[4]>=maxCount) {
        return 0.0/0.0;
    }

    // If we found a finder patter like section, but it's size is more than
    // 40% different than the original, assume it's a false positive
    int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    if(5*abs(stateCountTotal-originalStateCountTotal) >= 2*originalStateCountTotal) {
        return 0.0/0.0;
    }

    return checkRatios(stateCount) ? centerFromEnd(stateCount, i) : 0.0/0.0;
}

float qrReader::crossCheckHorizontal(int startJ, int centerI, int maxCount, int originalStateCountTotal) {
    int maxJ = img.cols;
    int[] stateCount = getCrossCheckStateCount();

    int j=startJ;
    while(j>=0 && img.at<uchar>(j, centerI)>128) {
        stateCount[2]++;
        j--;
    }
    if(j<0) {
        return 0.0/0.0;
    }

    while(j>=0 && img.at<uchar>(j, centerI)<128 && stateCount[1]<=maxCount) {
        stateCount[1]++;
        j--;
    }
    if(j<0 || stateCount[1]>maxCount) {
        return 0.0/0.0;
    }

    while(j>=0 && img.at<uchar>(j, centerI)>128 && stateCount[0]<=maxCount) {
        stateCount[0]++;
        j--;
    }
    if(j<0 || stateCount[0]>maxCount) {
        return 0.0/0.0;
    }

    j = startJ + 1;
    while(j<maxJ && img.at<uchar>(j, centerI)>128) {
        stateCount[2]++;
        j++;
    }
    if(j==maxJ) {
        return 0.0/0.0;
    }

    while(j<maxJ && img.at<uchar>(j, centerI)<128 && stateCount[3]<=maxCount) {
        stateCount[3]++;
        j++;
    }
    if(j==maxJ || stateCount[3]>maxCount) {
        return 0.0/0.0;
    }

    while(j<maxJ && img.at<uchar>(j, centerI)>128 && stateCount[4]<=maxCount) {
        stateCount[4]++;
        j++;
    }
    if(j==maxJ || stateCount[4]>maxCount) {
        return 0.0/0.0;
    }

    int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    if(5*abs(stateCountTotal-originalStateCountTotal) >= originalStateCountTotal) {
        return 0.0/0.0;
    }

    return checkRatios(stateCount)?centerFromEnd(stateCount, j) : 0.0/0.0;
}

bool qrReader::handlePossibleCenter(int[] stateCount, int i, int j) {
    int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    float centerJ = centerFromEnd(stateCount, j);
    float centerI = crossCheckVertical(i, (int)centerJ, stateCount[2], stateCountTotal);

    if(!isnan(centerI)) {
        // Cross check against the horizontal
        centerJ = crossCheckHorizontal((int)centerJ, (int)centerI, stateCount[2], stateCountTotal);

        // Do we have a center?
        if(!isnan(centerJ)) {
            float estimatedModuleSize = (float)stateCountTOtal/7.0f;
            boolean found = false;
            for(int index=0;index<possibleCenters.size();index++) {
                FinderPattern center = possibleCenters[index];
                if(center.aboutEquals(estimatedModuleSize, centerI, centerJ)) {
                    possiblecenters[index] = center.combineEstimate(centerI, centerJ, estimatedModuleSize);
                    found = true;
                    break;
                }
            }

            if(!found) {
                FinderPattern newCenter = FinderPattern(centerJ, centerI, estimatedModuleSize);
                possibleCenters.add(newCenter);
            }
            return true;
        }
    }
    return false;
}

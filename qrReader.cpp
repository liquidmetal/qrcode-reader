#include "qrReader.h"

qrReader::qrReader() {
    // Constructor
    this->possibleCenters = std::vector<FinderPattern*>();
}

qrReader::~qrReader() {
    // Destructor
}

void qrReader::setImage(cv::Mat img) {
    this->img = img;
}

FinderPatternTrio* qrReader::find() {
    bool done = false;
    int skipRows = 3;
    int stateCount[5] = {0};
    int currentState = 0;

    skipRows = (3*img.cols)/(4*MAXIMUM_MODULES);
    if(skipRows < MIN_SKIP) {
        skipRows = MIN_SKIP;
    }

    for(int row=skipRows-1; row<img.cols && !done;row+=skipRows) {
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
                if((currentState & 0x1) == 1) {
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
                            bool confirmed = handlePossibleCenter(stateCount, row, col);
                            if(confirmed) {
                                skipRows = 2;
                                if(hasSkipped) {
                                    done = haveMultiplyConfirmedCenters();
                                }
                                else {
                                    int rowsToSkip = getRowSkip();
                                    if(rowsToSkip > stateCount[2]) {
                                        row += rowsToSkip - stateCount[2] - skipRows;
                                        col = img.cols - 1;
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
                            stateCount[0] = 0;
                            stateCount[1] = 0;
                            stateCount[2] = 0;
                            stateCount[3] = 0;
                            stateCount[4] = 0;
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
            }
        }
        // ^ ends looping through current row

        if(checkRatio(stateCount)) {
            bool confirmed = handlePossibleCenter(stateCount, row, img.cols);
            if(confirmed) {
                skipRows = stateCount[0];
                if(hasSkipped) {
                    done = haveMultiplyConfirmedCenters();
                }
            }
        }
    }
    // ^ ends looping through rows

    for(unsigned int l=0;l<possibleCenters.size();l++) {
        printf("(%f, %f)\n", possibleCenters[l]->getX(), possibleCenters[l]->getY());
    }

    FinderPattern **patternInfo = identifyBestPatterns();
    patternInfo = orderBestPatterns(patternInfo);
    return new FinderPatternTrio(patternInfo);
}

FinderPattern** qrReader::identifyBestPatterns() {
    int initialSize = possibleCenters.size();
    if(initialSize < 3) {
        printf("Not enough centers found in the image. Skipping");
        
        return NULL;
    }

    if(initialSize > 3) {
        float combinedModuleSize = 0.0f;
        float sq = 0.0f;

        for(unsigned int index=0;index<possibleCenters.size();index++) {
            float size = possibleCenters[index]->getEstimatedModuleSize();
            combinedModuleSize += size;
            sq += size*size;
        }
        float avg = combinedModuleSize / (float)initialSize;
        float stdDev = (float)sqrt(sq/initialSize - avg*avg);
        float limit = 0.2f*avg;
        if(stdDev>limit)
            limit = stdDev;

        for(unsigned int l=0;l<possibleCenters.size();l++) {
            for(unsigned int k=0;k<possibleCenters.size()-1;k++) {
                if(fabs(possibleCenters[k]->getEstimatedModuleSize()-avg) > fabs(possibleCenters[k+1]->getEstimatedModuleSize()-avg)) {
                    FinderPattern *temp = possibleCenters[k];
                    possibleCenters.erase(possibleCenters.begin() + k);

                    possibleCenters.insert(possibleCenters.begin()+k+1, temp);
                    k--;
                }
            }
        }

        for(unsigned int l=0;l<possibleCenters.size() && possibleCenters.size()>3;l++) {
            if(fabs(avg - possibleCenters[l]->getEstimatedModuleSize()) > limit) {
                possibleCenters.erase(possibleCenters.begin()+l);
                l--;
            }
        }
    }

    FinderPattern **patterns = (FinderPattern**)malloc(sizeof(FinderPattern*)*3);
    patterns[0] = possibleCenters[0];
    patterns[1] = possibleCenters[1];
    patterns[2] = possibleCenters[2];

    return patterns;
}

// Order the three finder patterns A, B, C such that B is the top-left
// pattern and A, C are the ones on the sides.
FinderPattern** qrReader::orderBestPatterns(FinderPattern **pattern) {
    float distanceAB = sqrt((pattern[0]->getX()-pattern[1]->getX())*(pattern[0]->getX()-pattern[1]->getX()) - 
                            (pattern[0]->getY()-pattern[1]->getY())*(pattern[0]->getY()-pattern[1]->getY()));
    float distanceBC = sqrt((pattern[1]->getX()-pattern[2]->getX())*(pattern[1]->getX()-pattern[2]->getX()) - 
                            (pattern[1]->getY()-pattern[2]->getY())*(pattern[1]->getY()-pattern[2]->getY()));
    float distanceAC = sqrt((pattern[0]->getX()-pattern[2]->getX())*(pattern[0]->getX()-pattern[2]->getX()) - 
                            (pattern[0]->getY()-pattern[2]->getY())*(pattern[0]->getY()-pattern[2]->getY()));

    FinderPattern **returnPatterns = (FinderPattern**)malloc(sizeof(FinderPattern*)*3);
    FinderPattern *patternA, *patternB, *patternC;

    if(distanceBC >= distanceAB && distanceBC >= distanceAC) {
        patternA = pattern[1];
        patternB = pattern[0];
        patternC = pattern[2];
    }
    else if(distanceAC >= distanceAB && distanceAC >= distanceBC) {
        patternA = pattern[0];
        patternB = pattern[1];
        patternC = pattern[2];
    } else {
        patternA = pattern[0];
        patternB = pattern[2];
        patternC = pattern[1];
    }

    // Check the cross product
    float bx = patternB->getX();
    float by = patternB->getY();
    float cross = ((patternC->getX()-bx)*(patternA->getY()-by)) - ((patternC->getY()-by)*(patternA->getX()-bx));

    if(cross<0.0f) {
        FinderPattern *temp = patternA;
        patternA = patternC;
        patternC = temp;
    }
    returnPatterns[0] = patternA;
    returnPatterns[1] = patternB;
    returnPatterns[2] = patternC;

    return returnPatterns;
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
                   (abs(3*moduleSize - stateCount[2])) < 3*maxVariance &&
                   (abs(moduleSize - stateCount[3])) < maxVariance &&
                   (abs(moduleSize - stateCount[4])) < maxVariance);
    return retVal;
}

float qrReader::centerFromEnd(int stateCount[], int end) {
    return (float)(end - stateCount[4] - stateCount[3]) - stateCount[2]/2.0f;
}

int* qrReader::getCrossCheckStateCount() {
    crossCheckStateCount[0] = 0;
    crossCheckStateCount[1] = 0;
    crossCheckStateCount[2] = 0;
    crossCheckStateCount[3] = 0;
    crossCheckStateCount[4] = 0;
    return crossCheckStateCount;
}

float qrReader::crossCheckVertical(int startI, int centerJ, int maxCount, int originalStateCountTotal) {
    int* stateCount = getCrossCheckStateCount();

    int i = startI;
    int maxI = img.rows;
    while(i>=0 && img.at<uchar>(i, centerJ) < 128) {
        stateCount[2]++;
        i--;
    }
    if(i<0) {
        return 0.0/0.0;
    }

    while(i>=0 && img.at<uchar>(i, centerJ) > 128 && stateCount[1]<=maxCount) {
        stateCount[1]++;
        i--;
    }
    if(i<0 || stateCount[1] > maxCount) {
        return 0.0/0.0;
    }

    while(i>=0 && img.at<uchar>(i, centerJ) < 128 && stateCount[0] <= maxCount) {
        stateCount[0]++;
        i--;
    }
    if(stateCount[0]>maxCount) {
        return 0.0/0.0;
    }

    // Now start moving down from the center
    i=startI + 1;
    while(i<maxI && img.at<uchar>(i, centerJ) < 128) {
        stateCount[2]++;
        i++;
    }
    if(i==maxI) {
        return 0.0/0.0;
    }

    while(i<maxI && img.at<uchar>(i, centerJ) > 128 && stateCount[3]<maxCount) {
        stateCount[3]++;
        i++;
    }
    if(i==maxI || stateCount[3]>maxCount) {
        return 0.0/0.0;
    }

    while(i<maxI && img.at<uchar>(i, centerJ) < 128 && stateCount[4] < maxCount) {
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

    return checkRatio(stateCount)?centerFromEnd(stateCount, i):(0.0/0.0);
}

float qrReader::crossCheckHorizontal(int startJ, int centerI, int maxCount, int originalStateCountTotal) {
    int maxJ = img.cols;
    int* stateCount = getCrossCheckStateCount();

    int j=startJ;
    while(j>=0 && img.at<uchar>(centerI, j)<128) {
        stateCount[2]++;
        j--;
    }
    if(j<0) {
        return 0.0/0.0;
    }

    while(j>=0 && img.at<uchar>(centerI, j)>128 && stateCount[1]<=maxCount) {
        stateCount[1]++;
        j--;
    }
    if(j<0 || stateCount[1]>maxCount) {
        return 0.0/0.0;
    }

    while(j>=0 && img.at<uchar>(centerI, j)<128 && stateCount[0]<=maxCount) {
        stateCount[0]++;
        j--;
    }
    if(j<0 || stateCount[0]>maxCount) {
        return 0.0/0.0;
    }

    j = startJ + 1;
    while(j<maxJ && img.at<uchar>(centerI, j)<128) {
        stateCount[2]++;
        j++;
    }
    if(j==maxJ) {
        return 0.0/0.0;
    }

    while(j<maxJ && img.at<uchar>(centerI, j)>128 && stateCount[3]<=maxCount) {
        stateCount[3]++;
        j++;
    }
    if(j==maxJ || stateCount[3]>maxCount) {
        return 0.0/0.0;
    }

    while(j<maxJ && img.at<uchar>(centerI, j)<128 && stateCount[4]<=maxCount) {
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

    return checkRatio(stateCount)?centerFromEnd(stateCount, j) : 0.0/0.0;
}

bool qrReader::handlePossibleCenter(int stateCount[], int i, int j) {
    int stateCountTotal = stateCount[0] + stateCount[1] + stateCount[2] + stateCount[3] + stateCount[4];
    float centerJ = centerFromEnd(stateCount, j);
    float centerI = crossCheckVertical(i, (int)centerJ, stateCount[2], stateCountTotal);

    if(!isnan(centerI)) {
        // Cross check against the horizontal
        centerJ = crossCheckHorizontal((int)centerJ, (int)centerI, stateCount[2], stateCountTotal);

        // Do we have a center?
        if(!isnan(centerJ)) {
            float estimatedModuleSize = (float)stateCountTotal/7.0f;
            bool found = false;
            for(unsigned int index=0;index<this->possibleCenters.size();index++) {
                FinderPattern *center = this->possibleCenters[index];
                if(center->aboutEquals(estimatedModuleSize, centerI, centerJ)) {
                    this->possibleCenters[index] = center->combineEstimate(centerI, centerJ, estimatedModuleSize);
                    found = true;
                    break;
                }
            }

            if(!found) {
                FinderPattern *newCenter = new FinderPattern(centerJ, centerI, estimatedModuleSize);
                printf("Created new center: (%f, %f)\n", newCenter->getX(), newCenter->getY());
                possibleCenters.push_back(newCenter);
            }

            return true;
        }
    }
    printf("Returning false\n");
    return false;
}

bool qrReader::haveMultiplyConfirmedCenters() {
    return false;
}

int qrReader::getRowSkip() {
    int max = possibleCenters.size();
    if(max<=1) {
        return 0;
    }

    FinderPattern *firstConfirmedCenter = NULL;
    for(int index=0;index<max;index++) {
        FinderPattern *center = possibleCenters[index];

        if(center->getCount() >= CENTER_QUORUM) {
            if(firstConfirmedCenter==NULL) {
                firstConfirmedCenter = center;
            }
            else {
                hasSkipped = true;
                return (int)(abs(firstConfirmedCenter->getX() - center->getX()) - abs(firstConfirmedCenter->getY() - center->getY()))/2;
            }
        }
    }
    return 0;
}

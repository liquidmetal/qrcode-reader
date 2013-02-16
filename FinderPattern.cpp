#include "FinderPattern.h"

FinderPattern::FinderPattern(float posX, float posY, float estimatedModuleSize) {
    this->posX = posX;
    this->posY = posY;
    this->estimatedModuleSize = estimatedModuleSize;
    this->count = 1;
}

FinderPattern::FinderPattern(float posX, float posY, float estimatedModuleSize, int count) {
    this->posX = posX;
    this->posY = posY;
    this->estimatedModuleSize = estimatedModuleSize;
    this->count = count;
}

FinderPattern::~FinderPattern() {
    // Destructor
}

float FinderPattern::getEstimatedModuleSize() {
    return this->estimatedModuleSize;
}

int FinderPattern::getCount() {
    return this->count;
}

void FinderPattern::incrementCount() {
    this->count++;
}

bool FinderPattern::aboutEquals(float moduleSize, float i, float j) {
    if(abs(i-posY) <= moduleSize && abs(j-posX) <= moduleSize) {
        float moduleSizeDiff = abs(moduleSize - estimatedModuleSize);
        return moduleSizeDiff <= 1.0f || moduleSizeDiff <= estimatedModuleSize;
    }

    return false;
}

FinderPattern* FinderPattern::combineEstimate(float i, float j, float newModuleSize) {
    int combinedCount = count + 1;
    float combinedX = ((count * posX) + j) / combinedCount;
    float combinedY = ((count * posY) + i) / combinedCount;
    float combinedModuleSize = (count * estimatedModuleSize + newModuleSize) / combinedCount;

    return new FinderPattern(combinedX, combinedY, combinedModuleSize, combinedCount);
}

float FinderPattern::getX() {
    return posX;
}

float FinderPattern::getY() {
    return posY;
}

#include "FinderPattern.h"

FinderPattern::FinderPattern(float posX, float posY, float estimatedModuleSize) {
    this(posX, posY, estimatedModuleSize, 1);
}

FinderPattern::FinderPattern(float posX, float posY, float estimatedModuleSize, int count) {
    this->posX = posX;
    this->posY = posY;
    this->estimatedModuleSize = estimatedModuleSize;
    this->count = count;
}

float getEstimatedModuleSize() {
    return this->estimatedModuleSize;
}

int getCount() {
    return this->count;
}

void incrementCount() {
    this->count++;
}

bool aboutEquals(float moduleSize, float i, float j) {
    if(abs(i-posY) <= moduleSize && abs(j-posX) <= moduleSize) {
        float moduleSizeDiff = abs(moduleSize - estimatedModuleSize);
        return moduleSizeDIff <= 1.0f || moduleSizeDiff <= estimatedModuleSize;
    }

    return false;
}

FinderPattern combineEstimate(float i, float j, float newModuleSize) {
    int combinedCount = count + 1;
    float combinedX = ((count * posX) + j) / combinedCount;
    float combinedY = ((count * posY) + i) / combinedCount;
    float combinedModuleSize = (count * estimatedModuleSize + newModuleSize) / combinedCount;

    return FinderPattern(combinedX, combinedY, combinedModuleSize, combinedCount);
}

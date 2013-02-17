#include "FinderPatternTrio.h"

FinderPattern* FinderPatternTrio::getBottomLeft() {
    return bottomLeft;
}

FinderPattern* FinderPatternTrio::getTopLeft() {
    return topLeft;
}

FinderPattern* FinderPatternTrio::getTopRight() {
    return topRight;
}

FinderPatternTrio::FinderPatternTrio(FinderPattern **patterns) {
    bottomLeft = patterns[0];
    topLeft = patterns[1];
    topRight = patterns[2];
}

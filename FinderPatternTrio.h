#ifndef __FINDER_PATTERN_INFO
#define __FINDER_PATTERN_INFO

#include "FinderPattern.h"

class FinderPatternTrio {
private:
    FinderPattern *bottomLeft;
    FinderPattern *topLeft;
    FinderPattern *topRight;

public:
    FinderPattern getBottomLeft();
    FinderPattern getTopLeft();
    FinderPattern getTopRight();
    FinderPatternTrio(FinderPattern*);
};
#endif

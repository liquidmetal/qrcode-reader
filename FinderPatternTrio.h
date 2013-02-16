#include "FinderPattern.h"

class FinderPatternTrio {
private:
    FinderPattern bottomLeft;
    FinderPattern topLeft;
    FinderPattern topRight;

public:
    FinderPattern getBottomLeft();
    FinderPattern getTopLeft();
    FinderPattern getTopRight();
    FinderPatternInfo(FinderPattern[]);
}

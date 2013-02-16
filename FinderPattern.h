class FinderPattern {
private:
    float estimatedModuleSize;
    int count;

public:
    FinderPattern(float, float, float);
    FinderPattern(float, float, float, int);
    float getEstimatedModuleSize();
    int getCount();
    void incrementCount();
    bool aboutEquals(float, float, float);
    FinderPattern combineEstimate(float, float, float);
}

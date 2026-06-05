#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H


#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct BoundingBox
{
    float x, y, w, h;
    BoundingBox(void) : x(0), y(0), w(0), h(0) {}
    BoundingBox(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
};

enum class InterpolationMethod
{
    LINEAR,
    LINEAR_SEPARATE,
    CUBIC_SPLINE,
    SMOOTH_STEP
};

class Interpolator
{
public:
    static std::vector<BoundingBox> interpolateLinear(
        const BoundingBox& start,
        const BoundingBox& end,
        int numFrames
    );

    static std::vector<BoundingBox> interpolateSmoothStep(
        const BoundingBox& start,
        const BoundingBox& end,
        int numFrames
    );

    static std::vector<BoundingBox> interpolateCubicSpline(
        const std::vector<BoundingBox>& keyFrames,
        const std::vector<int>& keyIndices,
        int totalFrames
    );

    static void processSequence(
        const std::string& folderPath,
        const BoundingBox& firstBBox,
        const BoundingBox& lastBBox,
        InterpolationMethod method,
        const std::string& outputFolder = ""
    );
};


#endif // INTERPOLATOR_H
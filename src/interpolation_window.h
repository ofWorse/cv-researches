#ifndef INTERPOLATION_H
#define INTERPOLATION_H


#include "imgui.h"
#include "interpolator.h"
#include <string>

class InterpolationWindow
{
private:
    std::string folderPath;
    BoundingBox firstBbox;
    BoundingBox lastBbox;
    int currentMethod{0};
    bool useFirstImageSize{false};
    bool processing{false};
    int imageWidth{640};
    int imageHeight{480};
    std::string outputFolder;

public:
    InterpolationWindow(void);
    void render(void);

    bool isVisible{true};

private:
    void drawCoordinatesEditor(
        const std::string& label, 
        BoundingBox& bbox, 
        float imageWidth = 1.0f, 
        float imageHeight = 1.0f
    );
};


#endif // INTERPOLATION_H
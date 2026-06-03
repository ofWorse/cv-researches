#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <imgui.h>
#include <string>
#include <vector>

class ImageViewer
{
private:
    std::vector<std::string> imageFiles;
    std::string currentDir;
    std::string currentImage;
    int selectedIndex;

    float zoomLevel;
    ImVec2 imagePosition;
    bool autoFit;
    
public:
    ImageViewer(void);
    ~ImageViewer(void);

    void init(void);
    void update(void);
    void render(void);

    bool isVisible{true};

private:
    void renderFileBrowser(void);
    void renderImageDisplay(void);
    void renderToolBar(void);

    void scanDirectory(const std::string& dir);
    bool isImageFile(const std::string& filename);

};

#endif // IMAGE_VIEWER_H
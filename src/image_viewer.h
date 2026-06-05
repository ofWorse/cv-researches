#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <imgui.h>
#include <string>
#include <vector>
#include <stack>
#include <filesystem>

struct FileSystemEntry 
{
    std::string name;
    std::string path;
    bool isDirectory;
    uintmax_t size;
    
    FileSystemEntry(const std::string& n, const std::string& p, bool isDir, uintmax_t s = 0)
        : name(n), path(p), isDirectory(isDir), size(s) {}
};

class ImageViewer
{
private:
    std::vector<FileSystemEntry> fileSystemEntries;
    std::stack<std::string> backHistory;
    std::stack<std::string> forwardHistory;
    std::string currentPath;
    std::string currentDir;
    std::string currentImage;

    int selectedIndex;

    float zoomLevel;
    
    bool autoFit;
    bool showCoordinates;
    bool hasStartPoint;
    bool hasEndPoint;
    bool showOnlyImages;
    
    ImVec2 imagePosition;
    ImVec2 currentMouseCoord;
    ImVec2 currentMouseCoordNorm;
    ImVec2 startPoint;
    ImVec2 endPoint;
    
public:
    ImageViewer(void);
    ~ImageViewer(void);

    void init(void);
    void update(void);
    void render(void);

    bool isVisible{true};

private:
    void copyToClipboard(const std::string& text);

    void renderFileBrowser(void);
    void renderImageDisplay(void);
    void renderToolBar(void);
    void handleKeyboardShortcuts(void);

    void scanDirectory(const std::string& dir);
    bool isImageFile(const std::string& filename);
    
    void navigateTo(const std::string& newPath);
    void goBack(void);
    void goForward(void);
    void updateNavigationStack(const std::string& newPath);
    void clearForwardStack(void);
    
    std::string formatFileSize(uintmax_t size);
};

#endif // IMAGE_VIEWER_H
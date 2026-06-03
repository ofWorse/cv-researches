#ifndef APPLICATION_H
#define APPLICATION_H

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

class ImageViewer;

class Application
{
private:
    GLFWwindow* window;
    std::unique_ptr<ImageViewer> imageViewer;

    bool isRunning;
    ImVec4 clearColor;

public:
    Application(void);
    ~Application(void);

    bool init(int width, int height, const char* title);
    void run(void);
    void shutdown(void);

private:
    void processInput(void);
    void update(void);
    void render(void);
    void renderUI(void);
    bool initImGui(void);
    bool initGLFW(int width, int height, const char* title);
    void initComponents(void);
};




#endif // APPLICATION_H
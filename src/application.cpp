#include "application.h"
#include "image_viewer.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

Application::Application(void) 
    : window(nullptr)
    , isRunning(false)
    , clearColor(0.45f, 0.55f, 0.60f, 1.00f)
{
}

Application::~Application(void) 
{
    shutdown();
}

bool Application::init(int width, int height, const char* title) 
{
    if(!initGLFW(width, height, title)) 
    {
        return false;
    }

    if(!initImGui()) 
    {
        return false;
    }

    initComponents();
    isRunning = true;
    return true;
}

bool Application::initGLFW(int width, int height, const char* title) 
{
    if(!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if(window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return true;
}

bool Application::initImGui(void) 
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;

    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();

    if(!ImGui_ImplGlfw_InitForOpenGL(window, true)) 
    {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        return false;
    }

    if(!ImGui_ImplOpenGL3_Init("#version 330")) 
    {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void Application::initComponents(void) 
{
    imageViewer = std::make_unique<ImageViewer>();
    imageViewer->init();
    interpolationWindow = std::make_unique<InterpolationWindow>();

}

void Application::run(void) 
{
    while(isRunning && !glfwWindowShouldClose(window)) 
    {
        processInput();
        update();
        render();
    }
}

void Application::processInput(void) 
{
    glfwPollEvents();

    if(glfwWindowShouldClose(window)) 
    {
        isRunning = false;
    }

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
    {
        isRunning = false;
    }
}

void Application::update(void) 
{
    if(imageViewer) 
    {
        imageViewer->update();
    }
}

void Application::render(void) 
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderUI();

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void Application::renderUI(void) 
{

    // That's how we initialize elements of main menu.
    // Not by allocating memory for each widget, but by 
    // calling ImGui functions directly in render loop.
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Exit", "Ctrl+Q"))
            {
                isRunning = false;
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View")) 
        {
            ImGui::MenuItem("Image Viewer", nullptr, &imageViewer->isVisible);
            ImGui::MenuItem("Interpolation Tool", nullptr, &interpolationWindow->isVisible);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if(imageViewer && imageViewer->isVisible) 
    {
        imageViewer->render();
    }

    if(interpolationWindow && interpolationWindow->isVisible) 
    {
        interpolationWindow->render();
    }

    ImGui::Begin("Application Info");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Delta Time: %.3f ms", ImGui::GetIO().DeltaTime * 1000.0f);
    ImGui::End();   
}

void Application::shutdown(void) 
{
    imageViewer.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if(window) 
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
    }
}

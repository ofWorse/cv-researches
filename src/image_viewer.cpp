#include "image_viewer.h"
#include "texture_manager.h"
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

ImageViewer::ImageViewer(void) 
    : selectedIndex(-1)
    , zoomLevel(1.0f)
    , imagePosition(0.0f, 0.0f)
    , autoFit(true)
{
}

ImageViewer::~ImageViewer(void) 
{
}

void ImageViewer::init(void) 
{
    #ifdef __APPLE__
        const char* home = getenv("HOME");
        if(home)
        {
            currentDir = fs::path(home) / "Pictures";
        }
        else
        {
            currentDir = fs::current_path().string();
        }
    #elif defined(_WIN32)
        const char* pictures = getenv("USERPROFILE");
        if(pictures)
        {
            currentDir = fs::path(pictures) / "Pictures";
        }
        else
        {
            currentDir = fs::current_path().string();
        }
    #else
        currentDir = fs::current_path().string();
    #endif

    if(!fs::exists(currentDir)) 
    {
        std::cerr << "Default pictures directory does not exist: " << currentDir << '\n';
        currentDir = fs::current_path().string();
    }

    scanDirectory(currentDir);
}

void ImageViewer::update(void) 
{
    if(selectedIndex >= 0 && selectedIndex < static_cast<int>(imageFiles.size())) 
    {
        currentImage = imageFiles[selectedIndex];
    }
}

void ImageViewer::render(void) 
{
    if(!isVisible) 
    {
        return;
    }

    ImGui::Begin("Image Viewer", &isVisible);

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open..."))
            {
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Close", "Cmd+q"))
            {
                isVisible = false;
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Auto Fit", nullptr, &autoFit);
            if(ImGui::MenuItem("Zoom In", "Cmd++"))
            {
                zoomLevel *= 1.2f;
                autoFit = false;
            }
            if(ImGui::MenuItem("Zoom Out", "Cmd+-"))
            {
                zoomLevel /= 1.2f;
                autoFit = false;
            }
            if(ImGui::MenuItem("Reset Zoom", "Cmd+0"))
            {
                zoomLevel = 1.0f;
                autoFit = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Columns(2, "ImageViewerColumns");
    renderFileBrowser();
    ImGui::NextColumn();
    renderImageDisplay();
    ImGui::Columns(1);
    renderToolBar();
    ImGui::End();
}

void ImageViewer::renderFileBrowser(void) 
{
    ImGui::BeginChild("FileList", ImVec2(0, 0), true);


    ImGui::Text("Directory: %s", currentDir.c_str());
    ImGui::Separator();

    if(ImGui::Button("..") && currentDir != fs::path(currentDir).root_path().string())
    {
        fs::path parent = fs::path(currentDir).parent_path();
        if(!parent.empty())
        {
            currentDir = parent.string();
            scanDirectory(currentDir);
            selectedIndex = -1;
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Refresh"))
    {        
        scanDirectory(currentDir);
    }

    ImGui::Separator();

    ImGui::Text("Images (%zu)", imageFiles.size());
    ImGui::Separator();
    
    for (size_t i = 0; i < imageFiles.size(); ++i) {
        const auto& filename = imageFiles[i];
        fs::path filepath = fs::path(currentDir) / filename;
        
        // Показываем имя файла и размер
        std::string display_name = filename;
        if(fs::exists(filepath)) 
        {
            auto size = fs::file_size(filepath);
            if(size > 1024 * 1024)
            {
                display_name += " (" + std::to_string(static_cast<int>(size / (1024.0 * 1024.0))) + " MB)";
            } 
            else if(size > 1024) 
            {
                display_name += " (" + std::to_string(static_cast<int>(size / 1024.0)) + " KB)";
            }
        }
        
        if(ImGui::Selectable(display_name.c_str(), selectedIndex == i)) 
        {
            selectedIndex = i;
            
            std::string full_path = (fs::path(currentDir) / filename).string();
            TextureManager::getInstance().loadTexture(full_path, "current_image");
        }
    }
    
    ImGui::EndChild();
}

void ImageViewer::renderImageDisplay(void) 
{
    ImGui::BeginChild("ImageDisplay", ImVec2(0, 0), true);

    Texture* texture = TextureManager::getInstance().getTexture("current_image");

    if(texture && texture->id) 
    {
        ImGui::Text("Image: %s", currentImage.c_str());
        ImGui::Text("Size: %dx%d", texture->width, texture->height);
        ImGui::Text("Zoom: %.1f%%", zoomLevel * 100.0f);
        ImGui::Separator();
        
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        ImVec2 image_size = ImVec2(texture->width * zoomLevel, texture->height * zoomLevel);
        
        if(autoFit && image_size.x > available_size.x) 
        {
            float scale = available_size.x / image_size.x;
            image_size.x *= scale;
            image_size.y *= scale;
            zoomLevel = image_size.x / texture->width;
        }
        
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 center_offset(
            (available_size.x - image_size.x) * 0.5f,
            (available_size.y - image_size.y) * 0.5f
        );
        
        ImGui::SetCursorScreenPos(ImVec2(
            cursor_pos.x + center_offset.x + imagePosition.x,
            cursor_pos.y + center_offset.y + imagePosition.y
        ));
        
        ImGui::Image((ImTextureID)(intptr_t)texture->id, image_size);
        
        if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) 
        {
            imagePosition.x += ImGui::GetIO().MouseDelta.x;
            imagePosition.y += ImGui::GetIO().MouseDelta.y;
        }
        
        // Зум колесиком
        if(ImGui::IsItemHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if(wheel != 0) 
            {
                ImVec2 old_size = image_size;
                zoomLevel *= (wheel > 0) ? 1.1f : 0.9f;
                autoFit = false;
            }
        }
        
        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            zoomLevel = 1.0f;
            imagePosition = ImVec2(0, 0);
            autoFit = true;
        }
    } 
    else 
    {
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPos(ImVec2(available_size.x * 0.5f - 100, available_size.y * 0.5f));
        ImGui::Text("No image selected");
        ImGui::SetCursorPos(ImVec2(available_size.x * 0.5f - 150, available_size.y * 0.5f + 20));
        ImGui::Text("Select an image from the list on the left");
    }

    ImGui::EndChild();
}

void ImageViewer::renderToolBar(void) 
{
    ImGui::Separator();
    if(selectedIndex >= 0 && selectedIndex < static_cast<int>(imageFiles.size())) 
    {
        ImGui::Text("Selected: %s", imageFiles[selectedIndex].c_str());
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
}

void ImageViewer::scanDirectory(const std::string& dir) 
{
    imageFiles.clear();

    try 
    {
        for(const auto& entry : fs::directory_iterator(dir)) 
        {
            if(entry.is_regular_file()) 
            {
                std::string filename = entry.path().filename().string();
                if(isImageFile(filename))
                {   
                    imageFiles.push_back(filename);
                }
            }
        }

        std::sort(imageFiles.begin(), imageFiles.end());
        std::cout << "Found " << imageFiles.size() << " image(s) in directory: " << dir << '\n';
    } 
    catch(const fs::filesystem_error& e) 
    {
        std::cerr << "Error scanning directory: " << e.what() << '\n';
    }
}

bool ImageViewer::isImageFile(const std::string& filename) 
{
    static const std::vector<std::string> imageExtensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"};
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

    return std::any_of(imageExtensions.begin(), imageExtensions.end(), [&lowerFilename](const std::string& ext) 
    {
        return lowerFilename.size() >= ext.size() && lowerFilename.substr(lowerFilename.size() - ext.size()) == ext;
    });
}
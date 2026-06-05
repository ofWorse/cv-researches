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
    , currentPath("")
    , showOnlyImages(false)
    , showCoordinates(true)
    , hasStartPoint(false)
    , hasEndPoint(false)
{
    currentMouseCoord = ImVec2(0,0);
    currentMouseCoordNorm = ImVec2(0,0);
    startPoint = endPoint = ImVec2(0,0);
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

    currentPath = currentDir;
    scanDirectory(currentDir);
}

std::string ImageViewer::formatFileSize(uintmax_t size)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double fileSize = static_cast<double>(size);
    
    while(fileSize >= 1024.0 && unitIndex < 3)
    {
        fileSize /= 1024.0;
        unitIndex++;
    }
    
    char buffer[64];
    if(unitIndex == 0)
    {
        snprintf(buffer, sizeof(buffer), "%.0f %s", fileSize, units[unitIndex]);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%.1f %s", fileSize, units[unitIndex]);
    }
    
    return std::string(buffer);
}

void ImageViewer::update(void) 
{
    
}

void ImageViewer::copyToClipboard(const std::string& text) 
{
    ImGui::SetClipboardText(text.c_str());
}

void ImageViewer::navigateTo(const std::string& newPath)
{
    if(!fs::exists(newPath) || !fs::is_directory(newPath))
    {
        std::cerr << "Invalid directory: " << newPath << '\n';
        return;
    }
    
    updateNavigationStack(newPath);
    currentDir = newPath;
    currentPath = newPath;
    scanDirectory(currentDir);
    selectedIndex = -1;
    currentImage.clear();
    
    TextureManager::getInstance().unloadTexture("current_image");
}

void ImageViewer::updateNavigationStack(const std::string& newPath)
{
    if(currentPath != newPath)
    {
        backHistory.push(currentPath);
        clearForwardStack();
    }
}

void ImageViewer::clearForwardStack(void)
{
    while(!forwardHistory.empty())
    {
        forwardHistory.pop();
    }
}

void ImageViewer::goBack(void)
{
    if(!backHistory.empty())
    {
        std::string prevPath = backHistory.top();
        backHistory.pop();
        
        forwardHistory.push(currentPath);
        
        currentDir = prevPath;
        currentPath = prevPath;
        scanDirectory(currentDir);
        selectedIndex = -1;
        currentImage.clear();
        TextureManager::getInstance().unloadTexture("current_image");
    }
    else
    {
        std::cout << "No back history available\n";
    }
}

void ImageViewer::goForward(void)
{
    if(!forwardHistory.empty())
    {
        std::string nextPath = forwardHistory.top();
        forwardHistory.pop();
        
        backHistory.push(currentPath);
        
        currentDir = nextPath;
        currentPath = nextPath;
        scanDirectory(currentDir);
        selectedIndex = -1;
        currentImage.clear();
        TextureManager::getInstance().unloadTexture("current_image");
    }
    else
    {
        std::cout << "No forward history available\n";
    }
}

void ImageViewer::scanDirectory(const std::string& dir) 
{
    fileSystemEntries.clear();

    try 
    {
        for(const auto& entry : fs::directory_iterator(dir)) 
        {
            std::string name = entry.path().filename().string();
            std::string path = entry.path().string();
            
            if(entry.is_directory())
            {
                fileSystemEntries.emplace_back(name, path, true, 0);
            }
            else if(entry.is_regular_file() && isImageFile(name))
            {
                fileSystemEntries.emplace_back(name, path, false, fs::file_size(entry.path()));
            }
        }
        
        std::sort(fileSystemEntries.begin(), fileSystemEntries.end(), 
            [](const FileSystemEntry& a, const FileSystemEntry& b) {
                if(a.isDirectory != b.isDirectory)
                {
                    return a.isDirectory > b.isDirectory;
                }
                return a.name < b.name;
            });
        
        std::cout << "Found " << fileSystemEntries.size() << " items in directory: " << dir 
                  << " (Folders: " << std::count_if(fileSystemEntries.begin(), fileSystemEntries.end(), 
                     [](const FileSystemEntry& e) { return e.isDirectory; })
                  << ", Images: " << std::count_if(fileSystemEntries.begin(), fileSystemEntries.end(), 
                     [](const FileSystemEntry& e) { return !e.isDirectory; }) << ")\n";
    } 
    catch(const fs::filesystem_error& e) 
    {
        std::cerr << "Error scanning directory: " << e.what() << '\n';
    }
}

void ImageViewer::renderImageDisplay(void) 
{
    ImGui::BeginChild("ImageDisplay", ImVec2(0, 0), true);

    Texture* texture = TextureManager::getInstance().getTexture("current_image");
    static ImVec2 imageScreenPos;
    static ImVec2 imageDisplaySize;

    if(texture && texture->id) 
    {
        ImGui::Text("[IMG] %s", currentImage.c_str());
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
        
        ImVec2 final_pos = ImVec2(
            cursor_pos.x + center_offset.x + imagePosition.x,
            cursor_pos.y + center_offset.y + imagePosition.y
        );
        ImGui::SetCursorScreenPos(final_pos);
        
        imageScreenPos = final_pos;
        imageDisplaySize = image_size;
        
        ImGui::Image((ImTextureID)(intptr_t)texture->id, image_size);
        
        // --- Отображение координат мыши ---
        if (ImGui::IsItemHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            // вычисляем локальные координаты в текстуре (0..1)
            float u = (mousePos.x - imageScreenPos.x) / imageDisplaySize.x;
            float v = (mousePos.y - imageScreenPos.y) / imageDisplaySize.y;
            if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
                currentMouseCoordNorm = ImVec2(u, v);
                currentMouseCoord = ImVec2(u * texture->width, v * texture->height);
            }
        }
        
        // --- Панель координат и запоминания ---
        ImGui::Separator();
        ImGui::Text("Mouse coordinates: (%.1f, %.1f) px  |  (%.3f, %.3f) rel", 
                    currentMouseCoord.x, currentMouseCoord.y,
                    currentMouseCoordNorm.x, currentMouseCoordNorm.y);
        
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy")) {
            std::string coordText = std::to_string((int)currentMouseCoord.x) + ", " + std::to_string((int)currentMouseCoord.y);
            ImGui::SetClipboardText(coordText.c_str());
        }
        
        ImGui::BeginGroup();
        if (ImGui::Button("Set Start Point")) {
            startPoint = currentMouseCoord;
            hasStartPoint = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Set End Point")) {
            endPoint = currentMouseCoord;
            hasEndPoint = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Points")) {
            hasStartPoint = false;
            hasEndPoint = false;
        }
        ImGui::EndGroup();
        
        if (hasStartPoint) ImGui::Text("Start: (%.1f, %.1f)", startPoint.x, startPoint.y);
        if (hasEndPoint)   ImGui::Text("End:   (%.1f, %.1f)", endPoint.x, endPoint.y);
        
        // --- Обработка панорамирования (правой кнопкой) ---
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

void ImageViewer::renderFileBrowser(void) 
{
    ImGui::BeginChild("FileList", ImVec2(0, 0), true);

    ImGui::Text("Current Directory:");
    ImGui::TextWrapped("%s", currentDir.c_str());
    ImGui::Separator();

    ImGui::BeginGroup();
    
    bool backEnabled = !backHistory.empty();
    if(ImGui::Button("<-- Back", ImVec2(80, 0)) && backEnabled)
    {
        goBack();
    }
    
    ImGui::SameLine();
    
    bool forwardEnabled = !forwardHistory.empty();
    if(ImGui::Button("Forward -->", ImVec2(90, 0)) && forwardEnabled)
    {
        goForward();
    }
    
    ImGui::SameLine();
    
    if(ImGui::Button("[..] Parent"))
    {
        fs::path parent = fs::path(currentDir).parent_path();
        if(!parent.empty() && parent != currentDir)
        {
            navigateTo(parent.string());
        }
    }
    
    ImGui::SameLine();
    
    if(ImGui::Button("[R] Refresh"))
    {        
        scanDirectory(currentDir);
    }
    
    ImGui::SameLine();
    
    if(ImGui::Button("[H] Home"))
    {
        #ifdef __APPLE__
            const char* home = getenv("HOME");
            if(home)
            {
                navigateTo(fs::path(home) / "Pictures");
            }
        #elif defined(_WIN32)
            const char* home = getenv("USERPROFILE");
            if(home)
            {
                navigateTo(fs::path(home) / "Pictures");
            }
        #else
            navigateTo(fs::path(getenv("HOME")) / "Pictures");
        #endif
    }
    
    ImGui::SameLine();
    
    ImGui::Checkbox("Show only images", &showOnlyImages);
    
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    ImGui::Text("Navigation:");
    ImGui::SameLine();
    
    fs::path path = fs::path(currentDir);
    std::vector<fs::path> pathComponents;
    
    for(auto it = path.begin(); it != path.end(); ++it)
    {
        pathComponents.push_back(*it);
    }
    
    fs::path accumulatedPath;
    for(size_t i = 0; i < pathComponents.size(); ++i)
    {
        if(i > 0)
        {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
        }
        
        accumulatedPath /= pathComponents[i];
        std::string dirName = pathComponents[i].string();
        
        if(dirName.empty() && i == 0)
        {
            dirName = (path.has_root_path()) ? path.root_path().string() : "/";
        }
        
        if(ImGui::SmallButton(dirName.c_str()))
        {
            navigateTo(accumulatedPath.string());
            break;
        }
    }
    
    ImGui::Separator();

    size_t foldersCount = 0, imagesCount = 0;
    for(const auto& item : fileSystemEntries)
    {
        if(item.isDirectory) foldersCount++;
        else imagesCount++;
    }
    
    ImGui::Text("[F] Folders: %zu  |  [I] Images: %zu", foldersCount, imagesCount);
    if(imagesCount == 0 && !showOnlyImages)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(No images in this folder)");
    }
    ImGui::Separator();
    
    for (size_t i = 0; i < fileSystemEntries.size(); ++i) 
    {
        const auto& entry = fileSystemEntries[i];
        
        if(showOnlyImages && entry.isDirectory) continue;
        
        std::string display_name;
        ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        if(entry.isDirectory)
        {
            display_name = "[FOLDER] " + entry.name;
            text_color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
        }
        else
        {
            display_name = "[IMG] " + entry.name;
            if(entry.size != 0)
            {
                display_name += " (" + formatFileSize(entry.size) + ")";
            }
            text_color = ImVec4(1.0f, 1.0f, 0.8f, 1.0f);
        }
        
        bool is_selected = !entry.isDirectory && (selectedIndex == static_cast<int>(i));
        
        if(entry.isDirectory)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            if(ImGui::Selectable(display_name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    navigateTo(entry.path);
                }
            }
            ImGui::PopStyleColor();
            
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Double-click to open folder");
                ImGui::Text("Path: %s", entry.path.c_str());
                ImGui::EndTooltip();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
            if(ImGui::Selectable(display_name.c_str(), is_selected))
            {
                selectedIndex = i;
                std::string full_path = entry.path;
                TextureManager::getInstance().loadTexture(full_path, "current_image");
                
                zoomLevel = 1.0f;
                imagePosition = ImVec2(0, 0);
                currentImage = entry.name;
            }
            ImGui::PopStyleColor();
            
            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Click to view image");
                ImGui::Text("Full path: %s", entry.path.c_str());
                ImGui::Text("Size: %s", formatFileSize(entry.size).c_str());
                ImGui::EndTooltip();
            }
        }
    }
    
    ImGui::EndChild();
}

void ImageViewer::render(void) 
{
    if(!isVisible) 
    {
        return;
    }

    handleKeyboardShortcuts();

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
        
        if(ImGui::BeginMenu("Navigate"))
        {
            if(ImGui::MenuItem("Back", "Cmd+[", false, !backHistory.empty()))
            {
                goBack();
            }
            if(ImGui::MenuItem("Forward", "Cmd+]", false, !forwardHistory.empty()))
            {
                goForward();
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

void ImageViewer::handleKeyboardShortcuts(void)
{
    ImGuiIO& io = ImGui::GetIO();
    
    if(ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        goBack();
    }
    
    std::vector<int> imageIndices;
    for(size_t i = 0; i < fileSystemEntries.size(); ++i)
    {
        if(!fileSystemEntries[i].isDirectory)
        {
            imageIndices.push_back(i);
        }
    }
    
    int currentImagePos = -1;
    for(size_t pos = 0; pos < imageIndices.size(); ++pos)
    {
        if(imageIndices[pos] == selectedIndex)
        {
            currentImagePos = pos;
            break;
        }
    }
    
    if(ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentImagePos > 0)
    {
        int newIndex = imageIndices[currentImagePos - 1];
        selectedIndex = newIndex;
        std::string full_path = fileSystemEntries[selectedIndex].path;
        TextureManager::getInstance().loadTexture(full_path, "current_image");
        zoomLevel = 1.0f;
        imagePosition = ImVec2(0, 0);
        currentImage = fileSystemEntries[selectedIndex].name;
    }
    
    if(ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentImagePos < (int)imageIndices.size() - 1 && currentImagePos != -1)
    {
        int newIndex = imageIndices[currentImagePos + 1];
        selectedIndex = newIndex;
        std::string full_path = fileSystemEntries[selectedIndex].path;
        TextureManager::getInstance().loadTexture(full_path, "current_image");
        zoomLevel = 1.0f;
        imagePosition = ImVec2(0, 0);
        currentImage = fileSystemEntries[selectedIndex].name;
    }
    
    if(ImGui::IsKeyPressed(ImGuiKey_Home) && !imageIndices.empty())
    {
        selectedIndex = imageIndices[0];
        std::string full_path = fileSystemEntries[selectedIndex].path;
        TextureManager::getInstance().loadTexture(full_path, "current_image");
        zoomLevel = 1.0f;
        imagePosition = ImVec2(0, 0);
        currentImage = fileSystemEntries[selectedIndex].name;
    }
    
    if(ImGui::IsKeyPressed(ImGuiKey_End) && !imageIndices.empty())
    {
        selectedIndex = imageIndices.back();
        std::string full_path = fileSystemEntries[selectedIndex].path;
        TextureManager::getInstance().loadTexture(full_path, "current_image");
        zoomLevel = 1.0f;
        imagePosition = ImVec2(0, 0);
        currentImage = fileSystemEntries[selectedIndex].name;
    }
}

void ImageViewer::renderToolBar(void) 
{
    ImGui::Separator();
    
    ImGui::Text("[D] %s", currentDir.c_str());
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    
    int imageIndex = -1;
    for(size_t i = 0; i < fileSystemEntries.size(); ++i)
    {
        if(!fileSystemEntries[i].isDirectory && selectedIndex == static_cast<int>(i))
        {
            imageIndex = static_cast<int>(std::count_if(fileSystemEntries.begin(), 
                fileSystemEntries.begin() + i, 
                [](const FileSystemEntry& e) { return !e.isDirectory; }));
            break;
        }
    }
    
    int totalImages = std::count_if(fileSystemEntries.begin(), fileSystemEntries.end(),
        [](const FileSystemEntry& e) { return !e.isDirectory; });
    
    if(imageIndex >= 0 && totalImages > 0)
    {
        ImGui::Text("[I] %d/%d", imageIndex + 1, totalImages);
    }
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
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
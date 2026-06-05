#include "interpolation_window.h"
#include "texture_manager.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

InterpolationWindow::InterpolationWindow(void)
{
    folderPath = "";
    firstBbox = BoundingBox(100, 100, 200, 150);
    lastBbox = BoundingBox(300, 200, 200, 150);
    outputFolder = "";
}

void InterpolationWindow::drawCoordinatesEditor(
    const std::string& label, 
    BoundingBox& bbox,
    float imgW,
    float imgH)
{
    ImGui::Text("%s", label.c_str());
    ImGui::PushID(label.c_str());
    ImGui::SliderFloat("X", &bbox.x, 0.0f, imgW);
    ImGui::SliderFloat("Y", &bbox.y, 0.0f, imgH);
    ImGui::SliderFloat("Width", &bbox.w, 10.0f, imgW - bbox.x);
    ImGui::SliderFloat("Height", &bbox.h, 10.0f, imgH - bbox.y);
    ImGui::PopID();
}

void InterpolationWindow::render(void)
{
    if(!isVisible) return;
    
    ImGui::Begin("Sequence Interpolation", &isVisible);
    ImGui::Text("Folder with image sequence:");

    char folderBuf[512];
    strncpy(folderBuf, folderPath.c_str(), sizeof(folderBuf));
    if(ImGui::InputText("##folder", folderBuf, sizeof(folderBuf))) 
    {
        folderPath = folderBuf;
    }
    ImGui::SameLine();
    if(ImGui::Button("Browse...")) 
    {
        std::cout << "Choose folder manually\n";
    }
    
    if(!folderPath.empty() && fs::exists(folderPath))
     {
        std::vector<std::string> images;
        for(const auto& entry : fs::directory_iterator(folderPath))
         {
            if(entry.is_regular_file()) 
            {
                std::string ext = entry.path().extension().string();
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp")
                    images.push_back(entry.path().string());
            }
        }
        std::sort(images.begin(), images.end());
        if(!images.empty()) 
        {
            cv::Mat first = cv::imread(images.front());
            if(!first.empty())
            {
                imageWidth = first.cols;
                imageHeight = first.rows;
                useFirstImageSize = true;
            }
        }
    }
    
    ImGui::Separator();
    ImGui::Text("First frame bounding box:");
    drawCoordinatesEditor("##first", firstBbox, static_cast<float>(imageWidth), static_cast<float>(imageHeight));
    ImGui::Text("Last frame bounding box:");
    drawCoordinatesEditor("##last", lastBbox, static_cast<float>(imageWidth), static_cast<float>(imageHeight));
    
    ImGui::Separator();
    ImGui::Text("Interpolation method:");
    const char* methods[] = {"Linear", "Smooth step"};
    ImGui::Combo("##method", &currentMethod, methods, IM_ARRAYSIZE(methods));
    
    ImGui::Text("Output folder (leave empty to just display):");
    char outBuf[512];
    strncpy(outBuf, outputFolder.c_str(), sizeof(outBuf));
    if(ImGui::InputText("##output", outBuf, sizeof(outBuf))) 
    {
        outputFolder = outBuf;
    }
    
    if(ImGui::Button("Run Interpolation") && !folderPath.empty()) 
    {
        processing = true;
        InterpolationMethod method = (currentMethod == 0) ? InterpolationMethod::LINEAR : InterpolationMethod::SMOOTH_STEP;
        Interpolator::processSequence(folderPath, firstBbox, lastBbox, method, outputFolder);
        processing = false;
    }
    
    if(processing)
    {
        ImGui::Text("Processing...");
    }
    
    ImGui::End();
}
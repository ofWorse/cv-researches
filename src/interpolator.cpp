#include "interpolator.h"
#include <filesystem>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

namespace fs = std::filesystem;

std::vector<BoundingBox> Interpolator::interpolateLinear(
    const BoundingBox& start,
    const BoundingBox& end,
    int numFrames
) 
{
    std::vector<BoundingBox> result(numFrames);
    for (int i = 0; i < numFrames; ++i) 
    {
        float t = (numFrames == 1) ? 0.0f : static_cast<float>(i) / (numFrames - 1);
        result[i].x = start.x + t * (end.x - start.x);
        result[i].y = start.y + t * (end.y - start.y);
        result[i].w = start.w + t * (end.w - start.w);
        result[i].h = start.h + t * (end.h - start.h);
    }

    return result;
}

std::vector<BoundingBox> Interpolator::interpolateSmoothStep(
    const BoundingBox& start,
    const BoundingBox& end,
    int numFrames
) 
{
    std::vector<BoundingBox> result(numFrames);
    for (int i = 0; i < numFrames; ++i) 
    {
        float t = (numFrames == 1) ? 0.0f : static_cast<float>(i) / (numFrames - 1);
        float st = t * t * (3.0f - 2.0f * t);
        result[i].x = start.x + st * (end.x - start.x);
        result[i].y = start.y + st * (end.y - start.y);
        result[i].w = start.w + st * (end.w - start.w);
        result[i].h = start.h + st * (end.h - start.h);
    }

    return result;
}

std::vector<BoundingBox> Interpolator::interpolateCubicSpline(
    const std::vector<BoundingBox>& keyFrames,
    const std::vector<int>& keyIndices,
    int totalFrames
)
{
    if(keyFrames.size()==2)
    {
        return interpolateLinear(keyFrames[0], keyFrames[1], totalFrames);
    }

    // TODO
    std::cerr << "NOT IMPLEMENTED YET\n";
    return interpolateLinear(keyFrames.front(), keyFrames.back(), totalFrames);
}

void Interpolator::processSequence(
    const std::string& folderPath,
    const BoundingBox& firstBbox,
    const BoundingBox& lastBbox,
    InterpolationMethod method,
    const std::string& outputFolder
)
{
    std::vector<std::string> imageFiles;
    for(const auto& entry : fs::directory_iterator(folderPath))
    {
        if(entry.is_regular_file())
        {
            std::string ext = entry.path().extension().string();
            if(    ext == ".jpg" 
                || ext == ".jpeg"
                || ext == ".png"
                || ext == ".bmp"
                || ext == ".tiff"
                || ext == ".tif") 
            {
                imageFiles.push_back(entry.path().string());
            }
        }
    }

    std::sort(imageFiles.begin(), imageFiles.end());

    if(imageFiles.size() < 2)
    {
        std::cerr << "At least 2 images in the folder needed.\n";
        return;
    }

    int totalFrames = imageFiles.size();
    std::vector<BoundingBox> bboxes;

    switch(method)
    {
    case InterpolationMethod::SMOOTH_STEP:
        bboxes = interpolateSmoothStep(firstBbox, lastBbox, totalFrames);
        break;
    case InterpolationMethod::LINEAR:
        [[fallthrough]];
    default:
        bboxes = interpolateLinear(firstBbox, lastBbox, totalFrames);
        break;
    }

    std::string outDir = outputFolder.empty() ? (folderPath + "/interpolated") : outputFolder;
    if(!outputFolder.empty() && !fs::exists(outDir))
    {
        fs::create_directories(outDir);
    }

    for(int i = 0; i < totalFrames; ++i)
    {
        cv::Mat img = cv::imread(imageFiles[i]);
        if(img.empty()) continue;

        const BoundingBox& box = bboxes[i];
        cv::Point topLeft(static_cast<int>(box.x), static_cast<int>(box.y));
        cv::Point bottomRight(static_cast<int>(box.x + box.w), static_cast<int>(box.y + box.h));
        cv::rectangle(img, topLeft, bottomRight, cv::Scalar(0, 255, 0), 2);
        cv::circle(img, cv::Point(static_cast<int>(box.x + box.w/2), static_cast<int>(box.y + box.h/2)), 3, cv::Scalar(0, 0, 255), -1);
        
        if(!outputFolder.empty()) 
        {
            fs::path outPath = fs::path(outDir) / fs::path(imageFiles[i]).filename();
            cv::imwrite(outPath.string(), img);
        } 
        else 
        {
            cv::imshow("Frame " + std::to_string(i), img);
            cv::waitKey(100);
        }
    }
    if(outputFolder.empty()) cv::destroyAllWindows();
}
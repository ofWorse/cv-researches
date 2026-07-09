#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <queue>

struct Box
{
    int minx, miny, maxx, maxy;
    int pixels{0};

    int centerX(void) const { return (minx + maxx) / 2; }
    int centerY(void) const { return (miny + maxy) / 2; }
    int width(void)   const { return maxx - minx; }
    int height(void)  const { return maxy - miny; }
};


std::vector<Box> findConnectedComponents(const cv::Mat& binary, int min_pixels = 50);
std::vector<Box> mergeNearbyBoxes(const std::vector<Box>& boxes, int distance_threshold = 20);


int main(int argc, char** argv)
{
    cv::VideoCapture video;
    int threshold = 10;
    int min_object_size = 100;
    
    switch(argc)
    {
    case 1:
        video.open(0);
        break;
    case 2:
        video.open(argv[1]);
        break;
    case 3:
        video.open(argv[1]);
        threshold = std::stoi(argv[2]);
        break;
    default:
        std::cout << "Usage:\n";
        std::cout << argv[0] << "                      # camera\n";
        std::cout << argv[0] << " video.(mp4/mov)      # video\n";
        std::cout << argv[0] << " video.(mp4/mov) N    # video. N - threshold\n";
        return -1;
    }

    if(!video.isOpened())
    {
        std::cerr << "Error: unable to open video\n";
        return -1;
    }

    cv::Mat prev, curr, diff, result, debug;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));

    if(!video.read(prev))
    {
        std::cerr << "Error: unable to read first frame\n";
        return -1;
    }

    while(video.read(curr))
    {
        diff = cv::Mat(prev.rows, prev.cols, CV_8UC1);
        result = cv::Mat(prev.rows, prev.cols, CV_8UC1);
        
        cv::Mat prev_gray, curr_gray;
        cv::cvtColor(prev, prev_gray, cv::COLOR_BGR2GRAY);
        cv::cvtColor(curr, curr_gray, cv::COLOR_BGR2GRAY);
        
        cv::absdiff(prev_gray, curr_gray, diff);
        
        cv::threshold(diff, result, threshold, 255, cv::THRESH_BINARY);
        
        cv::morphologyEx(result, result, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(result, result, cv::MORPH_CLOSE, kernel);
        
        std::vector<Box> boxes = findConnectedComponents(result, min_object_size);
        
        boxes = mergeNearbyBoxes(boxes, 30);
        
        debug = curr.clone();
        
        for(const auto& box : boxes)
        {
            if(box.pixels < min_object_size)
            {
                continue;
            }
            
            cv::rectangle(
                debug,
                cv::Point(box.minx, box.miny),
                cv::Point(box.maxx, box.maxy),
                cv::Scalar(0, 0, 255), // Красный цвет
                2
            );
        }
        
        std::string total_info = "Objects: " + std::to_string(boxes.size()) + 
                                 " | Threshold: " + std::to_string(threshold);
        cv::putText(debug, total_info,
                   cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX,
                   0.8,
                   cv::Scalar(255, 255, 255),
                   2);
        
        cv::imshow("Original", curr);
        cv::imshow("Difference", result);
        cv::imshow("Detection", debug);
        
        char key = cv::waitKey(30);
        if(key == 27)
        {
            break;
        }
        else if(key == '+')
        {
            threshold += 2;
            std::cout << "Threshold: " << threshold << std::endl;
        }
        else if(key == '-')
        {
            threshold = std::max(0, threshold - 2);
            std::cout << "Threshold: " << threshold << std::endl;
        }

        prev = curr.clone();
    }

    cv::destroyAllWindows();
    return 0;
}

std::vector<Box> findConnectedComponents(const cv::Mat& binary, int min_pixels)
{
    std::vector<Box> boxes;
    
    if(binary.empty())
    {
        return boxes;
    }
    
    cv::Mat labels = cv::Mat::zeros(binary.size(), CV_32S);
    int label_count = 0;
    
    std::vector<std::vector<cv::Point>> components;
    
    for(int y = 0; y < binary.rows; ++y)
    {
        const uchar* row = binary.ptr<uchar>(y);
        for(int x = 0; x < binary.cols; ++x)
        {
            if(row[x] == 255 && labels.at<int>(y, x) == 0)
            {
                label_count++;
                std::queue<cv::Point> queue;
                queue.push(cv::Point(x, y));
                labels.at<int>(y, x) = label_count;
                
                Box box;
                box.minx = binary.cols;
                box.miny = binary.rows;
                box.maxx = 0;
                box.maxy = 0;
                box.pixels = 0;
                
                while(!queue.empty())
                {
                    cv::Point p = queue.front();
                    queue.pop();
                    
                    box.pixels++;
                    if (p.x < box.minx) box.minx = p.x;
                    if (p.x > box.maxx) box.maxx = p.x;
                    if (p.y < box.miny) box.miny = p.y;
                    if (p.y > box.maxy) box.maxy = p.y;
                    
                    for(int dy = -1; dy <= 1; ++dy)
                    {
                        for(int dx = -1; dx <= 1; ++dx)
                        {
                            if(dx == 0 && dy == 0) continue;
                            
                            int nx = p.x + dx;
                            int ny = p.y + dy;
                            
                            if(nx >= 0 && nx < binary.cols && 
                                ny >= 0 && ny < binary.rows &&
                                binary.at<uchar>(ny, nx) == 255 &&
                                labels.at<int>(ny, nx) == 0)
                            {
                                labels.at<int>(ny, nx) = label_count;
                                queue.push(cv::Point(nx, ny));
                            }
                        }
                    }
                }
                
                if(box.pixels >= min_pixels)
                {
                    boxes.push_back(box);
                }
            }
        }
    }
    
    return boxes;
}

std::vector<Box> mergeNearbyBoxes(const std::vector<Box>& boxes, int distance_threshold)
{
    if(boxes.empty())
    {
        return boxes;
    }
    
    std::vector<Box> merged_boxes = boxes;
    bool merged = true;
    
    while(merged)
    {
        merged = false;
        std::vector<Box> new_boxes;
        
        for(size_t i = 0; i < merged_boxes.size(); ++i)
        {
            bool is_merged = false;
            
            for(size_t j = i + 1; j < merged_boxes.size(); ++j)
            {
                int minx = std::min(merged_boxes[i].minx, merged_boxes[j].minx);
                int maxx = std::max(merged_boxes[i].maxx, merged_boxes[j].maxx);
                int miny = std::min(merged_boxes[i].miny, merged_boxes[j].miny);
                int maxy = std::max(merged_boxes[i].maxy, merged_boxes[j].maxy);
                
                int gap_x = std::max(0, std::min(merged_boxes[i].maxx, merged_boxes[j].maxx) - 
                                        std::max(merged_boxes[i].minx, merged_boxes[j].minx));
                int gap_y = std::max(0, std::min(merged_boxes[i].maxy, merged_boxes[j].maxy) - 
                                        std::max(merged_boxes[i].miny, merged_boxes[j].miny));
                
                if((gap_x > -distance_threshold && gap_y > -distance_threshold) ||
                    (std::abs(merged_boxes[i].centerX() - merged_boxes[j].centerX()) < distance_threshold * 2 &&
                     std::abs(merged_boxes[i].centerY() - merged_boxes[j].centerY()) < distance_threshold * 2))
                {
                    Box merged;
                    merged.minx = minx;
                    merged.miny = miny;
                    merged.maxx = maxx;
                    merged.maxy = maxy;
                    merged.pixels = merged_boxes[i].pixels + merged_boxes[j].pixels;
                    
                    new_boxes.push_back(merged);
                    is_merged = true;
                    break;
                }
            }
            
            if(!is_merged)
            {
                new_boxes.push_back(merged_boxes[i]);
            }
        }
        
        merged_boxes = new_boxes;
    }
    
    return merged_boxes;
}

#pragma once

#include <filesystem>
#include <opencv2/opencv.hpp>
#include "src/Args/ParseArgs.h"

void SRGBToLinear(cv::Mat& src);
void LinearToSRGB(cv::Mat& src);
cv::Mat GetThumbnailImage(const std::filesystem::path& path, Size thumbnailSize);

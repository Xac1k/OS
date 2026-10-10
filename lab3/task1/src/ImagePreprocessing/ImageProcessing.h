#pragma once

#include <filesystem>
#include <opencv2/opencv.hpp>

void SRGBToLinear(cv::Mat& src);
cv::Mat GetPreparedImage(const std::filesystem::path& path);
double ComputeMSE(const cv::Mat& reference, const cv::Mat& comparable);
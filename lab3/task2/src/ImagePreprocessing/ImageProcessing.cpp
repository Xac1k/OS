#include "ImageProcessing.h"
#include "src/Args/ParseArgs.h"

void SRGBToLinear(cv::Mat& src) {
    cv::Mat converted;
    src.convertTo(converted, CV_32F, 1.0 / 255.0);
    cv::pow(converted, 2.2, src);
}

void LinearToSRGB(cv::Mat& src) {
    cv::Mat converted;
    cv::pow(src, 1. / 2.2, converted);
    converted.convertTo(src, CV_8U, 255.0);
}

cv::Mat GetThumbnailImage(const std::filesystem::path& path, Size thumbnailSize) {
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        throw std::runtime_error("Failed to load image: " + path.string());
    }

    SRGBToLinear(img);
    cv::resize(img, img, cv::Size(static_cast<int>(thumbnailSize.width), static_cast<int>(thumbnailSize.height)), 0, 0, cv::INTER_LINEAR);
    LinearToSRGB(img);

    return img;
}
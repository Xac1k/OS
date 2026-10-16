#include "ImageProcessing.h"

constexpr auto IMAGE_SIZE = 256;

void SRGBToLinear(cv::Mat& src) {
    cv::Mat converted;
    src.convertTo(converted, CV_32F, 1.0 / 255.0);
    cv::pow(converted, 2.2, src);
}

void LinearToSRGB(cv::Mat& src) {
    cv::Mat converted;
    src.convertTo(converted, CV_32F, 1.0 / 255.0);
    cv::pow(converted, 1 / 2.2, src);
}

cv::Mat GetPreparedImage(const std::filesystem::path& path) {
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        throw std::runtime_error("Failed to load image: " + path.string());
    }

    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    SRGBToLinear(img);
    cv::resize(img, img, cv::Size(IMAGE_SIZE, IMAGE_SIZE), 0, 0, cv::INTER_LINEAR);

    return img;
}

double ComputeMSE(const cv::Mat& reference, const cv::Mat& comparable) {
    if ( reference.size != comparable.size || reference.type() != comparable.type() || comparable.size[0] != IMAGE_SIZE ) {
        throw std::runtime_error("Incompatible images");
    }

    CV_Assert(reference.type() == CV_32FC3);

    const int channels = reference.channels();

    double mse = 0;

    constexpr auto divider = 1.0 / (3 * IMAGE_SIZE * IMAGE_SIZE);
    for (auto x = 0; x < reference.cols; ++x) {
        for (auto y = 0; y < reference.rows; ++y) {
            auto pixel1 = reference.at<cv::Vec3f>(y, x);
            auto pixel2 = comparable.at<cv::Vec3f>(y, x);

            for (auto c = 0; c < channels; ++c) {
                const double diff = pixel1[c] - pixel2[c];
                mse += (diff * diff) * divider;
            }
        }
    }

    return mse;
}

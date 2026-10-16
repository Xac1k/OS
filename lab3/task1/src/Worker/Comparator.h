#pragma once
#include <filesystem>
#include <vector>
#include <opencv2/core/mat.hpp>

#include "src/Common/BaseThread.h"

struct Result {
    std::filesystem::path path;
    double mse;
};

class Comparator : public BaseThread<Comparator> {
public:
    Comparator(const std::vector<std::filesystem::path>& files, cv::Mat  reference);
    void WorkerLoop(const std::stop_token& stopToken);
    [[nodiscard]] std::vector<Result> Results() const;
private:
    std::vector<std::filesystem::path> m_files;
    cv::Mat m_reference;

    std::vector<Result> m_results;
};

#include "Comparator.h"

#include "src/ImagePreprocessing/ImageProcessing.h"

Comparator::Comparator(const std::vector<std::filesystem::path>& files, const cv::Mat& reference)
: m_files(files), m_reference(reference) {}

void Comparator::WorkerLoop() {
    for (const auto& file : m_files) {
        auto comparable = GetPreparedImage(file);

        auto mse = ComputeMSE(m_reference, comparable);
        m_results.emplace_back(file, mse);
    }
}

std::vector<Result> Comparator::Results() const {
    return m_results;
}

#include "Comparator.h"

#include <utility>

#include "src/ImagePreprocessing/ImageProcessing.h"

Comparator::Comparator(const std::vector<std::filesystem::path>& files, cv::Mat  reference)
: m_files(files), m_reference(std::move(reference)) {}

void Comparator::WorkerLoop(const std::stop_token& stopToken) {
    for (const auto& file : m_files) {
        if (stopToken.stop_requested()) break;

    }
}

std::vector<Result> Comparator::Results() const {
    return m_results;
}

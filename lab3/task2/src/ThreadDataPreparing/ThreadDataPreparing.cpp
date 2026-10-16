#include "ThreadDataPreparing.h"

#include <algorithm>

std::vector<std::filesystem::path> RecursiveCollectingFiles(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(root)) {
        throw std::runtime_error("Path does not exist: " + root.string());
    }
    if (!std::filesystem::is_directory(root)) {
        throw std::runtime_error("Path is not a directory: " + root.string());
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() && (
            entry.path().extension() == ".jpg" ||
            entry.path().extension() == ".png"
        )) {
            files.push_back(entry.path());
        }
    }

    return files;
}

std::vector<std::filesystem::path> GetFilesForWorkers(const std::filesystem::path& root) {
    auto files = RecursiveCollectingFiles(root);
    std::ranges::sort(files);

    return files;
}
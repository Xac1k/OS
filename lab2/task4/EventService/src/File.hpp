#pragma once
#include <fstream>
#include <string>
#include <filesystem>
#include <concepts>
#include <vector>

inline std::string BuildPath(const std::string &path, const int levelsUp = 0) {
    const std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path targetPath = currentPath;

    for (int i = 0; i < levelsUp; i++) {
        targetPath = targetPath.parent_path();
    }

    targetPath /= path;
    return targetPath.string();
}

template <typename FileType>
    requires std::same_as<FileType, std::ifstream> || std::same_as<FileType, std::ofstream>
void OpenFile(const std::string rpath, FileType& file, const int levelsUp = 0) {
    auto gpath = BuildPath(rpath, levelsUp);
    file.open(gpath);
    if(!file.is_open()) {
        throw std::runtime_error("Couldn't open file with path:" + gpath + "\n");
    }
}


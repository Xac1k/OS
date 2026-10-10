#pragma once
#include <filesystem>
#include <vector>

std::vector<std::vector<std::filesystem::path>> GetFilesForWorkers(const std::filesystem::path& root, const size_t workers);

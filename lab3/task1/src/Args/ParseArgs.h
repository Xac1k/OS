#pragma once
#include <filesystem>
#include <string>
#include <bits/chrono.h>

struct InputArgs {
    std::filesystem::path queryImage;
    std::filesystem::path inputDir;

    size_t threads = 1;
    size_t top = std::numeric_limits<size_t>::max();
    double threshold = std::numeric_limits<double>::max();
};

InputArgs ParseArgs(int argc, char** argv);

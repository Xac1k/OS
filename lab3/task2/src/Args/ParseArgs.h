#pragma once
#include <filesystem>
#include <string>
#include <bits/chrono.h>

struct Size {
    size_t height;
    size_t width;
};

struct InputArgs {
    std::filesystem::path inputDir;
    std::filesystem::path outputDir;
    Size thumbnailSize;

    size_t threads = 1;
};

InputArgs ParseArgs(int argc, char** argv);

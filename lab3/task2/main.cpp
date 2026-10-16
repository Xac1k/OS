#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include "src/Args/ParseArgs.h"
#include "src/ImagePreprocessing/ImageProcessing.h"
#include "src/ThreadDataPreparing/ThreadDataPreparing.h"

struct Result {
    size_t succeeded;
    size_t failed;
};

std::filesystem::path GetRelativePathFromBase(const std::filesystem::path& base, const std::filesystem::path& includingPath) {
    auto absBase = std::filesystem::weakly_canonical(base);
    auto absPath = std::filesystem::weakly_canonical(includingPath);

    auto baseIt = absBase.begin();
    auto pathIt = absPath.begin();
    for (; baseIt != absBase.end(); ++baseIt, ++pathIt) {
        if (pathIt == absPath.end() || *pathIt != *baseIt) {
            throw std::logic_error(
                "Path '" + includingPath.string() +
                "' is not inside base '" + base.string() + "'");
        }
    }

    std::filesystem::path relative;
    for (; pathIt != absPath.end(); ++pathIt) {
        relative /= *pathIt;
    }
    return relative;
}

Result SolveWithinFewThread(
    const InputArgs& args,
    const std::vector<std::filesystem::path>& filesForWorkers
) {
    boost::asio::thread_pool pool(args.threads);
    std::atomic<size_t> failed = 0;

    for (const auto& file : filesForWorkers) {
        boost::asio::post(pool, [file, args, &failed]() {
            try {
                const auto thumbnail = GetThumbnailImage(file, args.thumbnailSize);
                auto thumbnailPath = args.outputDir / GetRelativePathFromBase(args.inputDir, file);
                thumbnailPath.replace_filename(thumbnailPath.stem().string() + "_thumb.png");

                std::filesystem::create_directories(thumbnailPath.parent_path());
                if (!cv::imwrite(thumbnailPath.string(), thumbnail)) {
                    failed.fetch_add(1, std::memory_order_relaxed);
                };
            } catch (...) {
                failed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    pool.join();
    return {filesForWorkers.size() - failed, failed};
};

int main(const int argc, char** argv) {
    try {
        const auto args = ParseArgs(argc, argv);
        const auto files = GetFilesForWorkers(args.inputDir);
        auto now = std::chrono::steady_clock::now();
        const auto results = SolveWithinFewThread(args, files);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - now).count();
        std::cout << duration << " us\n";

        std::cout << "processed=" << results.succeeded << " failed=" << results.failed << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
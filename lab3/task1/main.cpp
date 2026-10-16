#include "src/Args/ParseArgs.h"
#include "src/ImagePreprocessing/ImageProcessing.h"
#include "src/ThreadDataPreparing/ThreadDataPreparing.h"
#include "src/Worker/Comparator.h"

std::vector<Result> SolveWithinFewThread(
    const cv::Mat& queryImage,
    const std::vector<std::vector<std::filesystem::path>>& filesForWorkers,
    const size_t threads
) {
    std::vector<std::unique_ptr<Comparator>> workers;
    for (int threadID = 0; threadID < threads; ++threadID) {
        auto comparator = std::make_unique<Comparator>(filesForWorkers.at(threadID), queryImage);
        comparator->Start();
        workers.push_back(std::move(comparator));
    }

    std::vector<Result> results;
    for (int threadID = 0; threadID < threads; ++threadID) {
        workers.at(threadID)->Wait();
        auto threadResults = workers.at(threadID)->Results();
        results.insert(results.end(), threadResults.begin(), threadResults.end());
    }

    std::ranges::sort(results, [](const Result& first, const Result& second) {
       return first.mse < second.mse;
    });

    return results;
};

void PrintResults(const InputArgs& args, const std::vector<Result>& results) {
    size_t printedOutputs = 0;

    for (const auto& [path, mse] : results) {
        if (mse > args.threshold) continue;

        std::cout << mse << " " << path.string() << std::endl;

        if (++printedOutputs == args.top) break;
    }
}

int main(const int argc, char** argv) {
    try {
        const auto args = ParseArgs(argc, argv);

        const auto queryImage = GetPreparedImage(args.queryImage);
        const auto filesForWorkers = GetFilesForWorkers(args.inputDir, args.threads);

        auto start = std::chrono::steady_clock::now();
        const auto results = SolveWithinFewThread(queryImage, filesForWorkers, args.threads);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Кол-во файлов; Время выполнения в us;" << std::endl;
        std::cout << results.size() << " " << duration.count() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
#include <thread>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include "src/Args/ParseArgs.h"
#include "src/ImagePreprocessing/ImageProcessing.h"
#include "src/ThreadDataPreparing/ThreadDataPreparing.h"

struct Result {
    std::filesystem::path path;
    double mse;
};

std::vector<Result> SolveWithinFewThread(
    const cv::Mat& queryImage,
    const std::vector<std::filesystem::path>& filesForWorkers,
    const size_t threads
) {
    std::vector<Result> results;
    std::mutex resultsMutex;
    boost::asio::thread_pool pool(threads); //Почему увеличение потоков в 8 раз не даёт увеличение в 8 раз.
    //Законы М Далла. Какая часть работы реально распараллеливается?

    // S = 1 / ((1 - P) + P/N)
    // 8339624 / 2316618  = 1 / ((1 - P) + P/16)
    // - 3.599913322 * P + 3.599913322/16 * P = 1 - 3.599913322
    // −3.374918739P = −2.599913322
    // P = −2.599913322 / −3.374918739
    // P = 0.770363236

    for (const auto& file : filesForWorkers) {
        boost::asio::post(pool, [&results, &queryImage, file, &resultsMutex]() {
            try {
                const auto comparable = GetPreparedImage(file);
                auto mse = ComputeMSE(queryImage, comparable);
                {
                    std::lock_guard lock(resultsMutex);
                    results.emplace_back(file, mse);
                }
            } catch (...) {}
        });
    }

    pool.join();

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
        const auto filesForWorkers = GetFilesForWorkers(args.inputDir);

        const auto start = std::chrono::steady_clock::now();
        const auto results = SolveWithinFewThread(queryImage, filesForWorkers, args.threads);
        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Кол-во файлов; Время выполнения в us;" << std::endl;
        std::cout << results.size() << " " << duration.count() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
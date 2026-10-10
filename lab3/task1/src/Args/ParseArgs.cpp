#include "ParseArgs.h"

#include <stdexcept>

using namespace std::literals;

void ValidatePath(const InputArgs& args) {
    namespace fs = std::filesystem;

    if (!fs::exists(args.queryImage)) {
        throw std::logic_error(
            "queryImage does not exist: " + fs::absolute(args.queryImage).string());
    }
    if (!fs::is_regular_file(args.queryImage)) {
        throw std::logic_error(
            "queryImage is not a regular file: " + fs::absolute(args.queryImage).string());
    }
    if (!fs::exists(args.inputDir)) {
        throw std::logic_error(
            "inputDir does not exist: " + fs::absolute(args.inputDir).string());
    }
    if (!fs::is_directory(args.inputDir)) {
        throw std::logic_error(
            "inputDir is not a directory: " + fs::absolute(args.inputDir).string());
    }
}

InputArgs ParseArgs(const int argc, char** argv) {
    if (argc < 3) {
        throw std::logic_error(
            "Usage: ./SimilarImageFinder QUERY_IMAGE INPUT_DIR [-j N] [--top K] [--threshold T]");
    }

    InputArgs args;
    bool hasThreads = false;
    bool hasTop = false;
    bool hasThreshold = false;

    enum class State {
        ExpectQuery,
        ExpectInputDir,
        ExpectFlagOrEnd,
        ExpectValueThreads,
        ExpectValueTop,
        ExpectValueThreshold,
        Done,
    };

    State state = State::ExpectQuery;

    for (int i = 1; i < argc; ++i) {
        const std::string token = argv[i];

        switch (state) {
        case State::ExpectQuery:
            args.queryImage = token;
            state = State::ExpectInputDir;
            break;

        case State::ExpectInputDir:
            args.inputDir = token;
            state = State::ExpectFlagOrEnd;
            break;

        case State::ExpectFlagOrEnd:
            if (token == "-j") {
                state = State::ExpectValueThreads;
            } else if (token == "--top") {
                state = State::ExpectValueTop;
            } else if (token == "--threshold") {
                state = State::ExpectValueThreshold;
            } else if (!token.empty() && token[0] == '-') {
                throw std::logic_error("Unknown flag: " + token);
            } else {
                throw std::logic_error(
                    "Unexpected positional argument: " + token);
            }
            break;

        case State::ExpectValueThreads:
            if (hasThreads) throw std::logic_error("Duplicate flag: -j");
            args.threads = std::stoul(token);
            if (args.threads == 0)
                throw std::logic_error("-j must be > 0");
            hasThreads = true;
            state = State::ExpectFlagOrEnd;
            break;

        case State::ExpectValueTop:
            if (hasTop) throw std::logic_error("Duplicate flag: --top");
            args.top = std::stoi(token);
            if (args.top <= 0)
                throw std::logic_error("--top must be > 0");
            hasTop = true;
            state = State::ExpectFlagOrEnd;
            break;

        case State::ExpectValueThreshold:
            if (hasThreshold) throw std::logic_error("Duplicate flag: --threshold");
            args.threshold = std::stod(token);
            if (args.threshold < 0.0)
                throw std::logic_error("--threshold must be >= 0");
            hasThreshold = true;
            state = State::ExpectFlagOrEnd;
            break;

        case State::Done:
            throw std::logic_error("Internal parser error: reached Done early");
        }
    }

    if (state != State::ExpectFlagOrEnd && state != State::Done) {
        throw std::logic_error(
            "Unexpected end of arguments: expected a value after a flag");
    }

    ValidatePath(args);
    return args;
}

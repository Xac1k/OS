#include "ParseArgs.h"

#include <stdexcept>

using namespace std::literals;

void ValidatePath(const InputArgs& args) {
    namespace fs = std::filesystem;

    if (!fs::exists(args.inputDir)) {
        throw std::logic_error(
            "inputDir does not exist: " + fs::absolute(args.inputDir).string());
    }
    if (!fs::is_directory(args.inputDir)) {
        throw std::logic_error(
            "inputDir is not a directory: " + fs::absolute(args.inputDir).string());
    }

    if (!fs::exists(args.outputDir)) {
        throw std::logic_error(
            "outputDir does not exist: " + fs::absolute(args.outputDir).string());
    }
    if (!fs::is_directory(args.outputDir)) {
        throw std::logic_error(
            "outputDir is not a directory: " + fs::absolute(args.outputDir).string());
    }
}

InputArgs ParseArgs(const int argc, char** argv) {
    if (argc < 4) {
        throw std::logic_error(
            "Usage: ./thumbgen INPUT_DIR OUTPUT_DIR --size=WxH [-j N]");
    }

    InputArgs args;
    enum class State {
        ExpectInputDir,
        ExpectOutputDir,
        ExpectValueThumbSize,
        ExpectFlagOrEnd,
        ExpectValueThreads,
        Done,
    };

    State state = State::ExpectInputDir;

    for (int i = 1; i < argc; ++i) {
        const std::string token = argv[i];

        switch (state) {
        case State::ExpectInputDir:
            args.inputDir = token;
            state = State::ExpectOutputDir;
            break;

        case State::ExpectOutputDir:
            args.outputDir = token;
            state = State::ExpectValueThumbSize;
            break;

        case State::ExpectValueThumbSize: {
            auto labeledValues = std::string_view(token);
            if (!labeledValues.starts_with("--size=")) {
                throw std::logic_error("Expected value for --size=WxH");
            }
            auto values = labeledValues.substr(7);
            const auto x = values.find('x');
            if (x == std::string_view::npos) {
                throw std::logic_error("Can't find x delimiter in size value");
            }

            const auto width = values.substr(0, x);
            const auto height = values.substr(x + 1);

            args.thumbnailSize = {std::stoul(std::string(width)), std::stoul(std::string(height))};
            state = State::ExpectFlagOrEnd;
            break;
        }

        case State::ExpectFlagOrEnd:
            if (token == "-j") {
                state = State::ExpectValueThreads;
            } else if (!token.empty() && token[0] == '-') {
                throw std::logic_error("Unknown flag: " + token);
            } else {
                throw std::logic_error(
                    "Unexpected positional argument: " + token);
            }
            break;

        case State::ExpectValueThreads:
            args.threads = std::stoul(token);
            if (args.threads == 0)
                throw std::logic_error("-j must be > 0");
            state = State::Done;
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
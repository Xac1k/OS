#include <chrono>
#include <iostream>
#include <ostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <format>
#include <csignal>
#include <cstring>

#include "src/File.hpp"
#include <Shared/EventInfo.h>
#include <Shared/FIFORAII/FIFORAII.h>

constexpr auto SHUTDOWN_MSG = "The shutdown has been proceeded.";
constexpr auto DATA_WAS_RECEIVED = "The data was received.";

struct Input {
    std::string pathToFiFo;
};
Input ArgumentsParse(const int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_fifo>" << std::endl;
    }
    Input input;
    input.pathToFiFo = argv[1];

    return input;
}

std::string CurrentDateTime() {
    const auto now = std::chrono::current_zone()->to_local(
        std::chrono::system_clock::now()
    );
    auto sec = std::chrono::floor<std::chrono::seconds>(now);
    return std::format("{:%Y-%m-%d %H:%M:%S}", sec);
}

void PrintMessage(const EventInfo& event, std::ostream& os) {
    os << CurrentDateTime() << " [" << EventLevelToString(event.level) << "] "
        << event.senderPID << " ";
    os.write(event.message, MAX_MESSAGE_LENGTH_WITH_NULL);
    os << std::endl;
}

struct InputData {
    std::ofstream eventLog;
    FifoRaii requestFiFo;
};

InputData GetInputData(const int argc, char *argv[]) {
    auto input = ArgumentsParse(argc, argv);

    FifoRaii fifo(input.pathToFiFo, true);
    std::cout << "event-service: listening on " << input.pathToFiFo << std::endl << std::endl;
    fifo.Open(O_RDWR);

    std::ofstream eventsLog;
    OpenFile("events.log", eventsLog);

    return {
        std::move(eventsLog),
        std::move(fifo)
    };
}

void InstallHandler(const int sig, void (*handler)(int), const int flags = 0) {
    struct sigaction sa = {};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;

    if (::sigaction(sig, &sa, nullptr) == -1)
        throw std::system_error(errno, std::generic_category(), "sigaction");
}

sig_atomic_t terminated{false};

static void OnSigTermHandler(int sig) {
    terminated = true;
}

int main(const int argc, char *argv[]) {
    try {
        InstallHandler(SIGTERM, OnSigTermHandler);
        InstallHandler(SIGINT, OnSigTermHandler);
        auto [eventLog, requestFiFo] = GetInputData(argc, argv);

        while (!terminated) {
            EventInfo event{};
            if (!ReadFull(requestFiFo, &event, sizeof(EventInfo))) {
                if (terminated) break;
                continue;
            }

            if (event.responseFiFo[MAX_FIFO_PATH_LENGTH - 1] != '\0') {
                std::cerr << "FiFo path for response with not null-terminated." << std::endl;
                continue;
            }

            if (event.message[MAX_MESSAGE_LENGTH_WITH_NULL - 1] != '\0') {
                event.message[MAX_MESSAGE_LENGTH_WITH_NULL - 1] = '\0';
            }

            FifoRaii responseFiFo(std::string(event.responseFiFo));
            responseFiFo.Open(O_WRONLY);

            if (event.command == Command::Shutdown) {
                responseFiFo.Write(SHUTDOWN_MSG, strlen(SHUTDOWN_MSG) + 1);
                break;
            }

            PrintMessage(event, std::cout);
            PrintMessage(event, eventLog);
            responseFiFo.Write(DATA_WAS_RECEIVED, strlen(DATA_WAS_RECEIVED) + 1);
        }

        return EXIT_SUCCESS;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
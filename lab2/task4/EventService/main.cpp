#include <chrono>
#include <iostream>
#include <ostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <format>
#include <signal.h>

#include "src/File.hpp"
#include <Shared/EventInfo.h>
#include <Shared/FIFORAII/FIFORAII.h>

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
    os.write(event.message, MAX_MESSAGE_LENGTH);
    os << std::endl;
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
    InstallHandler(SIGTERM, OnSigTermHandler);
    InstallHandler(SIGINT, OnSigTermHandler);

    auto input = ArgumentsParse(argc, argv);

    FIFO_RAII fifo(input.pathToFiFo, true);
    std::cout << "event-service: listening on " << input.pathToFiFo << std::endl << std::endl;

    try {
        fifo.Open(O_RDONLY);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::ofstream eventsLog;
    OpenFile("events.log", eventsLog);

    while (!terminated) {
        EventInfo event{};
        size_t bytesRead = fifo.Read(&event, sizeof(EventInfo));
        if (bytesRead == 0) continue;

        FIFO_RAII responseFiFo(std::string(event.responseFiFo));
        responseFiFo.Open(O_WRONLY);

        if (event.command == Command::Shutdown) {
            auto msg = "The shutdown has been proceeded."s;
            responseFiFo.Write(msg.data(), msg.size() + 1);
            break;
        }

        PrintMessage(event, std::cout);
        PrintMessage(event, eventsLog);
        auto msg = "The data was recieved. See ya :b"s;
        responseFiFo.Write(msg.data(), msg.size() + 1);
    }

    return EXIT_SUCCESS;
}

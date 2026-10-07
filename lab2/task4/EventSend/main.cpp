#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <format>

#include <Shared/FIFORAII/FIFORAII.h>
#include <Shared/EventInfo.h>

struct Input {
    std::string pathToFiFo;
    EventInfo event{};
};
Input Parse3Args(const int argc, char *argv[]) {
    if (argc < 3) {
        throw std::invalid_argument(std::format("Usage: [%s <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n"
                                                           "       [%s <path_to_fifo> --shutdown]", argv[0], argv[0]));
    }
    Input input;

    input.pathToFiFo = argv[1];
    if (strcmp(argv[2], "--shutdown") == 0) {
        input.event.command = Command::Shutdown;
        return input;
    }

    throw std::invalid_argument(std::format("Shutdown command not implemented: %s", argv[2]));
}

Input Parse4Args(const int argc, char *argv[]) {
    if (argc < 4) {
        throw std::invalid_argument(std::format("Usage: [%s <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n"
                                                           "       [%s <path_to_fifo> --shutdown]", argv[0], argv[0]));
    }
    Input input;

    if (strlen(argv[2]) > MAX_MESSAGE_LENGTH) {
        throw std::invalid_argument(std::format("Message too long: %s", argv[3]));
    }

    input.pathToFiFo.append(argv[1]);
    input.event.level = EventLevelFromString(argv[2]);
    strcpy(input.event.message, argv[3]);
    input.event.senderPID = getpid();
    input.event.command = Command::Log;
    return input;
}

Input ArgumentsParse(const int argc, char* argv[]) {
    if (argc == 3) {
        return Parse3Args(argc, argv);
    }
    if (argc == 4) {
        return Parse4Args(argc, argv);
    }

    throw std::invalid_argument(std::format("Usage: [%s <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n       [%s <path_to_fifo> --shutdown]", argv[0], argv[0]));
}

int main(const int argc, char *argv[]) {
    try {
        auto [pathToFiFoRequestFiFo, event] = ArgumentsParse(argc, argv);
        FIFO_RAII requestFiFo(pathToFiFoRequestFiFo);
        requestFiFo.Open(O_WRONLY | O_NONBLOCK);

        requestFiFo.Write(&event,  sizeof(EventInfo));
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
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
// Функция создающая путь в папке tmp к fifo для ответа. Внутри одного процесса гарантируется одинаковый результат.
std::filesystem::path GetPathToResponseFiFo() {
    std::filesystem::path responseFiFo("/tmp/" + std::to_string(getpid()) + "response.fifo");

    if (responseFiFo.string().size() > MAX_FIFO_PATH_LENGTH) {
        throw std::runtime_error("Can't create FiFo for response with path: " + responseFiFo.string());
    }

    return responseFiFo;
}

Input Parse3Args(const int argc, char *argv[]) {
    if (argc < 3) {
        throw std::invalid_argument(std::format("Usage: [{0} <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n"
                                                           "       [{1} <path_to_fifo> --shutdown]", argv[0], argv[0]));
    }
    Input input;

    input.pathToFiFo = argv[1];
    strcpy(input.event.responseFiFo, GetPathToResponseFiFo().c_str());
    if (strcmp(argv[2], "--shutdown") == 0) {
        input.event.command = Command::Shutdown;
        return input;
    }

    throw std::invalid_argument(std::format("Shutdown command not implemented: {0}", argv[2]));
}

Input Parse4Args(const int argc, char *argv[]) {
    if (argc < 4) {
        throw std::invalid_argument(std::format("Usage: [{0} <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n"
                                                           "       [{0} <path_to_fifo> --shutdown]", argv[0]));
    }
    Input input;

    auto messageSize = strlen(argv[3]);
    if (messageSize > MAX_MESSAGE_LENGTH_WITH_NULL - 1) {
        throw std::invalid_argument(std::format("Message too long: {0}", argv[3]));
    }

    input.pathToFiFo.append(argv[1]);
    input.event.level = EventLevelFromString(argv[2]);
    strcpy(input.event.responseFiFo, GetPathToResponseFiFo().c_str());
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

    throw std::invalid_argument(std::format("Usage: [{0} <path_to_fifo> <INFO|WARN|ERROR> <char[50]>] \n       [{1} <path_to_fifo> --shutdown]", argv[0], argv[0]));
}

int main(const int argc, char *argv[]) {
    try {
        auto [pathToFiFoRequestFiFo, event] = ArgumentsParse(argc, argv);
        auto pathToResponseFiFo = GetPathToResponseFiFo().string();
        FifoRaii requestFiFo(pathToFiFoRequestFiFo);
        FifoRaii responseFiFo(pathToResponseFiFo, true);
        requestFiFo.Open(O_WRONLY | O_NONBLOCK);

        requestFiFo.Write(&event,  sizeof(EventInfo));

        responseFiFo.Open(O_RDONLY);
        char responseMessage[MAX_RESPONSE_LENGTH_WITH_NULL];
        size_t bytesRead = 0;
        do {
            bytesRead = responseFiFo.Read(responseMessage, MAX_RESPONSE_LENGTH_WITH_NULL);
        } while (bytesRead == 0);
        std::cout.write(responseMessage, static_cast<std::streamsize>(bytesRead)) << std::endl;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
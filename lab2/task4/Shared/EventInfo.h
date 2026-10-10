#pragma once
#include <cstdint>
#include <string>
#include <ctime>

constexpr auto MAX_RESPONSE_LENGTH_WITH_NULL = 255;

enum class Command : uint8_t {Shutdown, Log};
enum class EventLevel :uint8_t {INFO, WARN, ERROR};
constexpr auto MAX_MESSAGE_LENGTH_WITH_NULL = 50;
constexpr auto MAX_FIFO_PATH_LENGTH = 255;
#pragma pack(push, 1)
struct EventInfo {
    Command command;
    pid_t senderPID;
    EventLevel level;
    char message[MAX_MESSAGE_LENGTH_WITH_NULL];
    char responseFiFo[MAX_FIFO_PATH_LENGTH];
};
#pragma pack(pop)

inline EventLevel EventLevelFromString(const std::string& eventLevel) {
    if (eventLevel == "INFO") {
        return EventLevel::INFO;
    } else if (eventLevel == "ERROR") {
        return EventLevel::ERROR;
    } else if (eventLevel == "WARN") {
        return EventLevel::WARN;
    } else {
        throw std::invalid_argument("Invalid event level");
    }
}

inline std::string EventLevelToString(const EventLevel level) {
    switch (level) {
    case EventLevel::INFO:
        return "INFO";
    case EventLevel::WARN:
        return "WARN";
    case EventLevel::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
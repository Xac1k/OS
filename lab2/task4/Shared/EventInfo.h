#pragma once
#include <cstdint>
#include <string>
#include <ctime>

enum class Command : uint8_t {Shutdown, Log};
enum class EventLevel :uint8_t {INFO, WARN, ERROR};
constexpr auto MAX_MESSAGE_LENGTH = 50;
#pragma pack(push, 1)
struct EventInfo {
    Command command;
    pid_t senderPID;
    EventLevel level;
    char message[MAX_MESSAGE_LENGTH];
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
#pragma once
#include <string>

using namespace std::literals;

class FifoRaii {
public:
    static constexpr auto MINIMAL_PERMISSIONS_OF_FIFO = 0666;

    explicit FifoRaii(const std::string& pathToFiFo, bool newFifo = false);
    void Open(int oflag);
    ssize_t Read(void* buf, size_t nbytes) const;
    void Write(const void* buf, size_t nbytes) const;
    [[nodiscard]] int FD() const;
    ~FifoRaii();

    FifoRaii(const FifoRaii&) = delete;
    FifoRaii& operator=(const FifoRaii&) = delete;

private:
    // Разделить класс на фд и фифо. Сейчас две отвественности внутри
    std::string m_pathToFiFo;
    int m_fifoFD = -1;
    bool m_newFifo = false;

    [[nodiscard]] bool IsOpened() const;
};
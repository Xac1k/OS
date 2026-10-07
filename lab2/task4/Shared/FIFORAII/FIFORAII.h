#pragma once
#include <string>

using namespace std::literals;

class FIFO_RAII {
public:
    static constexpr auto MINIMAL_PERMISSIONS_OF_FIFO = 0666;

    explicit FIFO_RAII(const std::string& pathToFiFo, bool newFifo = false);
    void Open(int oflag);
    ssize_t Read(void* buf, size_t nbytes) const;
    void Write(const void* buf, size_t nbytes) const;
    [[nodiscard]] int FD() const;
    ~FIFO_RAII();

    FIFO_RAII(const FIFO_RAII&) = delete;
    FIFO_RAII& operator=(const FIFO_RAII&) = delete;

private:
    std::string m_pathToFiFo;
    int m_fifoFD = -1;
    bool m_newFifo = false;

    [[nodiscard]] bool IsOpened() const;
};
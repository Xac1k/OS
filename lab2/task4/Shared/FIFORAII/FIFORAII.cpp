#include "FIFORAII.h"

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/stat.h>

FifoRaii::FifoRaii(const std::string& pathToFiFo, bool newFifo)
: m_pathToFiFo(pathToFiFo)
, m_newFifo(newFifo)
{
    if (newFifo) {
        if (mkfifo(pathToFiFo.c_str(), MINIMAL_PERMISSIONS_OF_FIFO) == -1) {
            if (errno == EEXIST) {
                if (unlink(pathToFiFo.c_str()) == -1) {
                    throw std::runtime_error("Error unlinking existing FIFO: " +
                                             pathToFiFo + "\n" + std::strerror(errno));
                }
                if (mkfifo(pathToFiFo.c_str(), MINIMAL_PERMISSIONS_OF_FIFO) == -1) {
                    throw std::runtime_error("Error creating FIFO after unlink: " +
                                             pathToFiFo + "\n" + std::strerror(errno));
                }
            } else {
                throw std::runtime_error("Error creating FIFO: " +
                                         pathToFiFo + "\n" + std::strerror(errno));
            }
        }
    }
}

void FifoRaii::Open(const int oflag) {
    if (IsOpened()) {
        throw std::runtime_error("FiFo is already opened");
    }

    m_fifoFD = open(m_pathToFiFo.c_str(), oflag);
    if (m_fifoFD == -1) {
        const auto err = errno;
        std::string msg;

        if (err == EINTR) {
            msg = "Interrupted open. Graceful shutdown requested.";
        } else if (err == ENXIO) {
            msg = "Service is not available (no reader on " + m_pathToFiFo + ")";
        } else if (err == ENOENT) {
            msg = "FIFO does not exist: " + m_pathToFiFo;
        } else {
            msg = "Open failed: " + std::string(std::strerror(err));
        }

        throw std::runtime_error(msg);
    }
}

bool FifoRaii::IsOpened() const {
    return m_fifoFD != -1;
}

ssize_t FifoRaii::Read(void* buf, size_t nbytes) const {
    if (!IsOpened()) {
        throw std::runtime_error("FiFo isn't opened yet.");
    }

    ssize_t n;
    do {
        n = read(m_fifoFD, buf, nbytes);
    } while (n == -1 && errno == EINTR);
    if (n == -1) {
        throw std::runtime_error("Error reading FIFO: "s + std::strerror(errno));
    }

    return n;
}

void FifoRaii::Write(const void* buf, size_t nbytes) const {
    if (!IsOpened()) {
        throw std::runtime_error("FiFo isn't opened yet.");
    }

    if (const auto writenBytes = write(m_fifoFD, buf, nbytes); writenBytes == -1) {
        throw std::runtime_error("Error writing FIFO: "s + std::strerror(errno));
    }
}

int FifoRaii::FD() const {
    return m_fifoFD;
}

FifoRaii::~FifoRaii() {
    if (IsOpened()) {
        close(m_fifoFD);
        m_fifoFD = -1;
    }
    if (m_newFifo) {
        unlink(m_pathToFiFo.c_str());
    }
}
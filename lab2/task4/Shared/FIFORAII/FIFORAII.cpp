//
// Created by maksim on 10/3/26.
//

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
            msg = "Interrupted open.";
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

void FifoRaii::Close() {
    if (IsOpened()) {
        close(m_fifoFD);
        m_fifoFD = -1;
    }
}

bool FifoRaii::IsOpened() const {
    return m_fifoFD != -1;
}

ssize_t FifoRaii::Read(void* buf, size_t nbytes) const {
    if (!IsOpened()) {
        throw std::runtime_error("FiFo isn't opened yet.");
    }

    const ssize_t n = read(m_fifoFD, buf, nbytes);
    if (n == -1) {
        throw std::runtime_error("Error reading FIFO: "s + std::strerror(errno));
    }

    return n;
}

void FifoRaii::Write(const void* buf, size_t nbytes) const {
    if (!IsOpened()) {
        throw std::runtime_error("FiFo isn't opened yet.");
    }

    const auto writenBytes = write(m_fifoFD, buf, nbytes);
    if (writenBytes == -1) {
        throw std::runtime_error("Error writing FIFO: "s + std::strerror(errno));
    }
}

int FifoRaii::FD() const {
    return m_fifoFD;
}

void FifoRaii::Free() {
    if (IsOpened()) {
        close(m_fifoFD);
        m_fifoFD = -1;
    }
    if (m_newFifo) {
        unlink(m_pathToFiFo.c_str());
    }
}

FifoRaii::~FifoRaii() {
    Free();
}

FifoRaii::FifoRaii(FifoRaii&& other) noexcept
: m_pathToFiFo(std::move(other.m_pathToFiFo))
, m_fifoFD(other.m_fifoFD)
, m_newFifo(other.m_newFifo)
{
    other.m_fifoFD = -1;
    other.m_newFifo = false;
}

FifoRaii& FifoRaii::operator=(FifoRaii&& other) noexcept {
    if (this == &other) return *this;

    FifoRaii tmp(std::move(other));
    std::swap(m_fifoFD, tmp.m_fifoFD);
    std::swap(m_newFifo, tmp.m_newFifo);
    std::swap(m_pathToFiFo, tmp.m_pathToFiFo);

    return *this;
}

bool ReadFull(const FifoRaii& fifo, void* buf, size_t nbytes) {
    size_t remainToRead = nbytes;

    while (remainToRead != 0) {
        auto shift = nbytes - remainToRead;
        const size_t bytesRead = fifo.Read(static_cast<char*>(buf) + shift, remainToRead);
        if (bytesRead == 0) return false;

        remainToRead -= bytesRead;
    }

    return true;
}
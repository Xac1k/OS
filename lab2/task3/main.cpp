#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

#include "utils/StringUtils.h"


class ChildProcess {
public:
    using Task = std::function<void()>;

    explicit ChildProcess(const Task& task) {
        m_pid = fork();
        if (m_pid == -1) {
            throw std::runtime_error(strerror(errno));
        }
        if (IsChild()) {
            RunWithoutFallThrough(task);
        }
    };

    ~ChildProcess() {
        Reap();
    }

    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    ChildProcess(ChildProcess&& other) noexcept
    : m_pid(other.m_pid) {
        other.m_pid = -1;
    }

    ChildProcess& operator=(ChildProcess&& other) noexcept {
        if (&other != this) {
            Reap();
            m_pid = other.m_pid;
            other.m_pid = -1;
        }
        return *this;
    }

    [[nodiscard]] bool IsChild() const noexcept {
        return m_pid == 0;
    }

    int Wait() {
        if (IsChild()) return -1;

        int status = 0;
        pid_t r;
        do {
            r = waitpid(m_pid, &status, 0);
        } while (r == -1 && errno == EINTR);

        if (r == -1) {
            m_pid = -1;
            return -1;
        }
        if (r == m_pid) m_pid = -1;

        return status;
    }

    [[nodiscard]] int GetPid() const noexcept {
        return m_pid;
    }

    static void DecodeStatus(const int status) noexcept {
        if (WIFEXITED(status)) {
            std::cout << "exited with code " << WEXITSTATUS(status) << "\n";
        } else if (WIFSIGNALED(status)) {
            std::cout << "killed by signal " << WTERMSIG(status)
                      << " (" << strsignal(WTERMSIG(status)) << ")";
            if (WCOREDUMP(status))
                std::cout << "  (core dumped)\n";
        } else if (WIFSTOPPED(status)) {
            std::cout << "stopped by signal " << WSTOPSIG(status) << "\n";
        } else {
            std::cout << "unknown status: " << status << "\n";
        }
    }

    void MarkReaped() noexcept {
        m_pid = -1;
    }
private:
    static void RunWithoutFallThrough(const Task& task) noexcept {
        task();
        _exit(EXIT_SUCCESS);
    };

    void Reap() {
        if (m_pid > 0) {
            int wstatus;
            pid_t r;
            do {
                r = waitpid(m_pid, &wstatus, 0);
            } while (r == -1 && errno == EINTR);

            m_pid = -1;
        }
    }

    int m_pid;
};

class Pipe {
public:
    Pipe() {
        int pipeFd[2];
        if (pipe(pipeFd) == -1) {
            throw std::runtime_error(strerror(errno));
        }

        readFd = pipeFd[0];
        writeFd = pipeFd[1];
    }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Pipe(Pipe&& other) noexcept
    : readFd(other.readFd)
    , writeFd(other.writeFd) {
        other.readFd = -1;
        other.writeFd = -1;
    }

    Pipe& operator=(Pipe&& other) noexcept
    {
        if (this != &other) {
            CloseFD(readFd);
            CloseFD(writeFd);
            readFd = std::exchange(other.readFd, -1);
            writeFd = std::exchange(other.writeFd, -1);
        }
        return *this;
    }

    static void CloseFD(int& fd) {
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }

    void CloseReadEnd() {
        CloseFD(readFd);
    }

    void CloseWriteEnd() {
        CloseFD(writeFd);
    }

    int ReadFD() const {
        return readFd;
    }

    int WriteFD() const {
        return writeFd;
    }

    ~Pipe() {
        CloseFD(readFd);
        CloseFD(writeFd);
    }
private:
    int readFd, writeFd;
};

struct Command {
    std::string name;
    std::vector<std::string> args;
};

Command ParseStringOntoCommand(const std::string& line) {
    std::istringstream commandStream(line);
    std::string commandName, arg;
    std::vector<std::string> args;

    commandStream >> commandName;

    while (commandStream >> arg) {
        args.push_back(arg);
    }

    return {commandName, args};
}

bool IsExitCommand(const std::string& line) {
    return ParseStringOntoCommand(line).name == "exit";
}

void ExecVP(const std::string& execFile, const std::vector<std::string>& args) {
    std::vector<char*> argv;

    argv.push_back(strdup(execFile.c_str()));
    for (auto& arg : args) {
        argv.push_back(strdup(arg.c_str()));
    }
    argv.push_back(nullptr);

    if (execvp(execFile.c_str(), argv.data()) != 1) {
        throw std::runtime_error(strerror(errno));
    };
}

std::vector<Command> ParsePipedCommandLines(const std::vector<std::string>& commandLines) {
    std::vector<Command> pipedCommands;
    for (const auto& commandLine : commandLines) {
        auto command = ParseStringOntoCommand(commandLine);
        pipedCommands.push_back(command);
    }

    return pipedCommands;
}

void HandleCommand(const Command& command) {
    ChildProcess commandTask([&command] {
        try {
            ExecVP(command.name, command.args);
        } catch (const std::runtime_error& e) {
            std::cout << "mini-shell: " << command.name << ": " << e.what() << std::endl;
        }
    });

    const auto pid = commandTask.GetPid();
    const auto status = commandTask.Wait();
    if (status != EXIT_SUCCESS) {
        std::cout << "Process " << pid << " ";
        ChildProcess::DecodeStatus(status);
    }
}

void HandlePipedCommands(const std::vector<Command>& pipedCommands) {
    if (pipedCommands.size() == 1) {
        HandleCommand(pipedCommands.at(0));
        return;
    }
    std::vector<ChildProcess> processes;
    processes.reserve(pipedCommands.size());

    std::vector<Pipe> pipes(pipedCommands.size() - 1);

    for (size_t i = 0; i < pipedCommands.size(); ++i) {
        const Command command = pipedCommands.at(i);
        const auto writeFd = i != pipes.size() ? pipes.at(i).WriteFD() : -1;
        const int readFd = i != 0 ? pipes.at(i - 1).ReadFD() : -1;

        processes.emplace_back([command, writeFd, readFd, &pipes]  {
            if (readFd != -1) {
                if (dup2(readFd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (writeFd != -1) {
                if (dup2(writeFd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            for (const auto& pipe : pipes) {
                close(pipe.ReadFD());
                close(pipe.WriteFD());
            }

            ExecVP(command.name, command.args);
        });
    }

    for (auto& pipe : pipes) {
        pipe.CloseReadEnd();
        pipe.CloseWriteEnd();
    }

    for (auto& process : processes) {
        const auto pid = process.GetPid();
        const auto status = process.Wait();
        if (status != EXIT_SUCCESS) {
            std::cout << "Process " << pid << " ";
            ChildProcess::DecodeStatus(status);
        }
    }
}

int main() {
    std::cout << "> ";

    std::string line;
    while (std::getline(std::cin, line, '\n')) {
        if (IsExitCommand(line)) {
            break;
        }

        auto pipedCommandLines = Separate(line, '|');
        auto pipedCommands = ParsePipedCommandLines(pipedCommandLines);

        HandlePipedCommands(pipedCommands);

        std::cout << "> ";
    }
}
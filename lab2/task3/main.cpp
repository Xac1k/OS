#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
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
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            throw std::runtime_error(strerror(errno));
        }

        readFd = pipefd[0];
        writeFd = pipefd[1];
    }

    enum class Port {Input, Output};

    void ForwardWriteEnd(int from) {
        if (dup2(writeFd, from) == -1) {
            throw std::runtime_error("dup2 failed");
        }
    }

    void Close() {
        close(readFd);
        close(writeFd);

        readFd = -1;
        writeFd = -1;
    }

    ~Pipe() {
        close(readFd);
        close(writeFd);
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
    // std::vector<ChildProcess> processes;
    // processes.reserve(pipedCommands.size());

    int pipeFd[2];
    if (pipe(pipeFd) == -1) {
        throw std::runtime_error(strerror(errno));
    };

    const int readFd  = pipeFd[0];
    const int writeFd = pipeFd[1];

    ChildProcess commandTask1([&pipedCommands, readFd, writeFd] {
        try {
            close(readFd);
            if (dup2(writeFd, STDOUT_FILENO) == -1) {
                throw std::runtime_error("dup2 failed");
            }
            close(writeFd);
            ExecVP(pipedCommands.at(0).name, pipedCommands.at(0).args);
        } catch (const std::runtime_error& e) {
            std::cout << "mini-shell: " << pipedCommands.at(0).name << ": " << e.what() << std::endl;
        }
    });

    ChildProcess commandTask2([&pipedCommands, pipeFd] {
        try {
            close(pipeFd[1]);
            dup2(pipeFd[0], STDIN_FILENO);
            ExecVP(pipedCommands.at(1).name, pipedCommands.at(1).args);
            close(pipeFd[0]);
        } catch (const std::runtime_error& e) {
            std::cout << "mini-shell: " << pipedCommands.at(1).name << ": " << e.what() << std::endl;
        }
    });

    close(pipeFd[0]);
    close(pipeFd[1]);

    const auto pid1 = commandTask1.GetPid();
    const auto status1 = commandTask1.Wait();
    if (status1 != EXIT_SUCCESS) {
        std::cout << "Process " << pid1 << " ";
        ChildProcess::DecodeStatus(status1);
    }

    const auto pid2 = commandTask2.GetPid();
    const auto status2 = commandTask2.Wait();
    if (status2 != EXIT_SUCCESS) {
        std::cout << "Process " << pid2 << " ";
        ChildProcess::DecodeStatus(status2);
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
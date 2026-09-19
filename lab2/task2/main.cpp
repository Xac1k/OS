#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <unistd.h>
#include <bits/this_thread_sleep.h>
#include <sys/wait.h>

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

void InstallHandler(const int sig, void (*handler)(int), const int flags = 0) {
    struct sigaction sa = {};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;

    if (::sigaction(sig, &sa, nullptr) == -1)
        throw std::system_error(errno, std::generic_category(), "sigaction");
}

volatile sig_atomic_t statusRequested = 0;

static void OnSigint(int sig) {
    statusRequested = 1;
}

volatile sig_atomic_t stopRequested = 0;

static void OnSigterm(int sig) {
    stopRequested = 1;
}



int main() {
    ChildProcess stopable([] {
        InstallHandler(SIGUSR1, OnSigint);
        InstallHandler(SIGTERM, OnSigterm);

        int iterations = 0;
        while (!stopRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            iterations++;

            if (statusRequested) {
                statusRequested ^= 1;

                std::cout << "Worker " << getpid() << ": iteration " << iterations << "\n";
            }
        }

        std::cout << "Worker " << getpid() << " is shutting down" << std::endl << std::flush;

        _exit(0);
    });
    const auto workerPid = stopable.GetPid();
    std::cout << "Worker started with PID " << workerPid << "\n";
    std::cout << "> " << std::flush;

    std::string command;
    while (getline(std::cin, command)) {
        if (command == "pause") {
            std::cout << "Worker " << workerPid << " paused\n";
            kill(workerPid, SIGSTOP);
        }
        else if (command == "resume") {
            std::cout << "Worker " << workerPid << " resumed\n";
            kill(workerPid, SIGCONT);
        }
        else if (command == "status") {
            kill(workerPid, SIGUSR1);
        }
        else if (command == "stop") {
            kill(stopable.GetPid(), SIGTERM);
            stopable.Wait();
            std::cout << "Worker " << workerPid << " exited\n";
        }
        else {
            std::cout << "Unknown command: " << command << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "> " << std::flush;
    }
}
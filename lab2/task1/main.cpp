#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <unistd.h>
#include <bits/this_thread_sleep.h>
#include <sys/wait.h>

#define STREAM_CREATING

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

int main()  {

#ifdef STREAM_CREATING
    std::srand(std::time(nullptr));

    std::vector<std::pair<std::string, ChildProcess>> childProcesses {};

    std::cout << "Starting task: normal" << std::endl;
    childProcesses.emplace_back("normal", [] {});

    std::cout << "Starting task: exit-42" << std::endl;
    childProcesses.emplace_back("exit-42", [] {
        _exit(42);
    });

    std::cout << "Starting task: abort" << std::endl;
    childProcesses.emplace_back("abort", [] {
        abort();
    });

    std::cout << "Starting task: segfault" << std::endl;
    childProcesses.emplace_back("segfault", [] {
        int *ptr = NULL;
        *ptr = 42;
    });
    std::cout << std::endl;

    std::size_t remaining = childProcesses.size();
    while (remaining != 0) {
        pid_t pid;
        int status = 0;
        do {
            pid = waitpid(-1, &status, 0);
        } while (pid == -1 && errno == EINTR);

        for (auto& childProcess : childProcesses) {
            if (childProcess.second.GetPid() == pid) {
                childProcess.second.MarkReaped();
                remaining--;

                std::cout << "Child " << childProcess.first << " ";
                ChildProcess::DecodeStatus(status);
            }
        }
    }


#else
    std::cout << "Starting task: delayed" << std::endl;
    ChildProcess delayedProcess([] {});

    const auto pidDelayed = delayedProcess.GetPid();
    std::cout << "Child " << pidDelayed << " started" << std::endl;
    std::cout << "Parent is waiting before waitpid()..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(20));
    const auto statusDelayed = delayedProcess.Wait();
    std::cout << "Child " << pidDelayed << " ";
    ChildProcess::DecodeStatus(statusDelayed);
    std::cout << std::endl;
#endif

    std::cout << "All tasks completed" << std::endl;
    return 0;
}
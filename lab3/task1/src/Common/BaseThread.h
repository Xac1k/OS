#pragma once
#include <thread>

template <typename T>
concept HasWorkerLoop = requires(T t, std::stop_token stopToken) {
    { t.WorkerLoop(stopToken) } -> std::same_as<void>;
};

template <typename Derived>
class BaseThread {
public:
    void Start() requires HasWorkerLoop<Derived> {
        if (IsThreadRunning()) {
            return;
        }
        auto* self = static_cast<Derived*>(this);
        m_thread = std::jthread([self](std::stop_token st) {
            self->WorkerLoop(st);
        });
    }

    void Stop() {
        if (!IsThreadRunning()) {
            return;
        }
        m_thread.request_stop();
        m_thread.join();
    }

    void Wait() {
        if (IsThreadRunning()) {
            m_thread.join();
        }
    }

    [[nodiscard]] bool IsThreadRunning() const noexcept {
        return m_thread.joinable();
    }

protected:
    ~BaseThread() = default;

private:
    std::jthread m_thread;
};

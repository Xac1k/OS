#pragma once
#include <thread>


template <typename T>
concept HasWorkerLoop = requires(T t) {
    { t.WorkerLoop() } -> std::same_as<void>;
};

template <typename Derived>
class BaseThread {
public:
    void Start() requires HasWorkerLoop<Derived> {
        if (!m_isRunning.exchange(true)) {
            m_thread = std::jthread(&Derived::WorkerLoop, static_cast<Derived*>(this));
        }
    }

    void Stop() {
        if (m_isRunning.exchange(false) && m_thread.joinable()) {
            m_thread.join();
        }
    }

    void Wait() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    [[nodiscard]] bool IsThreadRunning() const {
        return m_isRunning.load(std::memory_order_relaxed);
    }

private:
    std::jthread m_thread;
    std::atomic<bool> m_isRunning{false};
};
#include <boost/context/fiber.hpp>

#include <iostream>
#include <list>
#include <ranges>
#include <utility>

namespace ctx = boost::context;

enum class TaskState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

struct TaskContext {
    size_t pid = 0;
    ctx::fiber fiber;
    TaskState state = TaskState::READY;
    std::vector<size_t> pending;

    class Scheduler* scheduler = nullptr;
};

thread_local TaskContext* currentContext = nullptr;

class Scheduler {
public:
    using Task = std::function<void()>;

    size_t Spawn(Task task) {
        auto pid = pidCounter++;
        auto ctx = std::make_unique<TaskContext>();
        ctx->scheduler = this;
        ctx->pid = pid;
        ctx->fiber = ctx::fiber([task = std::move(task), ctx = ctx.get()](ctx::fiber&& runContinuation) {
            ctx->fiber = std::move(runContinuation);
            task();
            return std::move(ctx->fiber);
        });

        tasks.emplace(pid, std::move(ctx));
        return pid;
    }

    void Run() {
        while (!tasks.empty()) {
            for (auto& [pid, task]: tasks) {
                if (task->state != TaskState::READY) continue;

                task->state = TaskState::RUNNING;
                currentContext = task.get();
                task->fiber = std::move(task->fiber).resume();
                if (!task->fiber) {
                    task->state = TaskState::FINISHED;
                    PingPendingTasks(task->pid);
                    finished.push_back(pid);
                } else if (task->state == TaskState::RUNNING) {
                    task->state = TaskState::READY;
                }
                currentContext = nullptr;
            }
            ClearFinished();
        }
    }

    void Yield() {
        if (!currentContext) return;
        currentContext->fiber = std::move(currentContext->fiber).resume();
    }

    void Join(const size_t pid) {
        if (!currentContext) return;
        if (pid == currentContext->pid) return;

        currentContext->state = TaskState::WAITING;
        const auto pendingPid = currentContext->pid;

        const auto& prerequisite = GetTasksByPid(pid);
        prerequisite->pending.push_back(pendingPid);
    }
private:
    void PingPendingTasks(const size_t pid) {
        const auto& task = GetTasksByPid(pid);
        for (const auto& pending: task->pending) {
            const auto& pendingTask = GetTasksByPid(pending);
            pendingTask->state = TaskState::READY;
        }
    }

    void ClearFinished() {
        for (auto& pid: finished) {
            tasks.erase(pid);
        }
        finished.clear();
    }

    std::unique_ptr<TaskContext>& GetTasksByPid(const size_t pid) {
        const auto it = tasks.find(pid);
        if (it == tasks.end()) {
            throw std::invalid_argument("The process with pid:" + std::to_string(pid) + " was not found.");
        }

        return it->second;
    };

    std::vector<size_t> finished;
    std::unordered_map<size_t, std::unique_ptr<TaskContext>> tasks;
    size_t pidCounter = 0;
};

size_t Spawn(const Scheduler::Task& task) {
    if (currentContext && currentContext->scheduler) {
        return currentContext->scheduler->Spawn(task);
    }
    throw std::runtime_error("The environment wasn't set up");
}

void Yield() {
    if (currentContext && currentContext->scheduler) {
        currentContext->scheduler->Yield();
    }
}

size_t GetPid() {
    if (currentContext) return currentContext->pid;
    throw std::runtime_error("The environment wasn't set up");
}

void Join(const size_t pid) {
    if (!currentContext) return;
    currentContext->scheduler->Join(pid);
}

void TaskY() {
    for (int i = 1; i <= 4; ++i) {
        std::cout << "Y" << '\n';
        Yield();
    }
}

void TaskA()
{
    for (int i = 1; i <= 3; ++i)
    {
        std::cout << "A" << i << '\n';
        const auto pid = Spawn(TaskY);
        Join(pid);
        Yield();
    }
}

void TaskB()
{
    for (int i = 1; i <= 2; ++i)
    {
        std::cout << "B" << i << '\n';
        Yield();
    }
}

void TaskC()
{
    for (int i = 1; i <= 4; ++i)
    {
        std::cout << "C" << i << '\n';
        Yield();
    }
}

int main() {
    Scheduler scheduler;

    scheduler.Spawn(TaskA);
    scheduler.Spawn(TaskB);
    scheduler.Spawn(TaskC);

    scheduler.Run();

    std::cout << "Все задачи выполнены" << '\n';
}

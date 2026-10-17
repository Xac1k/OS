#include <boost/context/fiber.hpp>

#include <iostream>
#include <utility>

namespace ctx = boost::context;

enum class TaskState {
    READY,
    RUNNING,
    FINISHED
};

struct TaskContext {
    ctx::fiber fiber;
    TaskState state = TaskState::READY;
    class Scheduler* scheduler = nullptr;
};

thread_local TaskContext* currentContext = nullptr;

class Scheduler {
public:
    using Task = std::function<void()>;

    void Spawn(Task task) {
        auto ctx = std::make_unique<TaskContext>();
        ctx->scheduler = this;
        ctx->fiber = ctx::fiber([task = std::move(task), ctx = ctx.get()](ctx::fiber&& runContinuation) {
            ctx->fiber = std::move(runContinuation);
            task();
            return std::move(ctx->fiber);
        });

        tasks.push_back(std::move(ctx));
    }

    void Run() {
        while (HasNotFinished()) {
            for (auto& task: tasks) {
                if (task->state != TaskState::READY) continue;

                task->state = TaskState::RUNNING;
                currentContext = task.get();
                task->fiber = std::move(task->fiber).resume();
                if (task->fiber) {
                    task->state = TaskState::READY;
                } else {
                    task->state = TaskState::FINISHED;
                }
                currentContext = nullptr;
            }
        }
    }

    void Yield() {
        if (!currentContext) return;
        currentContext->fiber = std::move(currentContext->fiber).resume();
    }
private:
    bool HasNotFinished() {
        return std::ranges::any_of(tasks, [](const auto& task) {
            return task->state != TaskState::FINISHED;
        });
    }

    std::vector<std::unique_ptr<TaskContext>> tasks;
};

void Yield() {
    if (currentContext && currentContext->scheduler) {
        currentContext->scheduler->Yield();
    }
}

void TaskA()
{
    for (int i = 1; i <= 3; ++i)
    {
        std::cout << "A" << i << '\n';
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

#pragma once

#include <vector>
#include <thread>
#include <random>

#include "task.hpp"

class SimpleTaskCollection : public TaskCollection
{
private:
    std::vector<Task *> _data;

public:
    int size() const override { return static_cast<int>(_data.size()); }
    Task *operator[](int i) override { return _data[static_cast<size_t>(i)]; }
    void push(Task *t) override { _data.push_back(t); }
    Task *pop() override
    {
        if (_data.empty())
            return nullptr;
        Task *t = _data.back();
        _data.pop_back();
        return t;
    }
    void clear() override
    {
        _data.clear();
    }
};

class WorkStealingDeque
{
public:
    explicit WorkStealingDeque(long capacity = 4096) : _capacity(capacity),
                                                       _tasks(new Task *[capacity]), _top(0), _bottom(0)
    {
    }

    ~WorkStealingDeque()
    {
        delete[] _tasks;
    }

    // Avoid that this deque can be copied, which would be dangerous
    WorkStealingDeque(const WorkStealingDeque &) = delete;
    WorkStealingDeque &operator=(const WorkStealingDeque &) = delete;

    // pushBottom: called only by the owning thread of this deque
    bool pushBottom(Task *task)
    {
        long bottom = _bottom.load(std::memory_order_relaxed);
        long top = _top.load(std::memory_order_acquire);
        if (bottom - top >= _capacity)
        {
            return false; // deque is full
        }
        _tasks[bottom % _capacity] = task;                   // modulo because of circular deque
        std::atomic_thread_fence(std::memory_order_release); // force all preceding store/load to be visible before the next operations -> permit to insure that the task is really in the _tasks array before annoucing it with the incrementation of the bottom variable
        _bottom.store(bottom + 1, std::memory_order_relaxed);
        return true;
    }

    // popBottom : called only by the owning thread of this deque
    bool popBottom(Task *&result)
    {
        long bottom = _bottom.load(std::memory_order_relaxed) - 1;
        _bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        long top = _top.load(std::memory_order_relaxed);
        if (top <= bottom)
        {
            result = _tasks[bottom & _capacity];
            if (top == bottom) // If this is the last task in the dequeu, there is a possible race with a thief (voleur)
            {
                long expected = top;
                if (!_top.compare_exchange_strong(
                        expected, top + 1,
                        std::memory_order_seq_cst,
                        std::memory_order_relaxed))
                {
                    result = nullptr; // Stolen by another thread
                }
                _bottom.store(bottom + 1, std::memory_order_relaxed); // restore
            }
            return result != nullptr;
        }
        else
        {
            _bottom.store(bottom + 1, std::memory_order_relaxed); // restore
            result = nullptr;
            return false;
        }
    }

    // steal : called by other threads
    bool steal(Task *&result)
    {
        long top = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        long bottom = _bottom.load(std::memory_order_acquire);

        if (top < bottom)
        {
            result = _tasks[top % _capacity];
            long expected = top;
            if (!_top.compare_exchange_strong(
                    expected, top + 1,
                    std::memory_order_seq_cst,
                    std::memory_order_relaxed))
            {
                result = nullptr;
                return false;
            }
            return true;
        }
        result = nullptr;
        return false;
    }

private:
    const long _capacity;
    Task **_tasks;
    std::atomic<long> _top;
    std::atomic<long> _bottom; // Increase when deque is growing
};

class WorkStealingRunner : public TaskRunner
{
public:
    WorkStealingRunner(unsigned num_threads,
                       size_t max_initial_tasks,
                       long deque_capacity = 1 << 20)
        : _num_threads(num_threads),
          _max_initial_tasks(max_initial_tasks),
          _deques(num_threads),
          _threads(),
          _tasks_remaining(0),
          _stop(false)
    {
        if (_num_threads == 0)
            _num_threads = 1;

        if (_max_initial_tasks < 1)
            _max_initial_tasks = 1;

        // Create a WorkStealingDeque foreach thread and a random generator
        for (unsigned i = 0; i < _num_threads; ++i)
        {
            _deques[i] = std::make_unique<WorkStealingDeque>(deque_capacity);
            _rngs.emplace_back(std::random_device{}());
        }
    }

    void run(Task *root) override
    {
        // Filling the leaves array with all the task to distribute to the deques
        std::vector<Task *> leaves;
        partitionInitialTasks(root, leaves);

        if (leaves.empty())
        {
            TaskRunner::startTimer();
            root->solve();
            TaskRunner::stopTimer();
            return;
        }

        _tasks_remaining.store(static_cast<long>(leaves.size()), std::memory_order_relaxed);
        _stop.store(false, std::memory_order_relaxed);

        // Distribute the tasks in the deques using round-robin
        for (size_t i = 0; i < leaves.size(); i++)
        {
            unsigned deque_idx = static_cast<unsigned>(i % _num_threads);
            if (!_deques[deque_idx]->pushBottom(leaves[i]))
            {
                std::cerr << "The deque is full for the thread" << deque_idx << "\n";
                std::abort();
            }
        }
        // Launch all threads
        TaskRunner::startTimer();
        _threads.clear();
        _threads.reserve(_num_threads);

        for (unsigned i = 0; i < _num_threads; i++)
        {
            _threads.emplace_back(&WorkStealingRunner::workerLoop, this, i);
        }

        // Join all threads
        for (auto &th : _threads)
            th.join();
        TaskRunner::stopTimer();

        // Free all the temporary sub tasks (root will be deleted by the caller)
        for (Task *t : leaves)
        {
            if (t != root)
                delete t;
        }
    }

private:
    unsigned _num_threads;
    size_t _max_initial_tasks; // budget

    std::vector<std::unique_ptr<WorkStealingDeque>> _deques;
    std::vector<std::thread> _threads;
    std::vector<std::mt19937_64> _rngs; // random generator used to choose which deque to steal. One generator per thread

    std::atomic<long> _tasks_remaining;
    std::atomic<bool> _stop;

    void partitionInitialTasks(Task *root, std::vector<Task *> &leaves)
    {
        std::vector<Task *> current;
        std::vector<Task *> next;
        current.push_back(root); // add the root to the current task to split

        const size_t budget = _max_initial_tasks;
        SimpleTaskCollection children;

        while (!current.empty())
        {
            next.clear();

            for (size_t idx = 0; idx < current.size(); idx++)
            {
                Task *task = current[idx];

                // Before splitting, we check whether the number of tasks is close to the budget.
                if (leaves.size() + next.size() >= budget)
                {
                    // Push in leaves all the remaining current tasks
                    for (size_t j = idx; j < current.size(); ++j)
                    {
                        leaves.push_back(current[j]);
                    }

                    // Insert all the taks in next in leaves
                    leaves.insert(leaves.end(), next.begin(), next.end());
                    return;
                }

                children.clear();
                int n = task->split(&children);

                // Split didn't work
                if (n == 0)
                {
                    leaves.push_back(task);
                    continue;
                }

                // Split is too large
                if (leaves.size() + next.size() + (size_t)n > budget)
                {
                    for (int i = 0; i < n; i++)
                    {
                        delete children[i];
                    }
                    leaves.push_back(task);
                    continue;
                }

                // Put splitted tasks in next
                for (int i = 0; i < n; i++)
                {
                    next.push_back(children[i]);
                }
            }

            if (next.empty())
            {
                return; // done
            }

            current.swap(next);
        }
    }

    void workerLoop(unsigned id)
    {
        Task *task = nullptr;
        auto &rng = _rngs[id];
        std::uniform_int_distribution<unsigned> victim_dist(0, _num_threads - 1);

        while (true)
        {
            // Try taking a task in his own queue and solve it
            if (_deques[id]->popBottom(task))
            {
                if (task)
                {
                    task->solve();
                    task = nullptr;
                    long remaining = _tasks_remaining.fetch_sub(1, std::memory_order_acq_rel) - 1;
                    if (remaining == 0)
                    {
                        _stop.store(true, std::memory_order_release);
                        break;
                    }
                    continue;
                }
            }

            // Try to randomly steal a task to another deque. (2 * _num_threads is arbitrary choosen)
            bool stolen = false;
            for (unsigned attempt = 0; attempt < _num_threads * 2; ++attempt)
            {
                unsigned victim = victim_dist(rng); // choose a victim to try to steal
                if (victim == id)
                    continue; // Do not steal your own deque
                if (_deques[victim]->steal(task))
                {
                    stolen = true;
                    break; // Break just this for loop
                }
            }

            // Solve the stolen task
            if (stolen && task)
            {
                task->solve();
                task = nullptr;
                long remaining = _tasks_remaining.fetch_sub(1, std::memory_order_acq_rel) - 1;
                if (remaining == 0)
                {
                    _stop.store(true, std::memory_order_release);
                    break;
                }
                continue;
            }

            // No job found yet
            if (_stop.load(std::memory_order_acquire))
                break;

            // Mark the end
            if (_tasks_remaining.load(std::memory_order_acquire) == 0)
            {
                _stop.store(true, std::memory_order_release);
                break;
            }

            std::this_thread::yield();
        }
    }
};
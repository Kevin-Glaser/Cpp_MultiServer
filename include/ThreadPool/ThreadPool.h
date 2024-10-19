#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <functional>
#include <future>
#include <cassert>

#include "SingletonBase/Singleton.h"

template<typename T>
class SafeQueue {
public:
    void push(const T &item) {
        {
            std::scoped_lock lock(mtx_);
            queue_.push(item);
        }
        cond_.notify_one();
    }

    void push(T &&item) {
        {
            std::scoped_lock lock(mtx_);
            queue_.push(std::move(item));
        }
        cond_.notify_one();
    }

    bool pop(T &item) {
        std::unique_lock lock(mtx_);
        cond_.wait(lock, [&]() {
            return !queue_.empty() || stop_;
        });
        if (queue_.empty())
            return false;
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    std::size_t size() const {
        std::scoped_lock lock(mtx_);
        return queue_.size();
    }

    bool empty() const {
        std::scoped_lock lock(mtx_);
        return queue_.empty();
    }

    void stop() {
        {
            std::scoped_lock lock(mtx_);
            stop_ = true;
        }
        cond_.notify_all();
    }

private:
    std::condition_variable cond_;
    mutable std::mutex mtx_;
    std::queue<T> queue_;
    bool stop_ = false;
};

class ThreadPool : public Singleton<ThreadPool> {
public:
    using WorkItem = std::function<void()>;

    // 提交任务
    template<typename Func, typename... Args>
    auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;

    // 打印当前线程池的状态
    void printStatus() const;

private:
    ThreadPool(size_t thread_num = std::thread::hardware_concurrency()) : p_thread_pool_impl(std::make_unique<ThreadPoolImpl>(thread_num)) {
        std::cout << "MultiplePool" << std::endl;
    }
    ~ThreadPool() {
        std::cout << "~MultiplePool" << std::endl;
    }
private:
    friend class Singleton<ThreadPool>;
    class ThreadPoolImpl;
    std::unique_ptr<ThreadPoolImpl> p_thread_pool_impl;
};

class ThreadPool::ThreadPoolImpl {
public:
    ThreadPoolImpl(size_t thread_num = std::thread::hardware_concurrency()) 
        : queues_(thread_num),
        thread_num_(thread_num),
        curThreadNum_(thread_num),
        idleThreadNum_(thread_num),
        totalTaskCount_(0) {
            std::cout << "ThreadPoolImpl" << std::endl;
        auto worker = [this](size_t id) {
            while (true) {
                WorkItem task{};
                if (!queues_[id].pop(task))
                    break;

                if (task) {
                    std::lock_guard<std::mutex> lock(mtx);
                    std::cout << "get task...." << std::endl;
                    idleThreadNum_--;
                    executeTask(task, id);
                    idleThreadNum_++;
                    std::cout << "finish task" << std::endl;
                    totalTaskCount_++;
                }
            }
        };

        workers_.reserve(thread_num);
        for (size_t i = 0; i < thread_num; ++i) {
            workers_.emplace_back(worker, i);
        }
    }

    ~ThreadPoolImpl() {
        std::cout << "~ThreadPoolImpl" << std::endl;
        for (auto& queue: queues_) {
            queue.stop();
        }
        for (auto& worker: workers_) {
            worker.join(); // 阻塞，等待每个线程执行结束
        }
    }

    int schedule_by_id(WorkItem fn, size_t id = 0) {
        if (!fn)
        return -1;

        if (id == 0) {
            static std::atomic<size_t> next_queue{0};
            id = next_queue.fetch_add(1) % thread_num_;
        }
        assert(id < thread_num_);
        queues_[id].push(std::move(fn));
        return 0;
    }

    void executeTask(WorkItem task, size_t id) {
        if (task) {
            std::cout << "ExecuteTask on thread " << std::this_thread::get_id() << " from queue " << id << " QueueSize:" << queues_[id].size() << std::endl;
            task();
            std::cout << "Finished task on thread " << std::this_thread::get_id() << " from queue " << id << " QueueSize:" << queues_[id].size() << std::endl;
        }
    }

    mutable std::mutex mtx;
    std::vector<SafeQueue<WorkItem>> queues_;
    size_t thread_num_;
    std::vector<std::thread> workers_;
    size_t curThreadNum_;
    size_t idleThreadNum_;
    std::atomic_int totalTaskCount_;
};

template <typename Func, typename... Args>
inline auto ThreadPool::submitTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> {
    {
        using ReturnType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<ReturnType> result = task->get_future();

        // 将 task 包装成 WorkItem
        WorkItem wrappedTask = [task = std::move(task)] { (*task)(); };
        p_thread_pool_impl->schedule_by_id(std::move(wrappedTask));

        return result;
    }
}

void ThreadPool::printStatus() const{
    std::cout << "Current status of the thread pool:" << std::endl;
            std::cout << "Total threads: " << p_thread_pool_impl->curThreadNum_ << " Idle threads: " << p_thread_pool_impl->idleThreadNum_ << " Total tasks submitted: " << p_thread_pool_impl->totalTaskCount_ << std::endl;

            for (size_t i = 0; i < p_thread_pool_impl->thread_num_; ++i) {
                std::cout << "Queue " << i << ": Size = " << p_thread_pool_impl->queues_[i].size() << std::endl;
            }
}

#endif

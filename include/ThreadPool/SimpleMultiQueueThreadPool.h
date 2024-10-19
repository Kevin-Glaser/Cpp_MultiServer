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


using WorkItem = std::function<void()>;
class MultiplePool {
public:
    explicit MultiplePool(size_t thread_num = std::thread::hardware_concurrency())
        : queues_(thread_num),
            thread_num_(thread_num) {
        auto worker = [this](size_t id) {
            while (true) {
                WorkItem task{};
                if (!queues_[id].pop(task))
                    break;

                if (task)
                    task();
            }
        };

        workers_.reserve(thread_num_);
        for (size_t i = 0; i < thread_num_; ++i) {
            workers_.emplace_back(worker, i);
        }
    }

    int schedule_by_id(WorkItem fn, size_t id = 0) {
        if (fn == nullptr)
            return -1;

        if (id == 0) {
            id = rand() % thread_num_;
            queues_[id].push(std::move(fn));
        } else {
            assert(id < thread_num_);
            queues_[id].push(std::move(fn));
        }

        return 0;
    }

    ~MultiplePool() {
        for (auto& queue: queues_) {
            queue.stop();
        }
        for (auto& worker: workers_) {
            worker.join();// 阻塞，等待每个线程执行结束
        }
    }

private:
    std::vector<SafeQueue<WorkItem>> queues_;
    size_t thread_num_;
    std::vector<std::thread> workers_;
};


#endif
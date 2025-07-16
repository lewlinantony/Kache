#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    // Constructor starts a fixed number of worker threads
    ThreadPool(size_t threads);
    
    // Destructor joins all threads
    ~ThreadPool();

    // Add a new task to the queue
    void enqueue(std::function<void()> task);

private:
    // Worker threads
    std::vector<std::thread> workers;
    
    // Task queue
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


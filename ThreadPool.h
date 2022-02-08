#pragma once

#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  Runnable() = default;
  virtual ~Runnable() {}
  virtual void run() = 0;
  virtual bool stopped() const = 0;
};

class ThreadPool {
  std::vector<std::thread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<RunnablePtr> _queue;
 public:
  ThreadPool(unsigned numberThreads);
  ~ThreadPool();
  void stop();
  void push(RunnablePtr runnable);
  RunnablePtr get();
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <thread>
#include <vector>

class ThreadPoolSessions {
  void createThread();
  ThreadPoolSessions& operator =(const ThreadPoolSessions& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::deque<RunnablePtr> _queue;
  const int _maxNumberRunningTotal;
  std::atomic_flag _stopFlag;
  static std::shared_ptr<class KillThread> _killThread;
  public:
  ThreadPoolSessions(int maxNumberRunningTotal);
  ~ThreadPoolSessions();
  ThreadPoolSessions(const ThreadPoolSessions& other) = delete;
  void stop();
  void push(RunnablePtr runnable, std::function<bool(RunnablePtr)> func = nullptr);
  RunnablePtr get();
  int size() const { return _threads.size(); }
  void removeFromQueue(RunnablePtr toRemove);
};

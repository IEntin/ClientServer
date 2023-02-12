/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"

class ThreadPoolSession : public ThreadPool {
  ThreadPoolSession& operator =(const ThreadPoolSession& other) = delete;
  const int _maxNumberRunningTotal;
  public:
  ThreadPoolSession(int maxNumberRunningTotal);
  ~ThreadPoolSession() override;
  ThreadPoolSession(const ThreadPoolSession& other) = delete;
  RunnablePtr get() override;
  void push(RunnablePtr runnable, std::function<bool(RunnablePtr)> func = nullptr) override;
};

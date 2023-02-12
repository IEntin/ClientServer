/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"

class ThreadPoolDiffObj : public ThreadPool {
  ThreadPoolDiffObj& operator =(const ThreadPoolDiffObj& other) = delete;
  const int _maxNumberRunningTotal;
  std::function<bool(RunnablePtr)> _func = nullptr;
public:
  explicit ThreadPoolDiffObj(int maxNumberRunningTotal, std::function<bool(RunnablePtr)> func = nullptr);
  ~ThreadPoolDiffObj() override;
  ThreadPoolDiffObj(const ThreadPoolDiffObj& other) = delete;
  RunnablePtr get() override;
  void push(RunnablePtr runnable) override;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <functional>

class ThreadPoolDiffObj : public ThreadPool {
  ThreadPoolDiffObj(const ThreadPoolDiffObj& other) = delete;
  ThreadPoolDiffObj& operator =(const ThreadPoolDiffObj& other) = delete;
  std::function<bool(RunnablePtr)> _func = nullptr;
public:
  explicit ThreadPoolDiffObj(int maxSize, std::function<bool(RunnablePtr)> func = nullptr);
  ~ThreadPoolDiffObj() override;
  RunnablePtr get() override;
  void push(RunnablePtr runnable) override;
};

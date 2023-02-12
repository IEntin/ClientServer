/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

enum class STATUS : char;

class ThreadPool : public ThreadPoolBase {
protected:
  ThreadPool& operator =(const ThreadPool& other) = delete;
  const int _maxSize;
public:
  explicit ThreadPool(int maxSize = MAX_NUMBER_THREADS_DEFAULT);
  ~ThreadPool() override;
  ThreadPool(const ThreadPool& other) = delete;
  void push(RunnablePtr runnable, std::function<bool(RunnablePtr)> func = nullptr) override;
  int maxSize() const { return _maxSize; }
};

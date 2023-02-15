/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

enum class STATUS : char;

class ThreadPoolSameObj : public ThreadPoolBase {
protected:
  ThreadPoolSameObj(const ThreadPoolSameObj& other) = delete;
  ThreadPoolSameObj& operator =(const ThreadPoolSameObj& other) = delete;
public:
  explicit ThreadPoolSameObj(int maxSize = MAX_NUMBER_THREADS_DEFAULT);
  ~ThreadPoolSameObj() override;
  void push(RunnablePtr runnable) override;
};

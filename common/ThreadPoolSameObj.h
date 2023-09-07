/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

class ThreadPoolSameObj : public ThreadPoolBase {
protected:
  ThreadPoolSameObj(const ThreadPoolSameObj& other) = delete;
  ThreadPoolSameObj& operator =(const ThreadPoolSameObj& other) = delete;
public:
  explicit ThreadPoolSameObj(int maxSize);
  ~ThreadPoolSameObj() override;
  void push(RunnablePtr runnable) override;
  RunnablePtr get() override;
};

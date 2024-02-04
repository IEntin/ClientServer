/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

class ThreadPoolDiffObj : public ThreadPoolBase {
public:
  explicit ThreadPoolDiffObj(int maxSize);
  ~ThreadPoolDiffObj() override;
  RunnablePtr get() override;
  void calculateStatus(RunnablePtr runnable);
  STATUS push(RunnablePtr runnable) override;
};

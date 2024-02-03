/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <functional>

#include "ThreadPoolBase.h"

class ThreadPoolDiffObj : public ThreadPoolBase {
  std::function<bool(RunnablePtr)> _func = nullptr;
public:
  explicit ThreadPoolDiffObj(int maxSize, std::function<bool(RunnablePtr)> func = nullptr);
  ~ThreadPoolDiffObj() override;
  RunnablePtr get() override;
  STATUS push(RunnablePtr runnable) override;
};

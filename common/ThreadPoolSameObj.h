/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

class ThreadPoolSameObj : public ThreadPoolBase {
public:
  explicit ThreadPoolSameObj(int maxSize);
  ~ThreadPoolSameObj() override;
  STATUS push(RunnablePtr runnable) override;
  RunnablePtr get() override;
};

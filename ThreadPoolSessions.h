/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

class ThreadPoolSessions : public ThreadPoolBase {
public:
  explicit ThreadPoolSessions(int maxSize);
  void calculateStatus(RunnablePtr runnable);
  STATUS push(RunnablePtr runnable) override;
  RunnablePtr get() override;
};

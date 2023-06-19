/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <functional>

class WaitSignal : public RunnableT<WaitSignal> {
  void run() override;
  void stop() override;
  std::atomic<ACTIONS>& _flag;
  std::function<void()> _func;
public:
  WaitSignal(std::atomic<ACTIONS>& flag, std::function<void()> func);
  ~WaitSignal() override;
};

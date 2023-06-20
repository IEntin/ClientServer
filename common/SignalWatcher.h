/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <functional>

class SignalWatcher : public RunnableT<SignalWatcher> {
  void run() override;
  void stop() override;
  std::atomic<ACTIONS>& _flag;
  std::function<void()> _func;
public:
  SignalWatcher(std::atomic<ACTIONS>& flag, std::function<void()> func);
  ~SignalWatcher() override;
};

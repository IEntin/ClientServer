/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <functional>

class SignalWatcher : public Runnable {
  bool start() override { return true; }
  void run() override;
  void stop() override;
  unsigned getNumberObjects() const override { return 1; }
  unsigned getNumberRunningByType() const override { return 1; }
  void displayCapacityCheck(std::atomic<unsigned>&) const override {}
  std::atomic<ACTIONS>& _flag;
  std::function<void()> _func;
public:
  SignalWatcher(std::atomic<ACTIONS>& flag, std::function<void()> func);
  ~SignalWatcher() override;
};

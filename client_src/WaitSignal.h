/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

enum class ACTIONS : int {
  NONE,
  ACTION,
  STOP
};

class WaitSignal : public RunnableT<WaitSignal> {
  void run() override;
  bool start() override { return true; }
  void stop() override;
  std::atomic<ACTIONS>& _flag;
  std::string _fifoName;
 public:
  WaitSignal(std::atomic<ACTIONS>& flag, const std::string& fileName);
  ~WaitSignal() override;
};

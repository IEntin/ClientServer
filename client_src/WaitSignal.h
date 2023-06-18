/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"

enum class ACTIONS : int {
  NONE,
  ACTION,
  STOP
};

class ThreadPoolBase;

class WaitSignal : public RunnableT<WaitSignal> {
  void run() override;
  bool start() override { return true; }
  void stop() override;
  std::atomic<ACTIONS>& _flag;
  std::string _fifoName;
  ThreadPoolReference<ThreadPoolBase> _threadPool;
 public:
  WaitSignal(std::atomic<ACTIONS>& flag,
	     const std::string& fileName,
	     ThreadPoolBase& threadPool);
  ~WaitSignal() override;
};

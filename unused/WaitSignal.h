/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"
#include <functional>

class ThreadPoolBase;

class WaitSignal : public std::enable_shared_from_this<WaitSignal>,
  public RunnableT<WaitSignal> {
  void run() override;
  bool start() override;
  void stop() override;
  std::atomic_flag& _flag;
  std::function<bool()> _func = nullptr;
  ThreadPoolReference<ThreadPoolBase> _threadPoolClient;
  std::atomic<bool> _stopped = false;
 public:
  WaitSignal(std::atomic_flag& flag, std::function<bool()> func, ThreadPoolBase& threadPoolClient);
  ~WaitSignal() override;
};

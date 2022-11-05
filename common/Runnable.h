/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CommonConstants.h"
#include "Header.h"
#include <atomic>
#include <memory>
#include <string>

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  Runnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT);
  virtual ~Runnable();
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  virtual unsigned getNumberObjects() const = 0;
  virtual void checkCapacity();
  const unsigned _maxNumberThreads;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
};

class KillThread : public Runnable {
  void run() override {}
  bool start() override { return true; }
  void stop() override {}
  unsigned getNumberObjects() const override { return 0; }
 public:
  KillThread() = default;
  ~KillThread() override {}
  bool killThread() const override { return true; }
};

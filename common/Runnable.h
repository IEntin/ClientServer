/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <memory>
#include <string>

constexpr unsigned MAX_NUMBER_THREADS_DEFAULT = 1000;

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  Runnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT);
  virtual ~Runnable();
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  virtual unsigned getNumberObjects() const { return 0; }
  virtual void checkCapacity();
  const unsigned _maxNumberThreads;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
};

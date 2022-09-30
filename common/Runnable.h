/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>
#include <memory>
#include <string>

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  Runnable(unsigned maxNumberThreads = 0);
  virtual ~Runnable();
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  virtual unsigned getNumberObjects() const { return 0; }
  virtual PROBLEMS checkCapacity() const;
  virtual PROBLEMS getStatus() const { return PROBLEMS::NONE; }
  const unsigned _maxNumberThreads;
};

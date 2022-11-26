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

using RunnableWeakPtr = std::weak_ptr<class Runnable>;

class Runnable {
 public:
  explicit Runnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT);
  virtual ~Runnable() {}
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

// The Curiously Recurring Template Pattern (CRTP)
// Objects of specific derived class counting
template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    Runnable(maxNumberThreads) { _numberObjects++; }
    ~RunnableT() override { _numberObjects--; }
  static std::atomic<unsigned> _numberObjects;
 public:
  unsigned getNumberObjects() const override {
    return _numberObjects;
  }
};
template <typename T>
std::atomic<unsigned>  RunnableT<T>::_numberObjects;

class KillThread : public RunnableT<KillThread> {
  void run() override {}
  bool start() override { return true; }
  void stop() override {}
 public:
  KillThread() = default;
  ~KillThread() override {}
  bool killThread() const override { return true; }
};

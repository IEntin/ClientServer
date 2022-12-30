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
  explicit Runnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    _maxNumberThreads(maxNumberThreads) {}
  virtual ~Runnable() {}
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  virtual unsigned getNumberObjects() const = 0;
  virtual unsigned getNumberRunning() const = 0;
  virtual void checkCapacity() {
    if (getNumberObjects() > _maxNumberThreads)
      _status = STATUS::MAX_SPECIFIC_SESSIONS;
  }
  virtual std::string_view getType() const = 0;
  STATUS getStatus() const { return _status; }

  const unsigned _maxNumberThreads;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
};

// The Curiously Recurring Template Pattern (CRTP)
// Counting objects of specific derived class

template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    Runnable(maxNumberThreads) { _numberObjects++; }
  ~RunnableT() override { _numberObjects--; }
  struct CountRunning {
    CountRunning() { _numberRunning++; }
    ~CountRunning() { _numberRunning--; }
  };
  std::string_view getType() const override { return _type; }
  static inline std::atomic<unsigned> _numberObjects;
  static inline std::atomic<unsigned> _numberRunning;
  static inline const std::string _type = typeid(T).name();
 public:
  unsigned getNumberObjects() const override {
    return _numberObjects;
  }
  unsigned getNumberRunning() const override {
    return _numberRunning;
  }
};

class KillThread : public RunnableT<KillThread> {
  void run() override {}
  bool start() override { return true; }
  void stop() override {}
 public:
  KillThread() = default;
  ~KillThread() override {}
  bool killThread() const override { return true; }
};

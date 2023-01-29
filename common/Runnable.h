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
  explicit Runnable(int maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    _maxNumberThreads(maxNumberThreads) {}
  virtual ~Runnable() {}
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  virtual int getNumberObjects() const = 0;
  virtual void checkCapacity() {
    if (getNumberObjects() > _maxNumberThreads)
      _status = STATUS::MAX_SPECIFIC_SESSIONS;
  }
  virtual std::string_view getType() const = 0;
  virtual bool notify([[maybe_unused]] std::string_view stopping) { return false; }
  std::atomic<STATUS>& getStatus() { return _status; }

  std::atomic<bool> _waiting = false;
  const int _maxNumberThreads;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
  static inline std::atomic<int> _totalRunning = 0;
};

// The Curiously Recurring Template Pattern (CRTP)
// Counting objects of specific derived class

template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(int maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    Runnable(maxNumberThreads) { _numberObjects++; }
  ~RunnableT() override { _numberObjects--; }
  std::string_view getType() const override { return _type; }
  struct DecrementRunning {
    DecrementRunning() = default;
    ~DecrementRunning() {
      _numberRunning--;
      _totalRunning--;
    }
  };
  static inline std::atomic<int> _numberObjects = 0;
  static inline std::atomic<int> _numberRunning = 0;
  static inline const std::string _type = typeid(T).name();
 public:
  int getNumberObjects() const override {
    return _numberObjects;
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

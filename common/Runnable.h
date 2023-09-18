/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CommonConstants.h"
#include "Header.h"
#include <atomic>
#include <memory>
#include <string_view>

using RunnablePtr = std::shared_ptr<class Runnable>;

using RunnableWeakPtr = std::weak_ptr<class Runnable>;

class Runnable {
 public:
  explicit Runnable(int maxNumberRunningByType = MAX_NUMBER_THREADS_DEFAULT) :
    _maxNumberRunningByType(maxNumberRunningByType) {}
  Runnable(Runnable&) = delete;
  virtual ~Runnable() {}
  Runnable& operator=(Runnable&) = delete;
  virtual bool start() = 0;
  virtual void run() = 0;
  virtual void stop() = 0;
  virtual std::string_view getId() { return {}; }
  virtual unsigned getNumberObjects() const = 0;
  virtual unsigned getNumberRunningByType() const = 0;
  virtual bool sendStatusToClient() { return true; }
  virtual std::string_view getDisplayName() const { return _emptyView; }
  std::string_view getType() const;
  void displayCapacityCheck(std::atomic<unsigned>&) const;
  std::atomic<STATUS>& getStatus() { return _status; }
  bool checkCapacity();
  const unsigned _maxNumberRunningByType;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
  static constexpr std::string_view _emptyView{};
  static std::atomic<unsigned> _numberRunningTotal;
};

// The Curiously Recurring Template Pattern (CRTP)
// Counting objects of specific derived class

template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(int maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    Runnable(maxNumberThreads) { _numberObjects++; }
  ~RunnableT() override { _numberObjects--; }
  static inline std::atomic<unsigned> _numberObjects = 0;
  static inline std::atomic<unsigned> _numberRunningByType = 0;
 public:
  struct CountRunning {
    CountRunning() { _numberRunningByType++; _numberRunningTotal++; }
    ~CountRunning() { _numberRunningByType--; _numberRunningTotal--; }
  };
  unsigned getNumberObjects() const override {
    return _numberObjects;
  }
  unsigned getNumberRunningByType() const override { return _numberRunningByType; }
};

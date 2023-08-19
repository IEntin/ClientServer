/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CommonConstants.h"
#include "Header.h"
#include "Logger.h"
#include <atomic>
#include <memory>
#include <string>

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
  virtual std::string_view getId() { return std::string_view(); }
  virtual bool killThread() const { return false; }
  virtual int getNumberObjects() const = 0;
  virtual int getNumberRunningByType() const = 0;
  virtual void displayCapacityCheck([[maybe_unused]] std::atomic<int>& totalNumberObjects) const = 0;
  virtual std::string_view getType() const = 0;
  virtual bool sendStatusToClient() { return true; }
  std::atomic<STATUS>& getStatus() { return _status; }
  void checkCapacity() {
    if (getNumberObjects() > _maxNumberRunningByType)
      _status = STATUS::MAX_OBJECTS_OF_TYPE;
  }
  const int _maxNumberRunningByType;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
  static inline std::atomic<int> _numberRunningTotal = 0;
};

// The Curiously Recurring Template Pattern (CRTP)
// Counting objects of specific derived class

template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(int maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT,
		     std::string_view displayType = std::string_view()) :
    Runnable(maxNumberThreads) {_numberObjects++; _displayType = displayType; }
  ~RunnableT() override { _numberObjects--; }
  std::string_view getType() const override { return _type; }
  static inline std::string_view _displayType;
  static inline std::atomic<int> _numberObjects = 0;
  static inline std::atomic<int> _numberRunningByType = 0;
  static inline const std::string _type = typeid(T).name();
 public:
  struct CountRunning {
    CountRunning() { _numberRunningByType++; _numberRunningTotal++; }
    ~CountRunning() { _numberRunningByType--; _numberRunningTotal--; }
  };
  int getNumberObjects() const override {
    return _numberObjects;
  }
  int getNumberRunningByType() const override { return _numberRunningByType; }

  void displayCapacityCheck(std::atomic<int>& totalNumberObjects) const override {
    Info << "Number " << _displayType << " sessions=" << _numberObjects
	 << ", Number running " << _displayType << " sessions=" << _numberRunningByType
	 << ", max number " << _displayType << " running=" << _maxNumberRunningByType
	 << '\n';
  switch (_status.load()) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "\nThe number of " << _displayType << " sessions=" << _numberObjects
	 << " exceeds thread pool capacity." << '\n';
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal sessions=" << totalNumberObjects
	 << " exceeds system capacity." << '\n';
    break;
  default:
    break;
  }
}
};

class KillThread : public RunnableT<KillThread> {
  void run() override {}
  bool start() override { return true; }
  void stop() override {}
  void displayCapacityCheck([[maybe_unused]] std::atomic<int>& totalNumberObjects) const override {}
 public:
  KillThread() = default;
  ~KillThread() override {}
  bool killThread() const override { return true; }
};

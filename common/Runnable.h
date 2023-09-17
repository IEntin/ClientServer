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
  virtual std::string_view getId() { return {}; }
  virtual unsigned getNumberObjects() const = 0;
  virtual unsigned getNumberRunningByType() const = 0;
  virtual void displayCapacityCheck(std::atomic<unsigned>&) const = 0;
  virtual bool sendStatusToClient() { return true; }
  std::string_view getType() const { return typeid(*this).name(); }
  std::atomic<STATUS>& getStatus() { return _status; }
  bool checkCapacity() {
    if (getNumberObjects() > _maxNumberRunningByType) {
      _status = STATUS::MAX_OBJECTS_OF_TYPE;
      return false;
    }
    return true;
  }
  const unsigned _maxNumberRunningByType;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
  static std::atomic<unsigned> _numberRunningTotal;
};

// The Curiously Recurring Template Pattern (CRTP)
// Counting objects of specific derived class

template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(int maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT,
		     std::string_view displayType = {}) :
    Runnable(maxNumberThreads) { _numberObjects++; _displayType = displayType; }
  ~RunnableT() override { _numberObjects--; }
  static inline std::string_view _displayType;
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

  void displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const override {
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

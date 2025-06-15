/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <memory>

#include <boost/core/noncopyable.hpp>

#include "Header.h"

inline constexpr int MAX_NUMBER_THREADS_DEFAULT = 1000;

using RunnablePtr = std::shared_ptr<class Runnable>;

using RunnableWeakPtr = std::weak_ptr<class Runnable>;

class Runnable : private boost::noncopyable {
 public:
  explicit Runnable(int maxNumberRunningByType = MAX_NUMBER_THREADS_DEFAULT) :
    _maxNumberRunningByType(maxNumberRunningByType) {}
  virtual ~Runnable() = default;
  virtual bool start() = 0;
  virtual void run() = 0;
  virtual void stop() = 0;
  virtual unsigned getNumberObjects() const { return 0; }
  virtual unsigned getNumberRunningByType() const { return 0; }
  virtual void displayCapacityCheck(std::atomic<unsigned>&) const {}
  std::string getType() const;
  std::atomic<bool> _stopped = false;
  std::atomic<STATUS> _status = STATUS::NONE;
  const unsigned _maxNumberRunningByType;
  inline static std::atomic<unsigned> _numberRunningTotal = 0;
};

// Every template instantiation has a distinct static member.
// Used here to count objects of the specific type.
template <class T>
class RunnableT : public Runnable {
 protected:
  explicit RunnableT(int maxNumberThreads) :
    Runnable(maxNumberThreads) { _numberObjects++; }
  ~RunnableT() override { _numberObjects--; }
  inline static std::atomic<unsigned> _numberObjects = 0;
  inline static std::atomic<unsigned> _numberRunningByType = 0;
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

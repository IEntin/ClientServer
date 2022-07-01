/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

class Runnable {
 public:
  explicit Runnable(std::atomic<int>& combinedNumberInstances = _dummyInitializer1,
		    std::atomic<int>& numberInstances = _dummyInitializer2);
  virtual ~Runnable();
  virtual void run() = 0;
 private:
  // used for total connections
  std::atomic<int>& _combinedNumberInstances;
  // used for specific connection type
  std::atomic<int>& _numberInstances;
  static std::atomic<int> _dummyInitializer1;
  static std::atomic<int> _dummyInitializer2;
};

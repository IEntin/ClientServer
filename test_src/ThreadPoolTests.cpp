/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ObjectCounter.h"
#include "Runnable.h"
#include "ThreadPool.h"
#include <gtest/gtest.h>

class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
public:
  TestRunnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    Runnable(maxNumberThreads) {}

  ~TestRunnable() override {}
  void run() override {
    while (!_stopFlag)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  bool start() override {
    checkCapacity();
    return true;
  }
  void stop() override {}

  unsigned getNumberObjects() const override {
    return _objectCounter._numberObjects;
  }

  ObjectCounter<TestRunnable> _objectCounter;
  static std::atomic<bool> _stopFlag;
};
std::atomic<bool> TestRunnable::_stopFlag = false;

TEST(ThreadPoolTest, Fixed) {
  unsigned maxNumberThreads = 10;
  ThreadPool pool(maxNumberThreads);
  for (unsigned i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnable->start();
    if (i < maxNumberThreads)
      ASSERT_TRUE(runnable->_problem == PROBLEMS::NONE);
    else
      ASSERT_TRUE(runnable->_problem == PROBLEMS::MAX_NUMBER_RUNNABLES);
    pool.push(runnable);
  }
  ASSERT_TRUE(pool.size() == pool.maxSize());
  TestRunnable::_stopFlag.store(true);
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
  TestRunnable::_stopFlag.store(false);
}

TEST(ThreadPoolTest, Variable) {
  ThreadPool pool;
  const unsigned numberObjects = 20;
  for (unsigned i = 0; i < numberObjects; ++i) {
    auto runnable = std::make_shared<TestRunnable>();
    runnable->start();
    ASSERT_TRUE(runnable->_problem == PROBLEMS::NONE);
    pool.push(runnable);
    ASSERT_TRUE(runnable->getNumberObjects() == pool.size());
  }
  ASSERT_TRUE(pool.size() == numberObjects);
  TestRunnable::_stopFlag.store(true);
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
  TestRunnable::_stopFlag.store(false);
}

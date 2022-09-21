/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ObjectCount.h"
#include "Runnable.h"
#include "ThreadPool.h"
#include <gtest/gtest.h>

class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
public:
  TestRunnable() {}

  ~TestRunnable() override {}
  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  bool start() override {
    return true;
  }
  void stop() override {}

  unsigned getNumberObjects() const override {
    return _objectCount._numberObjects;
  }

  PROBLEMS checkCapacity() const override {
    if (_objectCount._numberObjects > _maxNumberThreads)
      return PROBLEMS::MAX_NUMBER_RUNNABLES;
    else
      return PROBLEMS::NONE;
  }
  static const unsigned _maxNumberThreads = 10;
  ObjectCount<TestRunnable> _objectCount;
};

TEST(ThreadPoolTest, Fixed) {
  ThreadPool pool(TestRunnable::_maxNumberThreads);
  for (unsigned i = 0; i < 2 * TestRunnable::_maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>();
    runnable->start();
    PROBLEMS problem = pool.push(runnable);
    if (i < TestRunnable::_maxNumberThreads) {
      ASSERT_TRUE(problem == PROBLEMS::NONE);
    }
  }
  ASSERT_TRUE(pool.getThreads().size() == pool.size());
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

TEST(ThreadPoolTest, Variable) {
  ThreadPool pool;
  const unsigned numberObjects = 20;
  for (unsigned i = 0; i < numberObjects; ++i) {
    auto runnable = std::make_shared<TestRunnable>();
    runnable->start();
    // if run() did not return yet in some runnables,
    // it should be pool.getThreads().size() == i + 1
    ASSERT_TRUE(runnable->getNumberObjects() <= i + 1);
    PROBLEMS problem = pool.push(runnable);
    ASSERT_TRUE(problem == PROBLEMS::NONE);
  }
  // if run() did not return yet in some runnables,
  // it should be pool.getThreads().size() == numberObjects
  ASSERT_TRUE(pool.getThreads().size() <= numberObjects);
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

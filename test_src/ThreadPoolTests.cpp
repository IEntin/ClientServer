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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  bool start() override {
    return true;
  }
  void stop() override {}

  unsigned getNumberObjects() const override {
    return _objectCounter._numberObjects;
  }

  ObjectCounter<TestRunnable> _objectCounter;
};

TEST(ThreadPoolTest, Fixed) {
  unsigned maxNumberThreads = 10;
  ThreadPool pool(maxNumberThreads);
  for (unsigned i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnable->start();
    pool.push(runnable);
  }
  ASSERT_TRUE(pool.size() == pool.maxSize());
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
    // it should be pool.size() == pool.size (i + 1)
    pool.push(runnable);
    ASSERT_TRUE(runnable->getNumberObjects() <= pool.size());
  }
  // if run() did not return yet in some runnables,
  // it should be pool.size() == numberObjects
  ASSERT_TRUE(pool.size() <= numberObjects);
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

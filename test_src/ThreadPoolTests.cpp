/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>

#include "ThreadPoolDiffObj.h"
#include "ThreadPoolSameObj.h"

// for i in {1..10}; do ./testbin --gtest_filter=ThreadPoolTest*; done

using TestRunnablePtr = std::shared_ptr<class TestRunnable>;

class TestRunnable : public RunnableT<TestRunnable> {
public:
  TestRunnable(int maxNumberThreads) :
    RunnableT(maxNumberThreads) {}

  ~TestRunnable() override {}
  void run() override {
    while (!_stopped)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  bool start() override {
    checkCapacity();
    return true;
  }
  void stop() override {
    _stopped = true;
  }
  bool _running = false;
};

TEST(ThreadPoolTest, Same) {
  std::vector<TestRunnablePtr> runnables;
  int maxNumberThreads = 10;
  ThreadPoolSameObj pool(maxNumberThreads);
  for (int i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnables.emplace_back(runnable);
    runnable->start();
    if (i < maxNumberThreads)
      ASSERT_TRUE(runnable->_status == STATUS::NONE);
    else
      ASSERT_TRUE(runnable->_status == STATUS::MAX_OBJECTS_OF_TYPE);
    pool.push(runnable);
  }
  ASSERT_TRUE(pool.size() == pool.maxSize());
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (RunnablePtr runnable : runnables)
    runnable->stop();
  pool.stop();
  ASSERT_TRUE(pool.size() == 0);
}

TEST(ThreadPoolTest, Diff) {
  std::vector<TestRunnablePtr> runnables;
  const unsigned maxNumberThreads = 20;
  ThreadPoolDiffObj pool(maxNumberThreads);
  for (unsigned i = 0; i < maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnables.emplace_back(runnable);
    runnable->start();
    ASSERT_TRUE(runnable->_status == STATUS::NONE);
    pool.calculateStatus(runnable);
    pool.push(runnable);
    std::string type = runnable->getType();
    ASSERT_TRUE(boost::contains(type, "TestRunnable"));
    ASSERT_TRUE(pool.size() == i + 1);
    ASSERT_TRUE(runnable->getNumberObjects() == pool.size());
  }
  ASSERT_TRUE(pool.size() == maxNumberThreads);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (RunnablePtr runnable : runnables)
    runnable->stop();
  pool.stop();
  ASSERT_TRUE(pool.size() == 0);
}

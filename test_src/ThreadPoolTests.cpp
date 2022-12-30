/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Logger.h"
#include <gtest/gtest.h>
#include <boost/algorithm/string.hpp>

class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public RunnableT<TestRunnable> {
public:
  TestRunnable(unsigned maxNumberThreads = MAX_NUMBER_THREADS_DEFAULT) :
    RunnableT(maxNumberThreads) {}

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

  static std::atomic<bool> _stopFlag;
};
std::atomic<bool> TestRunnable::_stopFlag;

TEST(ThreadPoolTest, Fixed) {
  unsigned maxNumberThreads = 10;
  ThreadPool pool(maxNumberThreads);
  for (unsigned i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnable->start();
    if (i < maxNumberThreads)
      ASSERT_TRUE(runnable->_status == STATUS::NONE);
    else
      ASSERT_TRUE(runnable->_status == STATUS::MAX_SPECIFIC_SESSIONS);
    pool.push(runnable);
  }
  ASSERT_TRUE(pool.size() == pool.maxSize());
  TestRunnable::_stopFlag = true;
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
  TestRunnable::_stopFlag = false;
}

TEST(ThreadPoolTest, Variable) {
  ThreadPool pool;
  const unsigned numberObjects = 20;
  for (unsigned i = 0; i < numberObjects; ++i) {
    auto runnable = std::make_shared<TestRunnable>();
    runnable->start();
    ASSERT_TRUE(runnable->_status == STATUS::NONE);
    pool.push(runnable);
    const auto& refObject = *runnable;
    std::string type = typeid(refObject).name();
    ASSERT_TRUE(boost::contains(type, "TestRunnable"));
    ASSERT_TRUE(pool.size() == i + 1);
    ASSERT_TRUE(runnable->getNumberObjects() == pool.size());
  }
  ASSERT_TRUE(pool.size() == numberObjects);
  TestRunnable::_stopFlag = true;
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
  TestRunnable::_stopFlag = false;
}

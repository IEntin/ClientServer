/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <gtest/gtest.h>

#include "ThreadPoolSessions.h"

// for i in {1..10}; do ./testbin --gtest_filter=ThreadPoolTest*; done

using TestRunnablePtr = std::shared_ptr<class TestRunnable>;

class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public RunnableT<TestRunnable> {
public:
  explicit TestRunnable(int maxNumberThreads) :
    RunnableT(maxNumberThreads) {}

  ~TestRunnable() override = default;
  void run() override {
    while (!_stopped)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  bool start() override {
    return true;
  }
  void stop() override {
    _stopped = true;
  }
  bool _running = false;
  bool sendStatusToClient() {
    _running = _status == STATUS::NONE;
    return true;
  }
};

TEST(ThreadPoolTest, Base) {
  std::vector<TestRunnablePtr> runnables;
  int maxNumberThreads = 10;
  ThreadPoolBase pool(maxNumberThreads);
  for (int i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnables.emplace_back(runnable);
    runnable->start();
    STATUS status = pool.push(runnable);
    ASSERT_TRUE(status == runnable->_status);
    if (i < maxNumberThreads)
      ASSERT_TRUE(runnable->_status == STATUS::NONE);
    else
      ASSERT_TRUE(runnable->_status == STATUS::MAX_TOTAL_OBJECTS);
  }
  ASSERT_TRUE(pool.size() == pool.maxSize());
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (RunnablePtr runnable : runnables)
    runnable->stop();
  pool.stop();
  ASSERT_TRUE(pool.size() == 0);
}

TEST(ThreadPoolTest, Sessions) {
  std::vector<TestRunnablePtr> runnables;
  const unsigned maxNumberThreads = 20;
  ThreadPoolSessions pool(maxNumberThreads);
  for (unsigned i = 0; i < maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(maxNumberThreads);
    runnables.emplace_back(runnable);
    runnable->start();
    ASSERT_TRUE(runnable->_status == STATUS::NONE);
    pool.calculateStatus(runnable);
    runnable->sendStatusToClient();
    pool.push(runnable);
    std::string type = runnable->getType();
    ASSERT_TRUE(type.contains("TestRunnable"));
    ASSERT_TRUE(pool.size() == i + 1);
    ASSERT_TRUE(runnable->getNumberObjects() == pool.size());
  }
  for (auto runnable : runnables)
    ASSERT_TRUE(runnable->_running);
  ASSERT_TRUE(pool.size() == maxNumberThreads);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  for (RunnablePtr runnable : runnables)
    runnable->stop();
  pool.stop();
  ASSERT_TRUE(pool.size() == 0);
}

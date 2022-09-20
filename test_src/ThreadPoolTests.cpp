/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "ThreadPool.h"
#include <gtest/gtest.h>

class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
public:
  TestRunnable(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {
    _numberObjects++;
  }
  ~TestRunnable() override {
    _numberObjects--;
  }
  void run() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  bool start() override {
    return true;
  }
  void stop() override {}

  unsigned getNumberObjects() const override {
    return _numberObjects;
  }

  PROBLEMS checkCapacity() const override {
    if (_numberObjects > _maxNumberThreads)
      return PROBLEMS::MAX_NUMBER_RUNNABLES;
    else
      return PROBLEMS::NONE;
  }
  const unsigned _maxNumberThreads;
  static std::atomic<unsigned> _numberObjects;
};
std::atomic<unsigned> TestRunnable::_numberObjects;

TEST(ThreadPoolTest, Fixed) {
  const unsigned maxNumberThreads = 10;
  ThreadPool pool(maxNumberThreads);
  for (unsigned i = 0; i < 2 * maxNumberThreads; ++i) {
    auto runnable = std::make_shared<TestRunnable>(i);
    PROBLEMS problem[[maybe_unused]] = pool.push(runnable);
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
    auto runnable = std::make_shared<TestRunnable>(i);
    ASSERT_EQ(runnable->getNumberObjects(), i + 1);
    pool.push(runnable);
  }
  ASSERT_TRUE(pool.getThreads().size() == numberObjects);
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

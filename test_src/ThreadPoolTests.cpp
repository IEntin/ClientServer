/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "ThreadPool.h"
#include <gtest/gtest.h>

TEST(ThreadPoolTest, 1) {
  ThreadPoolPtr pool = std::make_shared<ThreadPool>(10);
  class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
  public:
    TestRunnable(int number, ThreadPoolPtr pool) :
      _number(number), _pool(pool->shared_from_this()) {}
    ~TestRunnable() override {
      EXPECT_EQ(_id, std::this_thread::get_id());
    }
    void run() override {
      _id = std::this_thread::get_id();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    void start() {
      _pool->push(shared_from_this());
    }
    const int _number;
    ThreadPoolPtr _pool;
    std::thread::id _id;
  };
  for (int i = 0; i < 20; ++i) {
    auto runnable = std::make_shared<TestRunnable>(i, pool);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable->start();
  }
  pool->stop();
  bool allJoined = true;
  for (auto& thread : pool->getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

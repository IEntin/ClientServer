#include "ThreadPool.h"
#include <gtest/gtest.h>

TEST(ThreadPoolTest, ThreadPoolTest1) {
  ThreadPoolPtr pool = std::make_shared<ThreadPool>(10);
  class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
  public:
    TestRunnable(unsigned number, ThreadPoolPtr pool) :
      Runnable(), _number(number), _pool(pool->shared_from_this()) {}
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
    const unsigned _number;
    ThreadPoolPtr _pool;
    std::thread::id _id;
  };
  for (unsigned i = 0; i < 20; ++i) {
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

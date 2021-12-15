#pragma once

#include <condition_variable>
#include <string>
#include <thread>
#include <vector>

using Batch = std::vector<std::string>;

namespace fifo {

class FifoRunnable {
  static std::vector<FifoRunnable> _runnables;
  static std::vector<std::thread> _threads;
  static const std::string _fifoDirectoryName;
  static volatile std::atomic<bool> _stopFlag;
  static std::mutex _stopMutex;
  static std::condition_variable _stopCondition;
  std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(Batch& batch);
  bool receiveRequest(std::vector<char>& uncompressed);
  bool sendResponse(Batch& response);
public:
  FifoRunnable(const std::string& receiveFifoName);
  ~FifoRunnable();
  void operator()() noexcept;
  static bool startThreads();
  static void joinThreads();
  static void stop();
  static void removeFifoFiles();
};

} // end of namespace fifo

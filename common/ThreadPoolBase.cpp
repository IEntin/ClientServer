/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolBase.h"
#include "Logger.h"

std::shared_ptr<KillThread> ThreadPoolBase::_killThread = std::make_shared<KillThread>();

ThreadPoolBase::ThreadPoolBase(int maxSize) : _maxSize(maxSize) {}

ThreadPoolBase::~ThreadPoolBase() {
  stop();
  Trace << std::endl;
}

void ThreadPoolBase::stop() {
  // It is safe and cheap to call this more than once.
  // Wake up and join threads
  try {
    for (int i = 0; i < size(); ++i)
      push(_killThread);
    for (auto it = _threads.begin(); it != _threads.end(); ) {
      auto& thread = *it;
      if (thread.joinable()) {
	// prevents core dump in case of a code defect.
	// Should not happen.
	if (thread.get_id() == std::this_thread::get_id()) {
	  thread.detach();
	  Warn << "thread detached." << std::endl;
	}
	else
	  thread.join();
      }
      it = _threads.erase(it);
    }
  }
  catch (const std::system_error& e) {
    LogError << e.what() << std::endl;
    return;
  }
  Trace << "... _threads joined ..." << std::endl;
}

void  ThreadPoolBase::createThread() {
  _threads.emplace_back([this] () {
    while (true) {
      // additional scope for fast recycling
      // of the finished runnable
      {
	// this blocks waiting for a new runnable
	RunnablePtr runnable = get();
	if (!runnable)
	  continue;
	// this kills the thread
	if (runnable->killThread())
	  break;
	try {
	  runnable->run();
	}
	catch (const std::exception& e) {
	  runnable->stop();
	  LogError << e.what() << std::endl;
	}
	catch (...) {
	  runnable->stop();
	  LogError << "exception caught." << std::endl;
	}
      }
    }
  });
}

void ThreadPoolBase::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  if (_totalNumberObjects > size()) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << std::endl;
  }
  _queue.emplace_back(runnable);
  _queueCondition.notify_all();
}

RunnablePtr ThreadPoolBase::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop_front();
  return runnable;
}

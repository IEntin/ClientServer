/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "WaitSignal.h"
#include "Logger.h"
#include "ThreadPoolBase.h"

WaitSignal::WaitSignal(std::atomic_flag& flag,
		       std::function<bool()> func,
		       ThreadPoolBase& threadPoolClient) :
  _flag(flag), _func(func), _threadPoolClient(threadPoolClient)
{}

WaitSignal::~WaitSignal() {
  LogError << std::endl;
}

bool WaitSignal::start() {
  _threadPoolClient.push(shared_from_this());
  return true;
}
		       
void WaitSignal::stop() {
  if (!_flag.test()) {
    LogError << std::endl;
    _flag.test_and_set();
    _flag.notify_all();
    _stopped.store(true);
  }
}

void WaitSignal::run() {
  _flag.wait(false);
  if (_func)
    _func();
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "WaitSignal.h"
#include "Logger.h"
#include "ThreadPoolBase.h"
#include <filesystem>

WaitSignal::WaitSignal(std::atomic<ACTIONS>& flag,
		       const std::string& fifoName,
		       ThreadPoolBase& threadPool) :
  _flag(flag), _fifoName(fifoName), _threadPool(threadPool) {}

WaitSignal::~WaitSignal() {
  Trace << std::endl;
}
	       
void WaitSignal::stop() {
  if (_flag == ACTIONS::NONE) {
    _flag.store(ACTIONS::STOP);
    _flag.notify_all();
  }
}

void WaitSignal::run() {
  _flag.wait(ACTIONS::NONE);
  try {
    if (_flag == ACTIONS::ACTION) {
      std::filesystem::remove(_fifoName);
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

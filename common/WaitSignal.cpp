/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "WaitSignal.h"
#include "Logger.h"

WaitSignal::WaitSignal(std::atomic<ACTIONS>& flag, std::function<void()> func) :
  _flag(flag), _func(func) {}

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
  if (_flag == ACTIONS::ACTION) {
    _func();
  }
}

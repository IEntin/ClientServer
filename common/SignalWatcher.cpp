/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SignalWatcher.h"
#include "Logger.h"

SignalWatcher::SignalWatcher(std::atomic<ACTIONS>& flag, std::function<void()> func) :
  _flag(flag), _func(func) {}

SignalWatcher::~SignalWatcher() {
  Trace << '\n';
}

void SignalWatcher::stop() {
  if (_flag == ACTIONS::NONE) {
    _flag.store(ACTIONS::STOP);
    _flag.notify_all();
  }
}

void SignalWatcher::run() {
  _flag.wait(ACTIONS::NONE);
  if (_flag == ACTIONS::ACTION) {
    _func();
  }
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <mutex>

#include "Crypto.h"
#include "Runnable.h"
#include "Subtask.h"

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line, long index);

  CryptoWeakPtr _crypto;
  std::string _aggregate;
  Subtasks _subtasks;
  std::mutex _mutex;
  std::condition_variable _conditionTask;
  std::condition_variable _conditionResume;
  bool _resume = false;
  std::string _buffer;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(CryptoWeakPtr crypto);
  void stop() override;
  void getTask(Subtasks& task);
  STATUS createSubtask(class Lines& lines);
  void resume();
};

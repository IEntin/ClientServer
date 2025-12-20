/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <mutex>

#include "Client.h"

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line, long index);

  static thread_local std::string _aggregate;
  Subtasks _subtasks;
  unsigned _subtaskIndex;
  std::mutex _mutex;
  std::condition_variable _conditionTask;
  std::condition_variable _conditionResume;
  bool _resume = false;
  ENCRYPTORCONTAINER& _crypto;
  void run() override;
  bool start() override { return true; }
 public:
  explicit TaskBuilder(ENCRYPTORCONTAINER& crypto);
  ~TaskBuilder() override = default;
  void stop() override;
  std::pair<std::size_t, STATUS> getTask(Subtasks& task);
  STATUS createSubtask(class Lines2& lines);
  void resume();
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <mutex>

#include "EncryptorTemplates.h"
#include "Runnable.h"
#include "Subtask.h"

#include "CryptoVariant.h"

class Client;

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line, long index);

  std::string _aggregate;
  Subtasks _subtasks;
  unsigned _subtaskIndex;
  std::mutex _mutex;
  std::condition_variable _conditionTask;
  std::condition_variable _conditionResume;
  bool _resume = false;
  ENCRYPTORCONTAINER _crypto;
  std::string _buffer;
  void run() override;
  bool start() override { return true; }
 public:
  explicit TaskBuilder(ENCRYPTORCONTAINER crypto);
  ~TaskBuilder() override = default;
  void stop() override;
  std::pair<std::size_t, STATUS> getTask(Subtasks& task);
  STATUS createSubtask(class Lines& lines);
  void resume();
};

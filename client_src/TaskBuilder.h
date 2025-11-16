/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <mutex>

#include "Runnable.h"
#include "Subtask.h"

#ifdef CRYPTOVARIANT
#include "CryptoVariant.h"
#endif

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
  cryptovariant::CryptoVariant _crypto;
  std::string _buffer;
  void run() override;
  bool start() override { return true; }
 public:
  explicit TaskBuilder(cryptovariant::CryptoVariant crypto);
  ~TaskBuilder() override = default;
  void stop() override;
  std::pair<std::size_t, STATUS> getTask(Subtasks& task);
  STATUS createSubtask(class Lines& lines);
  void resume();
};

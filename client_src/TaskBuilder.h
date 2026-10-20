/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <mutex>

#include "CryptoOperations.h"
#include "FileLinesRange.h"
#include "Runnable.h"
#include "Subtask.h"

using namespace cryptooperations;

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  const std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr> _encryptors;
  std::string _batch;
  Subtasks _subtasks;
  unsigned _subtaskIndex;
  std::mutex _mutex;
  std::condition_variable _conditionTask;
  std::condition_variable _conditionResume;
  bool _resume = false;
  std::string _buffer;
  static FileLinesRange _linesR;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>& encryptors);
  ~TaskBuilder() override = default;
  void stop() override;
  std::pair<std::size_t, STATUS> getTask(Subtasks& task);
  STATUS createSubtask(class Lines& lines);
  void resume();
  static std::string& getSource()  { return _linesR._source; }
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

#include <cryptopp/integer.h>

#include "Runnable.h"
#include "Subtask.h"

enum class STATUS : uint8_t;

using Subtasks = std::deque<Subtask>;

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line, long index);

  const CryptoPP::SecByteBlock& _key;
  std::string _aggregate;
  Subtasks _subtasks;
  std::atomic<unsigned> _subtaskIndexConsumed = 0;
  std::atomic<unsigned> _subtaskIndexProduced = 0;
  std::mutex _mutex;
  std::condition_variable _condition;
  bool _resume = false;
  static Subtask _emptySubtask;
  static Subtasks _emptyTask;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const CryptoPP::SecByteBlock& key);
  ~TaskBuilder() override;
  void stop() override;
  std::tuple<STATUS, Subtasks&> getResult();
  Subtask& getSubtask();
  STATUS createSubtask(class Lines& lines);
  void resume();
};

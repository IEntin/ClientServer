/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;

class TaskProcessor : public std::enable_shared_from_this<TaskProcessor>, public Runnable {
  TaskProcessor(const TaskController& other) = delete;
  bool start() override;
  void run() noexcept override;
  unsigned getNumberObjects() const override;
  TaskProcessor& operator =(const TaskController& other) = delete;
  ObjectCounter<TaskProcessor> _objectCounter;
  TaskControllerWeakPtr _taskController;
 public:
  explicit TaskProcessor(TaskControllerWeakPtr taskController);
  ~TaskProcessor() override;
  void stop() override;
};

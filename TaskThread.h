/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Task.h"
#include <barrier>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

class TaskThread {
public:
  static bool startThreads(ProcessRequest processRequest);
  static void joinThreads();
private:
  TaskThread() = delete;
  ~TaskThread() = delete;
  static void onTaskFinish() noexcept;
  template<typename T>
    static void processTask(T& task, ProcessRequest processRequest);
  static const bool _useStringView;
  static TaskPtrSV _taskSV;
  static TaskPtrST _taskST;
  static unsigned _numberTaskThreads;
  static std::vector<std::thread> _taskThreads;
  static std::barrier<CompletionFunction> _barrier;
};
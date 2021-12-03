#pragma once

#include "TaskTemplate.h"
#include <barrier>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

class TaskRunnable {
public:
  TaskRunnable() = default;
  ~TaskRunnable() = default;
  void operator()(ProcessRequest processRequest) noexcept;
  static void onTaskFinish() noexcept;
  static bool startThreads(ProcessRequest processRequest);
  static void joinThreads();
private:
  static const bool _useStringView;
  static TaskPtrSV _taskSV;
  static TaskPtrST _taskST;
  static unsigned _numberTaskThreads;
  static std::vector<std::thread> _taskThreads;
  static std::barrier<CompletionFunction> _barrier;
};

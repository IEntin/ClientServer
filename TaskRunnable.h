#pragma once

#include <barrier>
#include <memory>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

template<typename Type>
class TaskTemplate;

//using Task = TaskTemplate<std::string_view>;
//using TaskPtr = std::shared_ptr<Task>;
//using Task = TaskTemplate<std::string>;
//using TaskPtr = std::shared_ptr<Task>;

class TaskRunnable {
public:
  TaskRunnable() = default;
  ~TaskRunnable() = default;
  void operator()(ProcessRequest processRequest) noexcept;
  static void onTaskFinish() noexcept;
  static bool startThreads(ProcessRequest processRequest);
  static void joinThreads();
private:
  static unsigned _numberTaskThreads;
  static std::barrier<CompletionFunction> _barrier;
  static std::vector<std::thread> _taskThreads;
};

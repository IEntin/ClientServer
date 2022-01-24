#include "Task.h"

std::mutex Task::_queueMutex;
std::condition_variable Task::_queueCondition;
std::queue<TaskPtr> Task::_queue;
thread_local std::vector<char> Task::_rawInput;
Batch Task::_emptyBatch;

Task::Task() : _response(_emptyBatch) {}

Task::Task(const TaskContext& context, std::vector<char>& input, Batch& response) :
  _context(context), _response(response)  {
  input.swap(_rawInput);
  utility::split(_rawInput, _storage);
  _response.resize(_storage.size());
}

void Task::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_all();
}

TaskPtr Task::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [] { return !_queue.empty(); });
  auto task = _queue.front();
  _queue.pop();
  return task;
}

void Task::process(const TaskContext& context, std::vector<char>& input, Batch& response) {
  try {
    TaskPtr task = std::make_shared<Task>(context, input, response);
    auto future = task->_promise.get_future();
    push(task);
    future.get();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

void Task::finish() {
  try {
    _promise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

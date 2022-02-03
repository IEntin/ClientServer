#include "Task.h"
#include "Utility.h"

std::mutex Task::_queueMutex;
std::condition_variable Task::_queueCondition;
std::queue<TaskPtr> Task::_queue;
thread_local std::vector<char> Task::_rawInput;
Batch Task::_emptyBatch;
// start with empty task
TaskPtr Task::_task(std::make_shared<Task>());

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

void Task::pop() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [] { return !_queue.empty(); });
  _task = _queue.front();
  _queue.pop();
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

std::tuple<std::string_view, bool, size_t> Task::nextImpl() {
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _storage.size()) {
    auto it = std::next(_storage.begin(), pointer);
    return std::make_tuple(std::string_view(it->data(), it->size()),
			   false,
			   std::distance(_storage.begin(), it));
  }
  else
    return std::make_tuple(std::string_view(), true, 0);
}

void Task::finish() {
  _task->finishImpl();
}

void Task::finishImpl() {
  try {
    _promise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

bool Task::isDiagnosticsEnabled() {
  return _task->_context._diagnostics;
}

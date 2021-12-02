#pragma once

#include "Utility.h"
#include <atomic>
#include <future>
#include <iostream>
#include <queue>

using Batch = std::vector<std::string>;

template<class T>
class TaskTemplate;

template<class T>
using TaskTemplatePtr = std::shared_ptr<TaskTemplate<T>>;

template <typename T>
using Requests = std::vector<T>;

template <typename T>
class TaskTemplate {
  TaskTemplate(const TaskTemplate& other) = delete;
  TaskTemplate& operator =(const TaskTemplate& other) = delete;
  static std::mutex _queueMutex;
  static std::condition_variable _queueCondition;
  static std::queue<TaskTemplatePtr<T>> _queue;
  std::string _address;
  Requests<T> _storage;
  static thread_local std::vector<char> _rawInput;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Batch& _response;
  static Batch _emptyBatch;
 public:
  TaskTemplate() : _response(_emptyBatch) {}

  TaskTemplate(std::string_view address, Batch& input, Batch& response) :
    _address(address), _response(response) {
    input.swap(_storage);
    _response.resize(_storage.size());
  }

  TaskTemplate(std::string_view address, std::vector<char>& input, Batch& response) :
    _address(address), _response(response) {
    input.swap(_rawInput);
    static size_t maxNumberRequests = 1;
    _storage.reserve(maxNumberRequests);
    utility::split(_rawInput, _storage);
    if (_storage.size() > maxNumberRequests)
      maxNumberRequests = _storage.size();
    _response.resize(_storage.size());
  }

  static std::shared_ptr<TaskTemplate> instance() {
    static std::shared_ptr<TaskTemplate> instance = std::make_shared<TaskTemplate>();
    return instance;
  }

  size_t size() const { return _storage.size(); }

  bool empty() const { return _storage.empty(); }

  std::tuple<std::string_view, bool, size_t> next() {
    size_t pointer = _pointer.fetch_add(1);
    if (pointer < _storage.size()) {
      typename Requests<T>::iterator it = std::next(_storage.begin(), pointer);
      return std::make_tuple(std::string_view(it->data(), it->size()),
			     false,
			     std::distance(_storage.begin(), it));
    }
    else
      return std::make_tuple(std::string_view(), true, 0);
  }

  static void push(TaskTemplatePtr<T> task) {
    std::lock_guard lock(_queueMutex);
    _queue.push(task);
    _queueCondition.notify_all();
  }

  static TaskTemplatePtr<T> get() {
    TaskTemplatePtr<T> task;
    std::unique_lock lock(_queueMutex);
    _queueCondition.wait(lock, [] { return !_queue.empty(); });
    task = _queue.front();
    _queue.pop();
    return task;
  }

  void updateResponse(size_t index, std::string&& rsp) {
    _response[index].swap(rsp);
  }

  void finish() {
    try {
      _promise.set_value();
    }
    catch (std::future_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
    }
  }

  static void process(std::string_view address, Requests<T>& input, Batch& response) {
    try {
      TaskTemplatePtr<T> task = std::make_shared<TaskTemplate>(address, input, response);
      std::future<void> future = task->_promise.get_future();
      push(task);
      future.get();
    }
    catch (std::future_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
    }
  }

  static void process(std::string_view address,
		      std::vector<char>& received,
		      Batch& response) {
    try {
      TaskTemplatePtr<T> task = std::make_shared<TaskTemplate>(address, received, response);
      std::future<void> future = task->_promise.get_future();
      push(task);
      future.get();
    }
    catch (std::future_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
    }
  }
};

template <typename T> std::mutex TaskTemplate<T>::_queueMutex;
template <typename T> std::condition_variable TaskTemplate<T>::_queueCondition;
template <typename T> std::queue<TaskTemplatePtr<T>> TaskTemplate<T>::_queue;
template <typename T> thread_local std::vector<char> TaskTemplate<T>::_rawInput;
template <typename T> Batch TaskTemplate<T>::_emptyBatch;

using TaskSV = TaskTemplate<std::string_view>;
using TaskST = TaskTemplate<std::string>;
using TaskPtrSV = std::shared_ptr<TaskSV>;
using TaskPtrST = std::shared_ptr<TaskST>;

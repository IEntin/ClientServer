/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"
#include <atomic>
#include <future>
#include <iostream>
#include <queue>

using Batch = std::vector<std::string>;

template<class T>
class Task;

template<class T>
using TaskPtr = std::shared_ptr<Task<T>>;

template <typename T>
using Requests = std::vector<T>;

template <typename T>
class Task {
  Task(const Task& other) = delete;
  Task& operator =(const Task& other) = delete;
  static std::mutex _queueMutex;
  static std::condition_variable _queueCondition;
  static std::queue<TaskPtr<T>> _queue;
  Requests<T> _storage;
  static thread_local std::vector<char> _rawInput;
  std::atomic<size_t> _pointer = 0;
  std::promise<void> _promise;
  Batch& _response;
  static Batch _emptyBatch;
 public:
  Task() : _response(_emptyBatch) {}

  Task(Batch& input, Batch& response) : _response(response) {
    input.swap(_storage);
    _response.resize(_storage.size());
  }

  Task(std::vector<char>& input, Batch& response) : _response(response) {
    input.swap(_rawInput);
    utility::split(_rawInput, _storage);
    _response.resize(_storage.size());
  }

  size_t size() const { return _storage.size(); }

  bool empty() const { return _storage.empty(); }

  std::tuple<std::string_view, bool, size_t> next() {
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

  static void push(TaskPtr<T> task) {
    std::lock_guard lock(_queueMutex);
    _queue.push(task);
    _queueCondition.notify_all();
  }

  static TaskPtr<T> get() {
    std::unique_lock lock(_queueMutex);
    _queueCondition.wait(lock, [] { return !_queue.empty(); });
    auto task = _queue.front();
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

  template<typename I>
  static void process(I& input, Batch& response) {
    try {
      TaskPtr<T> task = std::make_shared<Task>(input, response);
      auto future = task->_promise.get_future();
      push(task);
      future.get();
    }
    catch (std::future_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
    }
  }
};

template <typename T> std::mutex Task<T>::_queueMutex;
template <typename T> std::condition_variable Task<T>::_queueCondition;
template <typename T> std::queue<TaskPtr<T>> Task<T>::_queue;
template <typename T> thread_local std::vector<char> Task<T>::_rawInput;
template <typename T> Batch Task<T>::_emptyBatch;

using TaskSV = Task<std::string_view>;
using TaskST = Task<std::string>;
using TaskPtrSV = std::shared_ptr<TaskSV>;
using TaskPtrST = std::shared_ptr<TaskST>;

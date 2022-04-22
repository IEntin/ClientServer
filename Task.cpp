#include "Task.h"
#include "Utility.h"

ExtractKey Task::_extractKey = nullptr;
ProcessRequest Task::_processRequest = nullptr;

Task::Task(Batch& emptyBatch) : _response(emptyBatch) {}

Task::Task(const HEADER& header, std::vector<char>& input, Batch& response) :
  _header(header), _response(response)  {
  static thread_local std::vector<char> rawInput;
  input.swap(rawInput);
  utility::split(std::string_view(rawInput.data(), rawInput.size()), _storage);
  _response.resize(_storage.size());
}

void Task::sortRequests() {
  std::sort(_storage.begin(), _storage.end(), [] (const auto& t1, const auto& t2) {
	      return std::get<KEY>(t1) < std::get<KEY>(t2);
	    });
}

bool Task::extractKeyNext() {
  if (!_extractKey)
    return false;
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _storage.size()) {
    std::string_view request = std::get<REQUEST>(_storage[pointer]);
    _extractKey(std::get<KEY>(_storage[pointer]), request);
    return true;
  }
  else
    return false;
}

bool Task::processNext() {
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _storage.size()) {
    std::string_view key = std::get<KEY>(_storage[pointer]);
    std::string_view request = std::get<REQUEST>(_storage[pointer]);
    std::string response = _processRequest(key, request);
    _response[std::get<ORIGINALINDEX>(_storage[pointer])].swap(response);
    return true;
  }
  else
    return false;
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

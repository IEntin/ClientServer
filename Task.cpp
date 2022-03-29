#include "Task.h"
#include "Utility.h"

ProcessRequest Task::_processRequest = nullptr;

Task::Task(Batch& emptyBatch) : _response(emptyBatch) {}

Task::Task(const HEADER& header, std::vector<char>& input, Batch& response) :
  _header(header), _response(response)  {
  static thread_local std::vector<char> rawInput;
  input.swap(rawInput);
  utility::split(std::string_view(rawInput.data(), rawInput.size()), _storage);
  _response.resize(_storage.size());
}

bool Task::next() {
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _storage.size()) {
    std::string_view request = _storage[pointer];
    std::string response = _processRequest(request);
    _response[pointer].swap(response);
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

#include "Task.h"
#include "Utility.h"

Task::Task(Batch& emptyBatch) : _response(emptyBatch) {}

Task::Task(const HEADER& header, std::vector<char>& input, Batch& response) :
  _header(header), _response(response)  {
  static thread_local std::vector<char> rawInput;
  input.swap(rawInput);
  utility::split(std::string_view(rawInput.data(), rawInput.size()), _storage);
  _response.resize(_storage.size());
}

std::tuple<std::string_view, bool, size_t> Task::next() {
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
  try {
    _promise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

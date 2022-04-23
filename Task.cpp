#include "Task.h"
#include "Utility.h"

ExtractKey Task::_extractKey = nullptr;
ProcessRequest Task::_processRequest = nullptr;

Task::Task(Batch& emptyBatch) : _response(emptyBatch) {}

Task::Task(const HEADER& header, std::vector<char>& input, Batch& response) :
  _header(header), _response(response)  {
  static thread_local std::vector<char> rawInput;
  input.swap(rawInput);
  utility::split(std::string_view(rawInput.data(), rawInput.size()), _tuples);
  _indices.resize(_tuples.size());
  for (unsigned i = 0; i < _indices.size(); ++i) {
    _indices[i] = i;
    std::get<ORIGINALINDEX>(_tuples[i]) = i;
  }
  _response.resize(_tuples.size());
}

void Task::sortIndices() {
  std::sort(_indices.begin(), _indices.end(), [this] (unsigned index1, unsigned index2) {
	      return std::get<KEY>(_tuples[index1]) < std::get<KEY>(_tuples[index2]);
	    });
}

void Task::sortRequests() {
  std::sort(_tuples.begin(), _tuples.end(), [] (const auto& t1, const auto& t2) {
	      return std::get<KEY>(t1) < std::get<KEY>(t2);
 	    });
}

bool Task::extractKeyNext() {
  if (!_extractKey)
    return false;
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _tuples.size()) {
    Tuple& t = _tuples[pointer];
    std::string_view request = std::get<REQUEST>(t);
    _extractKey(std::get<KEY>(t), request);
    return true;
  }
  else
    return false;
}

bool Task::processNext() {
  size_t pointer = _pointer.fetch_add(1);
  if (pointer < _tuples.size()) {
    Tuple& t = _tuples[_indices[pointer]];
    std::string_view key = std::get<KEY>(t);
    std::string_view request = std::get<REQUEST>(t);
    std::string response = _processRequest(key, request);
    _response[std::get<ORIGINALINDEX>(t)].swap(response);
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

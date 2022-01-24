#include "TaskContext.h"

TaskContext::TaskContext(std::string_view headerView, HEADER& header) :
  _header(utility::decodeHeader(headerView)), _diagnostics(std::get<3>(_header))  {
  header = _header;
}

TaskContext::TaskContext(const HEADER& header) :
  _header(header), _diagnostics(std::get<3>(_header)) {}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Logger.h"

LOG_LEVEL Logger::_threshold = LOG_LEVEL::ERROR;

void Logger::translateLogThreshold(std::string_view configName) {
  constexpr int size = static_cast<int>(LOG_LEVEL::NUMBEROF);
  for (int index = 0; index < size; ++index) {
    if (configName == levelNames[index]) {
      _threshold = static_cast<LOG_LEVEL>(index);
      break;
    }
  }
}

Logger& Logger::printPrefix(const boost::source_location& location) {
  try {
    if (_level < _threshold || !_displayPrefix)
      return *this;
    static thread_local std::string output;
    output = "[";
    std::string_view leveName(levelNames[std::underlying_type_t<LOG_LEVEL>(_level)]);
    output.append(leveName);
    output.push_back(']');
    output.push_back(' ');
    output.append(location.file_name());
    output.push_back(':');
    ioutility::toChars(location.line(), output);
    output.push_back(' ');
    output.append(location.function_name());
    output.push_back(' ');
    _stream.write(output.data(), output.size());
    return *this;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return *this;
}

void Logger::integerWrite(Logger& logger, long value) {
  char buffer[ioutility::CONV_BUFFER_SIZE] = {};
  auto length = ioutility::toChars(value, buffer);
  logger._stream.write(buffer, length);
}

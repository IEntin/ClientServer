/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Logger.h"

#include "IOUtility.h"

using ioutility::operator<<;

LOG_LEVEL Logger::_threshold = LOG_LEVEL::EXPECTED;

void Logger::translateLogThreshold(std::string_view configName) {
  if ("TRACE" == configName)
    _threshold = LOG_LEVEL::TRACE;
  else if ("DEBUG" == configName)
    _threshold = LOG_LEVEL::DEBUG;
  else if ("INFO" == configName)
    _threshold = LOG_LEVEL::INFO;
  else if ("WARN" == configName)
    _threshold = LOG_LEVEL::WARN;
  else if ("EXPECTED" == configName)
    _threshold = LOG_LEVEL::EXPECTED;
  else if ("ERROR" == configName)
    _threshold = LOG_LEVEL::ERROR;
  else if ("ALWAYS" == configName)
    _threshold = LOG_LEVEL::ALWAYS;
}

Logger& Logger::printLocation(const boost::source_location& location) {
  try {
    if (_level < _threshold || !_displayLocation)
      return *this;
    static thread_local std::string output;
    output << '[';
    std::string_view levelName(levelNames[std::to_underlying(_level)]);
    output << levelName << ']' << ' ' << location.file_name() << ':'
	   << location.line() << ' ' << location.function_name() << ' ';
    _stream.write(output.data(), std::ssize(output));
    return *this;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return *this;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Logger.h"
#include "Utility.h"

LOG_LEVEL Logger::_threshold = LOG_LEVEL::ALWAYS;

void Logger::translateLogThreshold(std::string_view configName) {
  for (unsigned index = 0; index < sizeof(levelNames)/sizeof(std::string_view); ++index) {
    if (configName == levelNames[index]) {
      _threshold = static_cast<LOG_LEVEL>(index);
    }
  }
}

Logger& Logger::printPrefix(const char* file, int line, const char* func) {
  try {
    if (_level < _threshold || !_displayPrefix)
      return *this;
    static thread_local std::string output;
    output.resize(0);
    output.push_back('[');
    std::string_view leveName(levelNames[static_cast<int>(_level)]);
    output.insert(output.end(), leveName.cbegin(), leveName.cend());
    output.push_back(']');
    output.push_back(' ');
    output.insert(output.size(), file);
    output.push_back(':');
    utility::toChars(line, output);
    output.push_back(' ');
    output.insert(output.size(), func);
    output.push_back(' ');
    _stream.write(output.data(), output.size());
    return *this;
  }
  catch (const std::ios_base::failure& fail) {
    std::cerr << fail.what() << std::endl;
  }
  return *this;
}
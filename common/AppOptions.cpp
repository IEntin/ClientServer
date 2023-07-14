/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AppOptions.h"
#include "Logger.h"

AppOptions::AppOptions(const std::string& fileName) :
  _fileName(fileName), _initialized(initialize()) {}

bool AppOptions::initialize() {
  try {
    boost::property_tree::read_json(_fileName, _ptree);
  }
  catch (const std::exception& e) {
    static thread_local auto& printOnce[[maybe_unused]] =
      LogError << e.what() << ",\n"
	       << "default values will be returned, by design in tests." << '\n';
    return false;
  }
  catch (...) {
    LogError << "unknown exception." << '\n';
    return false;
  }
  return true;
}

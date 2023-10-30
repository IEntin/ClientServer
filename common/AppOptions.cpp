/*
 *  Copyright (C) 2021 Ilya Entin
 */


#include "AppOptions.h"
#include "Logger.h"

AppOptions::AppOptions(std::string_view fileName) :
  _initialized(initialize(fileName)) {}

bool AppOptions::initialize(std::string_view fileName) {
  try {
    boost::property_tree::read_json(fileName.data(), _ptree);
  }
  catch (const std::exception& e) {
    static auto& printOnce[[maybe_unused]] =
      LogError << e.what() << ",\n"
	       << "default values will be returned, by design in tests." << '\n';
    return false;
  }
  return true;
}

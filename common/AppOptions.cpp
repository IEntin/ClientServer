/*
 *  Copyright (C) 2021 Ilya Entin
 */


#include "AppOptions.h"

#include <filesystem>

#include "Logger.h"

AppOptions::AppOptions(std::string_view fileName) :
  _initialized(initialize(fileName)) {}

bool AppOptions::initialize(std::string_view fileName) {
  if (!fileName.empty() && !std::filesystem::exists(fileName)) {
    LogError << fileName << " not found\n";
      return false;
  }
  try {
    boost::property_tree::read_json(fileName.data(), _ptree);
    return true;
  }
  catch (const std::exception& e) {
    static auto& printOnce[[maybe_unused]] =
      Expected << e.what() << ",\n"
	       << "default values will be returned." << '\n';
  }
  return false;
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AppOptions.h"
#include "Utility.h"

AppOptions::AppOptions(const std::string& fileName) :
  _fileName(fileName), _initialized(initialize()) {}

bool AppOptions::initialize() {
  try {
    boost::property_tree::read_json(_fileName, _ptree);
  }
  catch (std::exception& e) {
    static auto& printOnce[[maybe_unused]] =
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what()
	   << ",\n" << "default values will be returned, by design in tests." << std::endl;
    return false;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":unknown exception." << std::endl;
    return false;
  }
  return true;
}

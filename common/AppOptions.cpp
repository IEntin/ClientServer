/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "AppOptions.h"
#include <iostream>

AppOptions::AppOptions(const std::string& fileName) : _fileName(fileName) {
  initialize();
}

bool AppOptions::initialize() {
  try {
    boost::property_tree::read_json(_fileName, _ptree);
  }
  catch (std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what()
	      << ",default values will be returned" << std::endl;
    return false;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":unknown exception" << std::endl;
    return false;
  }
  return true;
}

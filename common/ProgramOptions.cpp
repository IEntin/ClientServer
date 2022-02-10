/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ProgramOptions.h"
#include <filesystem>
#include <iostream>

boost::property_tree::ptree& ProgramOptions::instance() {
  static boost::property_tree::ptree pTree;
  static bool init [[maybe_unused]] = initialize(pTree);
  return pTree;
}

bool ProgramOptions::initialize(boost::property_tree::ptree& pTree) {
  try {
    static const std::string baseJsonName = "ProgramOptions.json";
    std::string directory = std::filesystem::current_path();
    std::string jsonName = directory + '/' + baseJsonName;
    boost::property_tree::read_json(jsonName, pTree);
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

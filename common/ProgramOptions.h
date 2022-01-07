/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

class ProgramOptions {
 public:
  static boost::property_tree::ptree& instance();
  template<typename T>
    static const T get(const std::string& name, const T& def) {
    return instance().get(name, def);
  }
  static bool initialize(boost::property_tree::ptree& pTree);
 private:
  ProgramOptions() = delete;
  ~ProgramOptions() = delete;
  ProgramOptions(const ProgramOptions& other) = delete;
};

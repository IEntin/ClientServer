/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

class AppOptions {
 public:
  AppOptions(const std::string& fileName);
  ~AppOptions() = default;

  template<typename T>
    const T get(const std::string& name, const T& def) {
    if (!_initialized)
      return def;
    return _ptree.get(name, def);
  }

  bool initialize();
 private:
  AppOptions(const AppOptions& other) = delete;
  const std::string _fileName;
  boost::property_tree::ptree _ptree;
  const bool _initialized;
};

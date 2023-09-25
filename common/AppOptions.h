/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#define BOOST_SPIRIT_THREADSAFE
#include <boost/core/noncopyable.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

class AppOptions : private boost::noncopyable {
 public:
  AppOptions(std::string_view fileName);
  ~AppOptions() = default;

  template<typename T>
    const T get(const std::string& name, const T& def) {
    if (!_initialized)
      return def;
    return _ptree.get(name, def);
  }

  bool initialize(std::string_view fileName);
 private:
  boost::property_tree::ptree _ptree;
  const bool _initialized;
};

/*
*  Copyright (C) 2021 Ilya Entin
*/

#include <boost/json/src.hpp>

#include "BoostJsonParser.h"
#include "Logger.h"
#include "Utility.h"

bool parseJson(std::string_view fileName, boost::json::value& jv) {
  std::string json_string;
  utility::readFile(fileName, json_string);
  boost::system::error_code ec;
  jv = boost::json::parse(json_string, ec);
  if (ec) {
    LogError << "Error parsing JSON: " << ec.what() << '\n';
    return false;
  }
  return true;
}

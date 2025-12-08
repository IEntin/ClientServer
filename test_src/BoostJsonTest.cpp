/*
*  Copyright (C) 2021 Ilya Entin
*/

#include <boost/json/src.hpp>

#include "Logger.h"
#include "TestEnvironment.h"

// ./testbin --gtest_filter=boostjsonparser*

TEST(boostjsonparser, 1) {
  std::string json_string;
  utility::readFile("ServerOptions.json", json_string);
  boost::system::error_code ec;
  boost::json::value jv = boost::json::parse(json_string, ec);
  if (ec) {
    LogError << "Error parsing JSON: " << ec.what() << '\n';
    ASSERT_TRUE(false);
  }
  Info << jv.at("AdsFileName").as_string() << '\n';
  ASSERT_TRUE(jv.at("AdsFileName") == "data/ads.txt");
  Info << jv.at("Policy").as_string() << '\n';
  ASSERT_TRUE(jv.at("Policy") == "NOSORTINPUT");
  Info << jv.at("NumberWorkThreads").as_int64() << '\n';
  ASSERT_TRUE(jv.at("NumberWorkThreads") == 0);
  Info << jv.at("doEncrypt").as_bool() << '\n';
  ASSERT_TRUE(jv.at("doEncrypt") == true);
}

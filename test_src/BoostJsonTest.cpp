/*
*  Copyright (C) 2021 Ilya Entin
*/

#include <boost/system/error_code.hpp>

#include "BoostJsonParser.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include "Utility.h"

//for i in {1..10}; do ./testbin --gtest_filter=boostjsonparser*;done

TEST(boostjsonparser, 1) {

  boost::json::value jv;
  ASSERT_TRUE(parseJson("ServerOptions.json", jv));
  Info << jv.at("AdsFileName").as_string() << '\n';
  ASSERT_TRUE(jv.at("AdsFileName") == "data/ads.txt");
  Info << jv.at("Policy").as_string() << '\n';
  ASSERT_TRUE(jv.at("Policy") == "NOSORTINPUT");
  Info << jv.at("NumberWorkThreads").as_int64() << '\n';
  ASSERT_TRUE(jv.at("NumberWorkThreads") == 0);
  Info << jv.at("doEncrypt").as_bool() << '\n';
  ASSERT_TRUE(jv.at("doEncrypt") == true);
}

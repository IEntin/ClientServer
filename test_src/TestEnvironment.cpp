/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "ClientOptions.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <filesystem>

ServerOptions TestEnvironment::_serverOptions;
std::ostringstream TestEnvironment::_oss;
ClientOptions TestEnvironment::_clientOptions("", &_oss);
std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;
const ServerOptions TestEnvironment::_serverOptionsOrg;
const ClientOptions TestEnvironment::_clientOptionsOrg("", &_oss);

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  try {
    _source = utility::readFile(_clientOptions._sourceName);
    _outputD = utility::readFile("data/outputD.txt");
    _outputND = utility::readFile("data/outputND.txt");
    _outputAltFormatD = utility::readFile("data/outputAltFormatD.txt");
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() <<std::endl;
    std::exit(1);
  }
}

void TestEnvironment::TearDown() {
  Metrics::save();
  Metrics::print();
}

void TestEnvironment::reset() {
  _serverOptions = _serverOptionsOrg;
  _oss.str("");
  _clientOptions = _clientOptionsOrg;
}

int main(int argc, char** argv) {
  TestEnvironment* env = new TestEnvironment();
  ::testing::AddGlobalTestEnvironment(env);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

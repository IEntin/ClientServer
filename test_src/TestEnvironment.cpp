/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

#include <filesystem>

#include "ClientOptions.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Utility.h"

std::ostringstream TestEnvironment::_oss;
std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  try {
    // Server and client in tests run in the same process
    // and the logic for *common* options always selects
    // values form ServerOptions unless ClientOptions
    // is explicitly specified. Extracted default values 
    // are the same for the server and client.
    // Tests do not change these values although values 
    // in json files can be different and arbitrarely changeable in
    // real apps. The list of affected settings with default values:
    //
    // _setPipeBufferSize = true
    // _pipeSize = 1000000
    // _numberRepeatENXIO = 50
    // _ENXIOwait = 10
    //
    ServerOptions::parse("");
    ClientOptions::parse("", &_oss);
    utility::readFile(ClientOptions::_sourceName, _source);
    utility::readFile("data/outputD.txt", _outputD);
    utility::readFile("data/outputND.txt", _outputND);
    utility::readFile("data/outputAltFormatD.txt", _outputAltFormatD);
  }
  catch (const std::exception& e) {
    LogError << e.what() <<'\n';
    std::exit(1);
  }
}

void TestEnvironment::TearDown() {
  try {
    Metrics::save();
    Metrics::print(LOG_LEVEL::ERROR, std::cerr, false);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

void TestEnvironment::reset() {
  _oss.str("");
  ServerOptions::parse("");
  ClientOptions::parse("", &_oss);
}

int main(int argc, char** argv) {
  TestEnvironment* env = new TestEnvironment();
  ::testing::AddGlobalTestEnvironment(env);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

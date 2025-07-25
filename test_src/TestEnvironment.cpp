/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

#include <filesystem>

#include "ClientOptions.h"
#include "DebugLog.h"
#include "Metrics.h"
#include "ServerOptions.h"
#include "Utility.h"

std::ostringstream TestEnvironment::_oss;
std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;
thread_local std::string TestEnvironment::_buffer;

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  ServerOptions::parse("");
  ClientOptions::parse("", &_oss);
  utility::readFile(ClientOptions::_sourceName, _source);
  utility::readFile("data/outputD.txt", _outputD);
  utility::readFile("data/outputND.txt", _outputND);
  utility::readFile("data/outputAltFormatD.txt", _outputAltFormatD);
  DebugLog::setDebugLog(APPTYPE::TESTS);
}

void TestEnvironment::TearDown() {
  Metrics::save();
  Metrics::print(LOG_LEVEL::ALWAYS, std::clog, false);
}

void TestEnvironment::reset() {
  _oss.str("");
  ServerOptions::parse("");
  ClientOptions::parse("", &_oss);
}

int main(int argc, char** argv) {
  try {
    if (sodium_init() < 0) {
      LogError << "sodium_init failure\n";
      return 1;
    }
    utility::setTestbinTerminal(getenv("GNOME_TERMINAL_SCREEN"));
    TestEnvironment* env = new TestEnvironment();
    ::testing::AddGlobalTestEnvironment(env);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
  return 1;
}

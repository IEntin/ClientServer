/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

#include "ClientOptions.h"
#include "DebugLog.h"
#include "Metrics.h"
#include "ServerOptions.h"

std::ostringstream TestEnvironment::_oss;
thread_local std::string TestEnvironment::_buffer;

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  ServerOptions::parse("ServerOptions.json");
  ClientOptions::parse("", &_oss);
  _outputD = utility::readFile("data/outputD.txt");
  _outputND = utility::readFile("data/outputND.txt");
  _outputAltFormatD = utility::readFile("data/outputAltFormatD.txt");
  DebugLog::setDebugLog(APPTYPE::TESTS);
}

void TestEnvironment::TearDown() {
  Metrics::save();
  Metrics::print(LOG_LEVEL::ALWAYS, std::clog, false);
}

void TestEnvironment::reset() {
  _oss.str("");
  ServerOptions::parse("ServerOptions.json");
  ClientOptions::parse("", &_oss);
}

int main(int argc, char** argv) {
  try {
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

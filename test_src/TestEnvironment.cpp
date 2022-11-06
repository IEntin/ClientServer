/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "ClientOptions.h"
#include "CommonConstants.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <boost/interprocess/sync/named_mutex.hpp>

ServerOptions TestEnvironment::_serverOptions;
std::ostringstream TestEnvironment::_oss;
ClientOptions TestEnvironment::_clientOptions("", &_oss);
std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;
const ServerOptions TestEnvironment::_serverOptionsOrg;
TaskControllerPtr TestEnvironment::_taskController;
const ClientOptions TestEnvironment::_clientOptionsOrg("", &_oss);

TestEnvironment::TestEnvironment() {}

TestEnvironment::~TestEnvironment() {
  if (!boost::interprocess::named_mutex::remove(WAKEUP_MUTEX))
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " mamed_mutex remove failed." << std::endl;
}

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  try {
    _source = utility::readFile(_clientOptions._sourceName);
    _outputD = utility::readFile("data/outputD.txt");
    _outputND = utility::readFile("data/outputND.txt");
    _outputAltFormatD = utility::readFile("data/outputAltFormatD.txt");
    _taskController = TaskController::instance(&_serverOptions, TaskControllerOps::RECREATE);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() <<std::endl;
  }
}

void TestEnvironment::TearDown() {
  _taskController->stop();
}

void TestEnvironment::reset() {
  _serverOptions = _serverOptionsOrg;
  _oss.str("");
  _clientOptions = _clientOptionsOrg;
}

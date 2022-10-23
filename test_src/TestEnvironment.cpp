/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "MemoryPool.h"
#include "TaskController.h"
#include "Utility.h"
#include "ClientOptions.h"
#include "ServerOptions.h"
#include <cassert>

std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;
ServerOptions TestEnvironment::_serverOptions;
const ServerOptions TestEnvironment::_serverOptionsOrg;
RunnablePtr TestEnvironment::_taskController;
std::ostringstream TestEnvironment::_oss;
ClientOptions TestEnvironment::_clientOptions("", &_oss);
const ClientOptions TestEnvironment::_clientOptionsOrg("", &_oss);

TestEnvironment::TestEnvironment() {
  assert(std::atomic<bool>{}.is_lock_free());
}

TestEnvironment::~TestEnvironment() {}

void TestEnvironment::SetUp() {
  signal(SIGPIPE, SIG_IGN);
  try {
    ClientOptions clientOptions("", nullptr);
    _source = utility::readFile(clientOptions._sourceName);
    _outputD = utility::readFile("data/outputD.txt");
    _outputND = utility::readFile("data/outputND.txt");
    _outputAltFormatD = utility::readFile("data/outputAltFormatD.txt");
    _taskController = TaskController::instance(&_serverOptions);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() <<std::endl;
  }
}

void TestEnvironment::TearDown() {}

void TestEnvironment::reset() {
  _serverOptions = _serverOptionsOrg;
  MemoryPool::setExpectedSize(_serverOptionsOrg._bufferSize);
  _oss.str("");
  _clientOptions = _clientOptionsOrg;
}

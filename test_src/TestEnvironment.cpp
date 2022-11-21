/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "ClientOptions.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"
#include <filesystem>
#include <sys/resource.h>

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

TestEnvironment::TestEnvironment() : _pid(getpid()) {
  std::string prefix = std::string("/proc/").append(std::to_string(_pid));
  _procFdPath = prefix;
  _procFdPath.append("/fd");
  _procThreadPath = prefix;
  _procThreadPath.append("/task");
}

TestEnvironment::~TestEnvironment() {}

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
    std::exit(1);
  }
}

void TestEnvironment::TearDown() {
  try {
    size_t numberFds =
      std::distance(std::filesystem::directory_iterator(_procFdPath), std::filesystem::directory_iterator{});
    CERR << "\'lsof\'=" << numberFds << std::endl;
    size_t numberThreads =
      std::distance(std::filesystem::directory_iterator(_procThreadPath), std::filesystem::directory_iterator{});
    CERR << "numberThreads=" << numberThreads << std::endl;
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
      return;
    }
    CERR << "maxRss=" << usage.ru_maxrss << std::endl;
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
  _taskController->stop();
}

void TestEnvironment::reset() {
  _serverOptions = _serverOptionsOrg;
  _oss.str("");
  _clientOptions = _clientOptionsOrg;
}

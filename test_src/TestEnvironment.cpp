/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "TaskController.h"
#include "Utility.h"
#include "ClientOptions.h"
#include "ServerOptions.h"
#include <csignal>

volatile std::sig_atomic_t stopFlag = false;

std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;
std::string TestEnvironment::_outputND;
std::string TestEnvironment::_outputAltFormatD;
ServerOptions TestEnvironment::_serverOptions;
TaskControllerPtr TestEnvironment::_taskController;
const bool TestEnvironment::_orgSortInput = TestEnvironment::_serverOptions._sortInput;
const COMPRESSORS TestEnvironment::_orgServerCompressor = TestEnvironment::_serverOptions._compressor;
const size_t TestEnvironment::_orgServerBufferSize = TestEnvironment::_serverOptions._bufferSize;
std::ostringstream TestEnvironment::_oss;
ClientOptions TestEnvironment::_clientOptions("", &_oss);
const std::string TestEnvironment::_orgSourceName = TestEnvironment::_clientOptions._sourceName;
const COMPRESSORS TestEnvironment::_orgClientCompressor = TestEnvironment::_clientOptions._compressor;
const bool TestEnvironment::_orgClientDiagnostics = TestEnvironment::_clientOptions._diagnostics;
const size_t TestEnvironment::_orgClientBufferSize = TestEnvironment::_clientOptions._bufferSize;

void TestEnvironment::SetUp() {
  try {
    ClientOptions clientOptions("", nullptr);
    _source = utility::readFile(clientOptions._sourceName);
    _outputD = utility::readFile("data/outputD.txt");
    _outputND = utility::readFile("data/outputND.txt");
    _outputAltFormatD = utility::readFile("data/outputAltFormatD.txt");
    _taskController = TaskController::instance(&TestEnvironment::_serverOptions);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ' ' << e.what() <<std::endl;
  }
}

void TestEnvironment::TearDown() {}

void TestEnvironment::reset() {
  _serverOptions._sortInput = _orgSortInput;
  _serverOptions._compressor = _orgServerCompressor;
  _serverOptions._bufferSize = _orgServerBufferSize;
  _taskController->setMemoryPoolSize(_orgServerBufferSize);
  _oss.str("");
  _clientOptions._compressor = _orgClientCompressor;
  _clientOptions._bufferSize = _orgClientBufferSize;
  _clientOptions._sourceName = _orgSourceName;
  _clientOptions._diagnostics = _orgClientDiagnostics;
}

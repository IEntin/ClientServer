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
TaskControllerPtr TestEnvironment::_taskController;
ServerOptions TestEnvironment::_serverOptions;
const COMPRESSORS TestEnvironment::_orgServerCompressor = TestEnvironment::_serverOptions._compressor;
const size_t TestEnvironment::_orgServerBufferSize = TestEnvironment::_serverOptions._bufferSize;
std::ostringstream TestEnvironment::_oss;
TcpClientOptions TestEnvironment::_tcpClientOptions(&_oss);
const COMPRESSORS TestEnvironment::_orgTcpClientCompressor = TestEnvironment::_tcpClientOptions._compressor;
const size_t TestEnvironment::_orgTcpClientBufferSize = TestEnvironment::_tcpClientOptions._bufferSize;
const bool TestEnvironment::_orgTcpClientDiagnostics = TestEnvironment::_tcpClientOptions._diagnostics;
FifoClientOptions TestEnvironment::_fifoClientOptions(&_oss);
const COMPRESSORS TestEnvironment::_orgFifoClientCompressor = TestEnvironment::_fifoClientOptions._compressor;
const size_t TestEnvironment::_orgFifoClientBufferSize = TestEnvironment::_fifoClientOptions._bufferSize;
const bool TestEnvironment::_orgFifoClientDiagnostics = TestEnvironment::_fifoClientOptions._diagnostics;

void TestEnvironment::SetUp() {
  try {
    ClientOptions clientOptions;
    _source = utility::readFile(clientOptions._sourceName);
    _outputD = utility::readFile("data/outputD.txt");
    _outputND = utility::readFile("data/outputND.txt");
    ServerOptions serverOptions;
    _taskController = TaskController::instance(&serverOptions);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << e.what() <<std::endl;
  }
}

void TestEnvironment::TearDown() {}

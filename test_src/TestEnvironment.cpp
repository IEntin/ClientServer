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

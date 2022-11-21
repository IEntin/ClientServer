/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <gtest/gtest.h>
#include <sstream>

struct ServerOptions;
struct ClientOptions;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment();

  ~TestEnvironment() override;

  void SetUp() override;

  void TearDown() override;

  static void reset();

  const size_t _pid;
  std::string _procFdPath;
  std::string _procThreadPath;
  static ServerOptions _serverOptions;
  static std::ostringstream _oss;
  static ClientOptions _clientOptions;
  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static TaskControllerPtr _taskController;
 private:
  static const ServerOptions _serverOptionsOrg;
  static const ClientOptions _clientOptionsOrg;
};

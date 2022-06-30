/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <gtest/gtest.h>
#include <sstream>

struct ServerOptions;
struct ClientOptions;
struct TcpClientOptions;
struct FifoClientOptions;
enum class COMPRESSORS : int;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class TestEnvironment : public ::testing::Environment {
public:
  virtual ~TestEnvironment() = default;

  virtual void SetUp();

  virtual void TearDown();

  static void reset();

  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static ServerOptions _serverOptions;
  static const ServerOptions _serverOptionsOrg;
  static TaskControllerPtr _taskController;
  static ServerOptions _serverOptionsEcho;
  static ServerOptions _serverOptionsEchoOrg;
  static TaskControllerPtr _taskControllerEcho;
  static std::ostringstream _oss;
  static ClientOptions _clientOptions;
  static const ClientOptions _clientOptionsOrg;
};

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
enum class COMPRESSORS : char;

using RunnablePtr = std::shared_ptr<class Runnable>;

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment();

  ~TestEnvironment() override;

  void SetUp() override;

  void TearDown() override;

  static void reset();

  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static ServerOptions _serverOptions;
  static RunnablePtr _taskController;
  static std::ostringstream _oss;
  static ClientOptions _clientOptions;
 private:
  static const ServerOptions _serverOptionsOrg;
  static const ClientOptions _clientOptionsOrg;
};

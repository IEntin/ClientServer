/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <gtest/gtest.h>
#include <sstream>

struct ServerOptions;
struct TcpClientOptions;
struct FifoClientOptions;
enum class COMPRESSORS : short unsigned int;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class TestEnvironment : public ::testing::Environment {
public:
  virtual ~TestEnvironment() = default;

  virtual void SetUp();

  virtual void TearDown();

  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static TaskControllerPtr _taskController;
  static ServerOptions _serverOptions;
  static const COMPRESSORS _orgServerCompressor;
  static const size_t _orgServerBufferSize;
  static std::ostringstream _oss;
  static TcpClientOptions _tcpClientOptions;
  static const COMPRESSORS _orgTcpClientCompressor;
  static const size_t _orgTcpClientBufferSize;
  static const bool _orgTcpClientDiagnostics;
  static FifoClientOptions _fifoClientOptions;
  static const COMPRESSORS _orgFifoClientCompressor;
  static const size_t _orgFifoClientBufferSize;
  static const bool _orgFifoClientDiagnostics;
};

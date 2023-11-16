/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment() {}

  ~TestEnvironment() override {}

  void SetUp() override;

  void TearDown() override;

  static void reset();

  static std::ostringstream _oss;
  static struct ServerOptions _serverOptions;
  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
};

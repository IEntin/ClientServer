/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sstream>

#include <gtest/gtest.h>

class TestEnvironment : public ::testing::Environment {
public:

  TestEnvironment() = default;

  ~TestEnvironment() override = default;

  void SetUp() override;

  void TearDown() override;

  static void reset();

  static std::ostringstream _oss;
  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static std::string _outputAltFormatD;
  static thread_local std::string _buffer;

};

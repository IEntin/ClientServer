/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <gtest/gtest.h>

class TestEnvironment : public ::testing::Environment {
public:
  virtual ~TestEnvironment() = default;

  virtual void SetUp();

  virtual void TearDown();

  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <gtest/gtest.h>

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class TestEnvironment : public ::testing::Environment {
public:
  virtual ~TestEnvironment() = default;

  virtual void SetUp();

  virtual void TearDown();

  static std::string _source;
  static std::string _outputD;
  static std::string _outputND;
  static TaskControllerPtr _taskController;
};

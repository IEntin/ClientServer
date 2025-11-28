/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Lines2.h"

class FileLines2 : public Lines2 {
 public:
  explicit FileLines2(std::string_view fileName, char delimiter = '\n', bool keepDelimiter = false);
  ~FileLines2() override = default;
  bool getLine(std::string&) override;
 protected:
  std::string _source;
};

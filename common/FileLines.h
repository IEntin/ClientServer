/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Lines.h"

#include <fstream>

class FileLines : public Lines {
 public:
  explicit FileLines(std::string_view fileName, char delimiter = '\n', bool keepDelimiter = false);
  ~FileLines() override = default;
 private:
  std::size_t getInputPosition() override;
  bool refillBuffer() override;
  std::ifstream _stream;
};

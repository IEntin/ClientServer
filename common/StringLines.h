/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Lines.h"

class StringLines : public Lines {
 public:
  StringLines(std::string_view source);
  ~StringLines() override {}
 private:
  std::size_t getInputPosition() override;
  bool refillBuffer() override;
  const std::string_view _source;
  std::size_t _inputPosition = 0;
};

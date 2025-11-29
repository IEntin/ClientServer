/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Lines2.h"

class StringLines2 : public Lines2 {
 public:
  explicit StringLines2(std::string_view source, char delimiter = '\n', bool keepDelimiter = false);
  ~StringLines2() override = default;
  bool getLine(boost::static_string<MAXSUBSTRSIZE>& line) override;
 protected:
  std::string _source;
};

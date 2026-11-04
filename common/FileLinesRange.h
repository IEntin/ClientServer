/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <string>
#include <vector>

#include "ClientOptions.h"
#include "Utility.h"

#pragma once

class FileLinesRange {
 public:
  explicit FileLinesRange(const std::string& globalSource = _dataRequests,
			  char delimiter = '\n',
			  int keepDelimiter = 1);
  ~FileLinesRange()  = default;
  const std::string& _source;
  static std::string _sourceWithIds;
  static std::vector<std::string_view> _lines;
  static  std::vector<std::string_view> _linesWithId;
  bool getLine(std::string_view& line);
  bool getLineWithId(std::string_view& lineWithId);
  bool createLineWithId(std::string& lineWithId, std::size_t index);
  std::size_t _linesIndex;
  std::size_t _linesWithIdIndex;
  void resetIndices();
};
